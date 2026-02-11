#include "skpch.h"
#include "ImGuiRenderer.h"

#include "Shark/Core/Application.h"
#include "Shark/Render/Renderer.h"
#include "Shark/Debug/Profiler.h"

#include <nvrhi/utils.h>

namespace Shark {

	bool ImGuiRenderer::Initialize(Ref<SwapChain> swapchain)
	{
		m_Swapchain = swapchain;

		ImGuiIO& io = ImGui::GetIO();
		io.BackendRendererName = "Shark NVRHI";
		io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
		io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
		io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;

		nvrhi::IDevice* device = Application::Get().GetDeviceManager()->GetDevice();

		m_CommandBuffer = RenderCommandBuffer::Create("ImGuiRenderer");

		Ref<Shader> shader = Renderer::GetShaderLibrary()->Get("ImGui");
		m_VertexShader = shader->GetHandle(nvrhi::ShaderType::Vertex);
		m_PixelShader = shader->GetHandle(nvrhi::ShaderType::Pixel);

		// create attribute layout object
		nvrhi::VertexAttributeDesc vertexAttribLayout[] = {
			{ "POSITION", nvrhi::Format::RG32_FLOAT,  1, 0, offsetof(ImDrawVert,pos), sizeof(ImDrawVert), false },
			{ "TEXCOORD", nvrhi::Format::RG32_FLOAT,  1, 0, offsetof(ImDrawVert,uv),  sizeof(ImDrawVert), false },
			{ "COLOR",    nvrhi::Format::RGBA8_UNORM, 1, 0, offsetof(ImDrawVert,col), sizeof(ImDrawVert), false },
		};

		m_ShaderAttribLayout = device->createInputLayout(vertexAttribLayout, sizeof(vertexAttribLayout) / sizeof(vertexAttribLayout[0]), m_VertexShader);

		// create PSO
		{
			nvrhi::BlendState blendState;
			blendState.targets[0].setBlendEnable(true)
				.setSrcBlend(nvrhi::BlendFactor::SrcAlpha)
				.setDestBlend(nvrhi::BlendFactor::InvSrcAlpha)
				.setSrcBlendAlpha(nvrhi::BlendFactor::InvSrcAlpha)
				.setDestBlendAlpha(nvrhi::BlendFactor::Zero);

			auto rasterState = nvrhi::RasterState()
				.setFillSolid()
				.setCullNone()
				.setScissorEnable(true)
				.setDepthClipEnable(true);

			auto depthStencilState = nvrhi::DepthStencilState()
				.disableDepthTest()
				.enableDepthWrite()
				.disableStencil()
				.setDepthFunc(nvrhi::ComparisonFunc::Always);

			nvrhi::RenderState renderState;
			renderState.blendState = blendState;
			renderState.depthStencilState = depthStencilState;
			renderState.rasterState = rasterState;

			nvrhi::BindingLayoutDesc layoutDesc;
			layoutDesc.visibility = nvrhi::ShaderType::All;
			layoutDesc.bindings = {
				nvrhi::BindingLayoutItem::PushConstants(0, sizeof(glm::vec2) * 2),
				nvrhi::BindingLayoutItem::Texture_SRV(0),
				nvrhi::BindingLayoutItem::Sampler(0)
			};
			m_BindingLayout = device->createBindingLayout(layoutDesc);

			nvrhi::GraphicsPipelineDesc pipelineDesc;
			pipelineDesc.primType = nvrhi::PrimitiveType::TriangleList;
			pipelineDesc.inputLayout = m_ShaderAttribLayout;
			pipelineDesc.VS = m_VertexShader;
			pipelineDesc.PS = m_PixelShader;
			pipelineDesc.renderState = renderState;
			pipelineDesc.bindingLayouts = { m_BindingLayout };

			m_Pipeline = device->createGraphicsPipeline(pipelineDesc, swapchain->GetFramebufferInfo());
		}

		{
			const auto desc = nvrhi::SamplerDesc()
				.setAllAddressModes(nvrhi::SamplerAddressMode::Wrap)
				.setAllFilters(true);

			m_FontSampler = device->createSampler(desc);
		}

		return true;
	}

	void ImGuiRenderer::DestroyTextures()
	{
		// Destroy all textures
		for (ImTextureData* tex : ImGui::GetPlatformIO().Textures)
			if (tex->RefCount == 1)
				DestroyTexture(tex);
	}

	bool ImGuiRenderer::Render(ImGuiViewport* viewport)
	{
		SK_PROFILE_FUNCTION();
		SK_PERF_SCOPED("ImGui Render");

		const ImDrawData* drawData = viewport->DrawData;

		m_CommandBuffer->Begin();

		Renderer::Submit([commandBuffer = m_CommandBuffer, swapchain = m_Swapchain]()
		{
			auto commandList = commandBuffer->GetHandle();
			nvrhi::utils::ClearColorAttachment(commandList, swapchain->GetCurrentFramebuffer(), 0, { 1.0f, 0.0f, 0.0f, 1.0f });
		});

		if (drawData->Textures)
			for (ImTextureData* texture : *drawData->Textures)
				UpdateTexture(m_CommandBuffer, texture);

		if (!UpdateGeometry(m_CommandBuffer, drawData))
		{
			m_CommandBuffer->End();
			return false;
		}

		struct PushConstants
		{
			glm::vec2 Translation;
			glm::vec2 Scale;
		};

		PushConstants pushConstants;
		pushConstants.Scale.x = 2.0f / drawData->DisplaySize.x;
		pushConstants.Scale.y = 2.0f / drawData->DisplaySize.y;
		pushConstants.Translation.x = -1.0f - drawData->DisplayPos.x * pushConstants.Scale.x;
		pushConstants.Translation.y = -1.0f - drawData->DisplayPos.y * pushConstants.Scale.y;

		// set up graphics state
		nvrhi::GraphicsState drawState;
		
		drawState.pipeline = m_Pipeline;

		drawState.viewport.viewports.push_back(nvrhi::Viewport(drawData->DisplaySize.x * drawData->FramebufferScale.x,
															   drawData->DisplaySize.y * drawData->FramebufferScale.y));
		drawState.viewport.scissorRects.resize(1);  // updated below

		nvrhi::VertexBufferBinding vbufBinding;
		vbufBinding.buffer = m_VertexBuffer;
		vbufBinding.slot = 0;
		vbufBinding.offset = 0;
		drawState.vertexBuffers.push_back(vbufBinding);

		drawState.indexBuffer.buffer = m_IndexBuffer;
		drawState.indexBuffer.format = (sizeof(ImDrawIdx) == 2 ? nvrhi::Format::R16_UINT : nvrhi::Format::R32_UINT);
		drawState.indexBuffer.offset = 0;

		// render command lists
		int vtxOffset = 0;
		int idxOffset = 0;
		const ImVec2 clipOffset = drawData->DisplayPos;
		const ImVec2 clipScale = drawData->FramebufferScale;
		for (int n = 0; n < drawData->CmdListsCount; n++)
		{
			const ImDrawList* cmdList = drawData->CmdLists[n];
			for (int i = 0; i < cmdList->CmdBuffer.Size; i++)
			{
				const ImDrawCmd* pCmd = &cmdList->CmdBuffer[i];
				SK_CORE_VERIFY(pCmd->UserCallback == nullptr, "UserCallback not supported at the moment");

				drawState.viewport.scissorRects[0] = nvrhi::Rect(static_cast<int>((pCmd->ClipRect.x - clipOffset.x) * clipScale.x),
																 static_cast<int>((pCmd->ClipRect.z - clipOffset.x) * clipScale.x),
																 static_cast<int>((pCmd->ClipRect.y - clipOffset.y) * clipScale.y),
																 static_cast<int>((pCmd->ClipRect.w - clipOffset.y) * clipScale.y));

				const auto& clipRect = drawState.viewport.scissorRects[0];
				if (clipRect.maxX <= clipRect.minX || clipRect.maxY <= clipRect.minY)
					continue;

#if 0
				Ref<ViewableResource> texture;
				texture.Attach(reinterpret_cast<ViewableResource*>(pCmd->GetTexID()));
#endif

				Ref<ViewableResource> texture = reinterpret_cast<ViewableResource*>(pCmd->GetTexID());

				nvrhi::DrawArguments drawArguments;
				drawArguments.vertexCount = pCmd->ElemCount;
				drawArguments.startIndexLocation = pCmd->IdxOffset + idxOffset;
				drawArguments.startVertexLocation = pCmd->VtxOffset + vtxOffset;

				ImGuiRenderer* instance = this;
				Renderer::Submit([instance, commandBuffer = m_CommandBuffer, swapchain = m_Swapchain, tex = texture, drawState, drawArguments, pushConstants]() mutable
				{
					SK_PROFILE_SCOPED("ImGui - Draw cmd");

					drawState.framebuffer = swapchain->GetCurrentFramebuffer();
					drawState.bindings = { instance->GetBindingSet(tex) };

					auto commandList = commandBuffer->GetHandle();
					commandList->setGraphicsState(drawState);
					commandList->setPushConstants(&pushConstants, sizeof(PushConstants));
					commandList->drawIndexed(drawArguments);
				});

#if 0
				if (dynamic_cast<ImGuiTexture*>(texture.Raw()))
					texture.Detach();
#endif

			}

			idxOffset += cmdList->IdxBuffer.Size;
			vtxOffset += cmdList->VtxBuffer.Size;
		}

		m_CommandBuffer->End();
		m_CommandBuffer->Execute();
		return true;
	}

	bool ImGuiRenderer::ReallocateBuffer(nvrhi::BufferHandle& buffer, uint64_t requiredSize, uint64_t reallocateSize, bool isIndexBuffer)
	{
		if (buffer == nullptr || buffer->getDesc().byteSize < requiredSize)
		{
			nvrhi::BufferDesc desc;
			desc.byteSize = reallocateSize;
			desc.structStride = 0;
			desc.debugName = isIndexBuffer ? "ImGui index buffer" : "ImGui vertex buffer";
			desc.canHaveUAVs = false;
			desc.isVertexBuffer = !isIndexBuffer;
			desc.isIndexBuffer = isIndexBuffer;
			desc.isDrawIndirectArgs = false;
			desc.isVolatile = false;
			desc.initialState = isIndexBuffer ? nvrhi::ResourceStates::IndexBuffer : nvrhi::ResourceStates::VertexBuffer;
			desc.keepInitialState = true;

			auto device = Application::Get().GetDeviceManager()->GetDevice();
			buffer = device->createBuffer(desc);

			if (!buffer)
			{
				return false;
			}
		}

		return true;
	}

	void ImGuiRenderer::UpdateTexture(Ref<RenderCommandBuffer> commandBuffer, ImTextureData* texture)
	{
		auto device = Application::Get().GetDeviceManager()->GetDevice();

		if (texture->Status == ImTextureStatus_WantCreate)
		{
			auto textureDesc = nvrhi::TextureDesc()
				.setWidth(texture->Width)
				.setHeight(texture->Height)
				.setFormat(nvrhi::Format::RGBA8_UNORM)
				.setDebugName(fmt::format("ImGui Texture {}", texture->UniqueID));

			auto textureHandle = device->createTexture(textureDesc);

			auto tex = Ref<ImGuiTexture>::Create();
			tex->View.Handle = textureHandle;
			tex->View.TextureSampler = m_FontSampler;

			texture->BackendUserData = tex.Detach();
			texture->SetTexID(reinterpret_cast<ImTextureID>(texture->BackendUserData));
			texture->SetStatus(ImTextureStatus_OK);
		}
		else if (texture->Status == ImTextureStatus_WantDestroy)
		{
			DestroyTexture(texture);
		}
		else if (texture->Status == ImTextureStatus_WantUpdates)
		{
			Ref<ImGuiTexture> tex = static_cast<ImGuiTexture*>(texture->BackendUserData);

			Renderer::Submit([commandBuffer, tex, temp = Buffer::Copy(texture->Pixels, texture->GetSizeInBytes()), pitch = texture->GetPitch()]() mutable
			{
				auto commandList = commandBuffer->GetHandle();
				const auto& viewInfo = tex->GetViewInfo();

				commandList->beginTrackingTextureState(viewInfo.Handle, nvrhi::AllSubresources, nvrhi::ResourceStates::Common);
				commandList->writeTexture(viewInfo.Handle, 0, 0, temp.As<const void>(), pitch);
				commandList->setPermanentTextureState(viewInfo.Handle, nvrhi::ResourceStates::ShaderResource);
				commandList->commitBarriers();

				temp.Release();
			});

			texture->SetStatus(ImTextureStatus_OK);
		}
	}

	void ImGuiRenderer::DestroyTexture(ImTextureData* texture)
	{
		Ref<ImGuiTexture> tex;
		tex.Attach(static_cast<ImGuiTexture*>(texture->BackendUserData));

		texture->SetTexID(ImTextureID_Invalid);
		texture->SetStatus(ImTextureStatus_Destroyed);
		texture->BackendUserData = nullptr;
	}

	nvrhi::IBindingSet* ImGuiRenderer::GetBindingSet(Ref<ViewableResource> viewable)
	{
		auto device = Application::Get().GetDeviceManager()->GetDevice();

		const auto iter = m_BindingsCache.find(viewable->GetViewInfo());
		if (iter != m_BindingsCache.end())
			iter->second;

		nvrhi::BindingSetDesc desc;
		const auto& viewInfo = viewable->GetViewInfo();

		desc.bindings = {
			nvrhi::BindingSetItem::PushConstants(0, sizeof(glm::vec2) * 2),
			nvrhi::BindingSetItem::Texture_SRV(0, viewInfo.Handle, viewInfo.Format, viewInfo.SubresourceSet, viewInfo.Dimension),
			nvrhi::BindingSetItem::Sampler(0, viewInfo.TextureSampler ? viewInfo.TextureSampler : m_FontSampler)
		};

		nvrhi::BindingSetHandle binding;
		binding = device->createBindingSet(desc, m_BindingLayout);
		assert(binding);

		m_BindingsCache[viewInfo] = binding;
		return binding;
	}

	bool ImGuiRenderer::UpdateGeometry(Ref<RenderCommandBuffer> commandBuffer, const ImDrawData* drawData)
	{
		// create/resize vertex and index buffers if needed
		if (!ReallocateBuffer(m_VertexBuffer,
							  drawData->TotalVtxCount * sizeof(ImDrawVert),
							  (drawData->TotalVtxCount + 5000) * sizeof(ImDrawVert),
							  false))
		{
			return false;
		}

		if (!ReallocateBuffer(m_IndexBuffer,
							  drawData->TotalIdxCount * sizeof(ImDrawIdx),
							  (drawData->TotalIdxCount + 5000) * sizeof(ImDrawIdx),
							  true))
		{
			return false;
		}

		uint32_t index = Renderer::GetCurrentFrameIndex() % 2;
		auto& vertexBufferData = m_VertexBufferData[index];
		auto& indexBufferData = m_IndexBufferData[index];

		vertexBufferData.resize(m_VertexBuffer->getDesc().byteSize / sizeof(ImDrawVert));
		indexBufferData.resize(m_IndexBuffer->getDesc().byteSize / sizeof(ImDrawIdx));

		// copy and convert all vertices into a single contiguous buffer
		ImDrawVert* vtxDst = &vertexBufferData[0];
		ImDrawIdx* idxDst = &indexBufferData[0];

		for (int n = 0; n < drawData->CmdListsCount; n++)
		{
			const ImDrawList* cmdList = drawData->CmdLists[n];

			memcpy(vtxDst, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
			memcpy(idxDst, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));

			vtxDst += cmdList->VtxBuffer.Size;
			idxDst += cmdList->IdxBuffer.Size;
		}

		ImGuiRenderer* instance = this;
		Renderer::Submit([instance, commandBuffer, vertexBuffer = m_VertexBuffer, indexBuffer = m_IndexBuffer]()
		{
			auto commandList = commandBuffer->GetHandle();

			uint32_t index = Renderer::RT_GetCurrentFrameIndex() % 2;
			auto& vertexBufferData = instance->m_VertexBufferData[index];
			auto& indexBufferData = instance->m_IndexBufferData[index];

			commandList->writeBuffer(vertexBuffer, &vertexBufferData[0], vertexBuffer->getDesc().byteSize);
			commandList->writeBuffer(indexBuffer, &indexBufferData[0], indexBuffer->getDesc().byteSize);
		});

		return true;
	}

}
