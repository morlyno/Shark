#include "skpch.h"
#include "ImGuiRenderer.h"

#include "Shark/Core/Application.h"
#include "Shark/Render/Renderer.h"
#include "Shark/Debug/Profiler.h"

#include <nvrhi/utils.h>

namespace Shark {

	bool ImGuiRenderer::Initialize()
	{
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

			m_BasePSODesc.primType = nvrhi::PrimitiveType::TriangleList;
			m_BasePSODesc.inputLayout = m_ShaderAttribLayout;
			m_BasePSODesc.VS = m_VertexShader;
			m_BasePSODesc.PS = m_PixelShader;
			m_BasePSODesc.renderState = renderState;
			m_BasePSODesc.bindingLayouts = { m_BindingLayout };
		}

		{
			const auto desc = nvrhi::SamplerDesc()
				.setAllAddressModes(nvrhi::SamplerAddressMode::Wrap)
				.setAllFilters(true);

			m_FontSampler = device->createSampler(desc);
		}

		return true;
	}

	bool ImGuiRenderer::Render(ImGuiViewport* viewport, nvrhi::GraphicsPipelineHandle pipeline, nvrhi::FramebufferHandle framebuffer)
	{
		SK_PROFILE_FUNCTION();
		SK_PERF_SCOPED("ImGui Render");

		ImDrawData* drawData = viewport->DrawData;
		const auto& io = ImGui::GetIO();

		m_CommandBuffer->RT_Begin();
		auto commandList = m_CommandBuffer->GetHandle();

		nvrhi::utils::ClearColorAttachment(commandList, framebuffer, 0, { 1.0f, 0.0f, 0.0f, 1.0f });

		if (drawData->Textures)
			for (ImTextureData* texture : *drawData->Textures)
				UpdateTexture(commandList, texture);

		if (!UpdateGeometry(commandList))
		{
			m_CommandBuffer->RT_End();
			return false;
		}

		// handle DPI scaling
		drawData->ScaleClipRects(io.DisplayFramebufferScale);

		struct PushConstants
		{
			glm::vec2 Translation;
			glm::vec2 Scale;
		} pushConstants;
		pushConstants.Scale.x = 2.0f / drawData->DisplaySize.x;
		pushConstants.Scale.y = 2.0f / drawData->DisplaySize.y;
		pushConstants.Translation.x = -1.0f - drawData->DisplayPos.x * pushConstants.Scale.x;
		pushConstants.Translation.y = -1.0f - drawData->DisplayPos.y * pushConstants.Scale.y;

		// set up graphics state
		nvrhi::GraphicsState drawState;

		drawState.framebuffer = framebuffer;
		assert(drawState.framebuffer);

		drawState.pipeline = pipeline;

		drawState.viewport.viewports.push_back(nvrhi::Viewport(io.DisplaySize.x * io.DisplayFramebufferScale.x,
															   io.DisplaySize.y * io.DisplayFramebufferScale.y));
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
		for (int n = 0; n < drawData->CmdListsCount; n++)
		{
			const ImDrawList* cmdList = drawData->CmdLists[n];
			for (int i = 0; i < cmdList->CmdBuffer.Size; i++)
			{
				const ImDrawCmd* pCmd = &cmdList->CmdBuffer[i];

				if (pCmd->UserCallback)
				{
					pCmd->UserCallback(cmdList, pCmd);
				}
				else {
					drawState.bindings = { GetBindingSet((const ViewInfo*)pCmd->GetTexID()) };
					assert(drawState.bindings[0]);

					drawState.viewport.scissorRects[0] = nvrhi::Rect(int(pCmd->ClipRect.x),
																	 int(pCmd->ClipRect.z),
																	 int(pCmd->ClipRect.y),
																	 int(pCmd->ClipRect.w));

					nvrhi::DrawArguments drawArguments;
					drawArguments.vertexCount = pCmd->ElemCount;
					drawArguments.startIndexLocation = idxOffset;
					drawArguments.startVertexLocation = vtxOffset;

					commandList->setGraphicsState(drawState);
					commandList->setPushConstants(&pushConstants, sizeof(PushConstants));
					commandList->drawIndexed(drawArguments);
				}

				idxOffset += pCmd->ElemCount;
			}

			vtxOffset += cmdList->VtxBuffer.Size;
		}

		m_CommandBuffer->RT_End();
		m_CommandBuffer->RT_Execute();

		return true;
	}

	bool ImGuiRenderer::RenderToSwapchain(ImGuiViewport* viewport, Ref<SwapChain> swapchain)
	{
		nvrhi::FramebufferHandle framebuffer = swapchain->GetCurrentFramebuffer();
		nvrhi::GraphicsPipelineHandle pipeline = m_PipelineCache[swapchain.Raw()];
		if (!pipeline)
		{
			auto device = Application::Get().GetDeviceManager()->GetDevice();
			pipeline = device->createGraphicsPipeline(m_BasePSODesc, framebuffer);
			m_PipelineCache[swapchain.Raw()] = pipeline;
		}

		return Render(viewport, pipeline, framebuffer);
	}

	void ImGuiRenderer::OnDestroySwapchain(Ref<SwapChain> swapchain)
	{
		m_PipelineCache.erase(swapchain.Raw());
	}

	bool ImGuiRenderer::ReallocateBuffer(nvrhi::BufferHandle& buffer, size_t requiredSize, size_t reallocateSize, bool isIndexBuffer)
	{
		if (buffer == nullptr || size_t(buffer->getDesc().byteSize) < requiredSize)
		{
			nvrhi::BufferDesc desc;
			desc.byteSize = uint32_t(reallocateSize);
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

	void ImGuiRenderer::UpdateTexture(nvrhi::CommandListHandle commandList, ImTextureData* texture)
	{
		auto device = Application::Get().GetDeviceManager()->GetDevice();

		if (texture->Status == ImTextureStatus_WantCreate)
		{
			auto textureDesc = nvrhi::TextureDesc()
				.setWidth(texture->Width)
				.setHeight(texture->Height)
				.setFormat(nvrhi::Format::RGBA8_UNORM);

			auto textureHandle = device->createTexture(textureDesc);

			auto viewInfo = sknew ViewInfo();
			viewInfo->ImageHandle = textureHandle;
			viewInfo->Sampler = m_FontSampler;

			texture->BackendUserData = viewInfo;
			texture->SetTexID((ImTextureID)texture->BackendUserData);
			texture->SetStatus(ImTextureStatus_OK);
		}
		else if (texture->Status == ImTextureStatus_WantDestroy)
		{
			auto viewInfo = (ViewInfo*)texture->BackendUserData;
			viewInfo->ImageHandle = nullptr;

			skdelete viewInfo;

			texture->SetTexID(ImTextureID_Invalid);
			texture->SetStatus(ImTextureStatus_Destroyed);
			texture->BackendUserData = nullptr;
		}
		else if (texture->Status == ImTextureStatus_WantUpdates)
		{
			auto viewInfo = (ViewInfo*)texture->BackendUserData;

			commandList->beginTrackingTextureState(viewInfo->ImageHandle, nvrhi::AllSubresources, nvrhi::ResourceStates::Common);
			commandList->writeTexture(viewInfo->ImageHandle, 0, 0, texture->Pixels, texture->Width * 4);
			commandList->setPermanentTextureState(viewInfo->ImageHandle, nvrhi::ResourceStates::ShaderResource);
			commandList->commitBarriers();

			texture->SetStatus(ImTextureStatus_OK);
		}
	}

	nvrhi::IBindingSet* ImGuiRenderer::GetBindingSet(const ViewInfo* viewInfo)
	{
		auto device = Application::Get().GetDeviceManager()->GetDevice();

		auto iter = m_BindingsCache.find(viewInfo);
		if (iter != m_BindingsCache.end())
		{
			auto activeSampler = viewInfo->Sampler ? viewInfo->Sampler : m_FontSampler;

			nvrhi::IBindingSet* bindingSet = iter->second;
			if (bindingSet->getDesc()->bindings[1].resourceHandle == viewInfo->ImageHandle &&
				bindingSet->getDesc()->bindings[2].resourceHandle == activeSampler)
				return iter->second;

			m_BindingsCache.erase(viewInfo);
		}

		nvrhi::BindingSetDesc desc;

		desc.bindings = {
			nvrhi::BindingSetItem::PushConstants(0, sizeof(glm::vec2) * 2),
			nvrhi::BindingSetItem::Texture_SRV(0, viewInfo->ImageHandle, nvrhi::Format::UNKNOWN, viewInfo->SubresourceSet),
			nvrhi::BindingSetItem::Sampler(0, viewInfo->Sampler ? viewInfo->Sampler : m_FontSampler)
		};

		nvrhi::BindingSetHandle binding;
		binding = device->createBindingSet(desc, m_BindingLayout);
		assert(binding);

		m_BindingsCache[viewInfo] = binding;
		return binding;
	}

	bool ImGuiRenderer::UpdateGeometry(nvrhi::ICommandList* commandList)
	{
		ImDrawData* drawData = ImGui::GetDrawData();

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

		m_VertexBufferData.resize(m_VertexBuffer->getDesc().byteSize / sizeof(ImDrawVert));
		m_IndexBufferData.resize(m_IndexBuffer->getDesc().byteSize / sizeof(ImDrawIdx));

		// copy and convert all vertices into a single contiguous buffer
		ImDrawVert* vtxDst = &m_VertexBufferData[0];
		ImDrawIdx* idxDst = &m_IndexBufferData[0];

		for (int n = 0; n < drawData->CmdListsCount; n++)
		{
			const ImDrawList* cmdList = drawData->CmdLists[n];

			memcpy(vtxDst, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
			memcpy(idxDst, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));

			vtxDst += cmdList->VtxBuffer.Size;
			idxDst += cmdList->IdxBuffer.Size;
		}

		commandList->writeBuffer(m_VertexBuffer, &m_VertexBufferData[0], m_VertexBuffer->getDesc().byteSize);
		commandList->writeBuffer(m_IndexBuffer, &m_IndexBufferData[0], m_IndexBuffer->getDesc().byteSize);

		return true;
	}

}
