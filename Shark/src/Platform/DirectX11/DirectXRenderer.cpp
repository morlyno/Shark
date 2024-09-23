#include "skpch.h"
#undef GetMessage
#include "DirectXRenderer.h"

#include "Shark/Core/Application.h"
#include "Shark/Core/Timer.h"
#include "Shark/Render/Renderer.h"

#include "Shark/Utils/String.h"
#include "Shark/Debug/Profiler.h"

#include "Platform/DirectX11/DirectXAPI.h"
#include "Platform/DirectX11/DirectXContext.h"
#include "Platform/DirectX11/DirectXRenderCommandBuffer.h"
#include "Platform/DirectX11/DirectXFrameBuffer.h"
#include "Platform/DirectX11/DirectXShader.h"
#include "Platform/DirectX11/DirectXConstantBuffer.h"
#include "Platform/DirectX11/DirectXStorageBuffer.h"
#include "Platform/DirectX11/DirectXTexture.h"
#include "Platform/DirectX11/DirectXRenderPass.h"
#include "Platform/DirectX11/DirectXImGuiLayer.h"
#include "Platform/Windows/WindowsUtils.h"

#include <backends/imgui_impl_dx11.h>
#include <dxgi1_3.h>
#include <dxgidebug.h>

#if SK_ENABLE_GPU_VALIDATION && false
#define DX11_VALIDATE_CONTEXT(ctx) DirectXRenderer::Get()->GetDebug()->ValidateContext(ctx);
#else
#define DX11_VALIDATE_CONTEXT(ctx)
#endif

namespace Shark {

	namespace utils {

		static void BindShaderResourceView(ID3D11DeviceContext* context, ID3D11ShaderResourceView* view, ShaderReflection::ShaderStage stage, uint32_t binding)
		{
			switch (stage)
			{
				case ShaderReflection::ShaderStage::Vertex: context->VSSetShaderResources(binding, 1, &view); break;
				case ShaderReflection::ShaderStage::Pixel: context->PSSetShaderResources(binding, 1, &view); break;
				case ShaderReflection::ShaderStage::Compute: context->CSSetShaderResources(binding, 1, &view); break;
				default: SK_CORE_ASSERT(false, "Unkown shader stage");
			}
		}

		static void BindSamplerStage(ID3D11DeviceContext* context, ID3D11SamplerState* samplerState, ShaderReflection::ShaderStage stage, uint32_t binding)
		{
			switch (stage)
			{
				case ShaderReflection::ShaderStage::Vertex: context->VSSetSamplers(binding, 1, &samplerState); break;
				case ShaderReflection::ShaderStage::Pixel: context->PSSetSamplers(binding, 1, &samplerState); break;
				case ShaderReflection::ShaderStage::Compute: context->CSSetSamplers(binding, 1, &samplerState); break;
				default: SK_CORE_ASSERT(false, "Unkown shader stage");
			}
		}
		
		static void BindConstantBuffer(ID3D11DeviceContext* context, ID3D11Buffer* constantBuffer, ShaderReflection::ShaderStage stage, uint32_t binding)
		{
			switch (stage)
			{
				case ShaderReflection::ShaderStage::Vertex: context->VSSetConstantBuffers(binding, 1, &constantBuffer); break;
				case ShaderReflection::ShaderStage::Pixel: context->PSSetConstantBuffers(binding, 1, &constantBuffer); break;
				case ShaderReflection::ShaderStage::Compute: context->CSSetConstantBuffers(binding, 1, &constantBuffer); break;
				default: SK_CORE_ASSERT(false, "Unkown shader stage");
			}
		}

	}

	DirectXRenderer::DirectXRenderer()
	{
	}

	DirectXRenderer::~DirectXRenderer()
	{
	}

	void DirectXRenderer::Init()
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_INFO_TAG("Renderer", "Initializing DirectX Renderer");

		QueryCapabilities();

		auto device = DirectXContext::GetCurrentDevice();
		auto dxDevice = device->GetDirectXDevice();

		D3D11_QUERY_DESC queryDesc;
		queryDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
		queryDesc.MiscFlags = 0;
		dxDevice->CreateQuery(&queryDesc, &m_FrequencyQuery);

		D3D11_SAMPLER_DESC samplerDesc = CD3D11_SAMPLER_DESC(D3D11_DEFAULT);
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		DirectXAPI::CreateSamplerState(dxDevice, samplerDesc, m_ClampLinearSampler);

		m_QuadVertexLayout = {
			{ VertexDataType::Float2, "Position" }
		};

		float vertices[4 * 2] = {
			-1.0f,  1.0f,
			 1.0f,  1.0f,
			 1.0f, -1.0f,
			-1.0f, -1.0f
		};

		uint32_t indices[3 * 2] = {
			0, 1, 2,
			2, 3, 0
		};

		m_QuadVertexBuffer = Ref<DirectXVertexBuffer>::Create((uint32_t)sizeof(vertices), false, Buffer::FromArray(vertices));
		m_QuadIndexBuffer = Ref<DirectXIndexBuffer>::Create((uint32_t)std::size(indices), false, Buffer::FromArray(indices));

		{
			glm::vec3 vertices[] = {
				{ -1, -1, -1 },
				{ 1, -1, -1 },
				{ -1, 1, -1 },
				{ 1, 1, -1 },
				{ -1, -1, 1 },
				{ 1, -1, 1 },
				{ -1, 1, 1 },
				{ 1, 1, 1 },
			};

			uint32_t indices[] = {
				0, 2, 1, 2, 3, 1,
				1, 3, 5, 3, 7, 5,
				2, 6, 3, 3, 6, 7,
				4, 5, 7, 4, 7, 6,
				0, 4, 2, 2, 4, 6,
				0, 1, 4, 1, 5, 4
			};

			m_CubeVertexBuffer = VertexBuffer::Create(Buffer::FromArray(vertices)).As<DirectXVertexBuffer>();
			m_CubeIndexBuffer = IndexBuffer::Create(Buffer::FromArray(indices)).As<DirectXIndexBuffer>();
		}

		m_CommandBuffer = RenderCommandBuffer::Create();

		Renderer::Submit([instance = Ref(this)]() { SK_CORE_INFO_TAG("Renderer", "GPU Resources Created"); instance->m_ResourceCreated = true; });
	}

	void DirectXRenderer::ShutDown()
	{
		SK_PROFILE_FUNCTION();

		m_QuadVertexBuffer = nullptr;
		m_QuadIndexBuffer = nullptr;
		m_CubeVertexBuffer = nullptr;
		m_CubeIndexBuffer = nullptr;
		m_CommandBuffer = nullptr;

		m_FrequencyQuery->Release();
		m_ClampLinearSampler->Release();

		m_FrequencyQuery = nullptr;
		m_ClampLinearSampler = nullptr;
		m_ResourceCreated = false;
		SK_CORE_WARN_TAG("Renderer", "DirectXRenderer destroyed");
	}

	void DirectXRenderer::BeginEventMarker(Ref<RenderCommandBuffer> commandBuffer, const std::string& name)
	{
		Renderer::Submit([commandBuffer, name]()
		{
			ID3DUserDefinedAnnotation* annotation = commandBuffer.As<DirectXRenderCommandBuffer>()->GetAnnotation();
			std::wstring namew = String::ToWide(name);
			annotation->BeginEvent(namew.c_str());
		});
	}

	void DirectXRenderer::EndEventMarker(Ref<RenderCommandBuffer> commandBuffer)
	{
		Renderer::Submit([commandBuffer]()
		{
			ID3DUserDefinedAnnotation* annotation = commandBuffer.As<DirectXRenderCommandBuffer>()->GetAnnotation();
			annotation->EndEvent();
		});
	}

	void DirectXRenderer::BeginFrame()
	{
		Ref<DirectXRenderer> instance = this;
		Renderer::Submit([instance]() { instance->m_RTFrameIndex++; });
		m_FrameIndex++;

		m_CommandBuffer->Begin();
		m_GPUTimeQuery = m_CommandBuffer->BeginTimestampQuery();

		Renderer::Submit([instance]()
		{
			instance->RT_BeginFrequencyQuery();
		});
	}

	void DirectXRenderer::EndFrame()
	{
		Ref<DirectXRenderer> instance = this;
		Renderer::Submit([instance]()
		{
			instance->RT_EndFrequencyQuery();
		});

		m_CommandBuffer->EndTimestampQuery(m_GPUTimeQuery);
		m_CommandBuffer->End();
		m_CommandBuffer->Execute();
		m_GPUTime = m_CommandBuffer->GetTime(m_GPUTimeQuery);

		Renderer::Submit([]() { DirectXContext::Get()->FlushMessages(); });
	}

	void DirectXRenderer::BeginRenderPass(Ref<RenderCommandBuffer> commandBuffer, Ref<RenderPass> renderPass, bool expliciteClear)
	{
		if (expliciteClear)
		{
			renderPass->GetSpecification().Pipeline->GetSpecification().TargetFrameBuffer->Clear(commandBuffer);
		}

		Ref<DirectXRenderer> instance = this;
		Renderer::Submit([instance, dxCommandBuffer = commandBuffer.As<DirectXRenderCommandBuffer>(), dxRenderPass = renderPass.As<DirectXRenderPass>(), expliciteClear]()
		{
			Ref<DirectXShader> shader = dxRenderPass->m_Specification.Pipeline->GetSpecification().Shader.As<DirectXShader>();
			Ref<DirectXPipeline> pipeline = dxRenderPass->m_Specification.Pipeline.As<DirectXPipeline>();
			pipeline->BeginRenderPass();

			ID3D11DeviceContext* context = dxCommandBuffer->GetContext();
			ID3D11RenderTargetView* nullViews[8]{};
			context->OMSetRenderTargets(8, nullViews, nullptr);

			instance->RT_BindResources(dxCommandBuffer, shader, dxRenderPass->GetBoundResources());
		});
	}

	void DirectXRenderer::EndRenderPass(Ref<RenderCommandBuffer> commandBuffer, Ref<RenderPass> renderPass)
	{
		Renderer::Submit([dxCommandBuffer = commandBuffer.As<DirectXRenderCommandBuffer>(), dxRenderPass = renderPass.As<DirectXRenderPass>()]()
		{
			Ref<DirectXPipeline> pipeline = dxRenderPass->m_Specification.Pipeline.As<DirectXPipeline>();
			pipeline->EndRenderPass();
		});
	}

	void DirectXRenderer::RenderFullScreenQuad(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Material> material)
	{
		SK_PROFILE_FUNCTION();

		Ref<DirectXRenderCommandBuffer> dxCommandBuffer = commandBuffer.As<DirectXRenderCommandBuffer>();
		Ref<DirectXPipeline> dxPipeline = pipeline.As<DirectXPipeline>();
		Ref<DirectXMaterial> dxMaterial = material.As<DirectXMaterial>();
		Ref<DirectXRenderer> instance = this;
		Renderer::Submit([instance, dxCommandBuffer, dxPipeline, frambuffer = dxPipeline->m_FrameBuffer, dxMaterial, vertexBuffer = m_QuadVertexBuffer, indexBuffer = m_QuadIndexBuffer]()
		{
			SK_PROFILE_SCOPED("DirectXRenderer::RenderFullScreenQuad");

			ID3D11DeviceContext* ctx = dxCommandBuffer->GetContext();

			std::array<ID3D11RenderTargetView*, 8> nullRTVs;
			nullRTVs.fill(nullptr);
			ctx->OMSetRenderTargets(8, nullRTVs.data(), nullptr);

			Ref<DirectXShader> dxShader = dxPipeline->m_Shader;
			ctx->VSSetShader(dxShader->m_VertexShader, nullptr, 0);
			ctx->PSSetShader(dxShader->m_PixelShader, nullptr, 0);

			const UINT offset = 0;
			const UINT stride = dxPipeline->GetSpecification().Layout.GetVertexSize();
			ctx->IASetVertexBuffers(0, 1, &vertexBuffer->m_VertexBuffer, &stride, &offset);
			ctx->IASetIndexBuffer(indexBuffer->m_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
			ctx->IASetInputLayout(dxPipeline->m_InputLayout);

			ctx->RSSetState(dxPipeline->m_RasterizerState);
			ctx->OMSetDepthStencilState(dxPipeline->m_DepthStencilState, dxPipeline->m_Specification.StencilRef);

			instance->RT_PrepareAndBindMaterial(dxCommandBuffer, dxMaterial);
			instance->RT_BindPushConstants(dxCommandBuffer, dxPipeline);

			Ref<DirectXFrameBuffer> dxFrameBuffer = frambuffer;
			ctx->OMSetRenderTargets(dxFrameBuffer->m_Count, dxFrameBuffer->m_FrameBuffers.data(), dxFrameBuffer->m_DepthStencil);
			ctx->OMSetBlendState(dxFrameBuffer->m_BlendState, nullptr, 0xFFFFFFFF);
			ctx->RSSetViewports(1, &dxFrameBuffer->m_Viewport);

			
			ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			DX11_VALIDATE_CONTEXT(ctx);
			ctx->DrawIndexed(6, 0, 0);
		});
	}

	void DirectXRenderer::BeginBatch(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer)
	{
		SK_PROFILE_FUNCTION();

		Ref<DirectXRenderCommandBuffer> commandBuffer = renderCommandBuffer.As<DirectXRenderCommandBuffer>();
		Ref<DirectXVertexBuffer> dxVB = vertexBuffer.As<DirectXVertexBuffer>();
		Ref<DirectXIndexBuffer> dxIB = indexBuffer.As<DirectXIndexBuffer>();
		Ref<DirectXPipeline> dxPipeline = pipeline.As<DirectXPipeline>();
		Ref<DirectXRenderer> instance = this;

		Renderer::Submit([instance, commandBuffer, dxVB, dxIB, dxPipeline]()
		{
			SK_PROFILE_SCOPED("DirectXRenderer::BeginBatch");

			ID3D11DeviceContext* ctx = commandBuffer->GetContext();

			const UINT offset = 0;
			const UINT stride = dxPipeline->GetSpecification().Layout.GetVertexSize();
			ctx->IASetVertexBuffers(0, 1, &dxVB->m_VertexBuffer, &stride, &offset);

			ctx->IASetIndexBuffer(dxIB->m_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);


			Ref<DirectXShader> dxShader = dxPipeline->m_Shader;

			ctx->VSSetShader(dxShader->m_VertexShader, nullptr, 0);
			ctx->PSSetShader(dxShader->m_PixelShader, nullptr, 0);

			ctx->IASetInputLayout(dxPipeline->m_InputLayout);

			Ref<DirectXFrameBuffer> dxFrameBuffer = dxPipeline->m_FrameBuffer;

			ctx->OMSetRenderTargets(dxFrameBuffer->m_Count, dxFrameBuffer->m_FrameBuffers.data(), dxFrameBuffer->m_DepthStencil);
			ctx->RSSetViewports(1, &dxFrameBuffer->m_Viewport);

			ctx->RSSetState(dxPipeline->m_RasterizerState);
			ctx->OMSetDepthStencilState(dxPipeline->m_DepthStencilState, dxPipeline->m_Specification.StencilRef);
			ctx->OMSetBlendState(dxFrameBuffer->m_BlendState, nullptr, 0xFFFFFFFF);

			ctx->IASetPrimitiveTopology(dxPipeline->m_PrimitveTopology);
		});
	}

	void DirectXRenderer::RenderBatch(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Material> material, uint32_t indexCount, uint32_t startIndex)
	{
		Ref<DirectXRenderCommandBuffer> commandBuffer = renderCommandBuffer.As<DirectXRenderCommandBuffer>();
		Ref<DirectXMaterial> dxMaterial = material.As<DirectXMaterial>();
		Ref<DirectXRenderer> instance = this;
		Renderer::Submit([instance, commandBuffer, dxMaterial, indexCount, startIndex]()
		{
			SK_PROFILE_SCOPED("DirectXRenderer::RenderBatch");

			ID3D11DeviceContext* ctx = commandBuffer->GetContext();
			instance->RT_PrepareAndBindMaterial(commandBuffer, dxMaterial);
			DX11_VALIDATE_CONTEXT(ctx);
			ctx->DrawIndexed(indexCount, startIndex, 0);
		});
	}

	void DirectXRenderer::EndBatch(Ref<RenderCommandBuffer> renderCommandBuffer)
	{
	}

	void DirectXRenderer::RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, uint32_t indexCount)
	{
		SK_PROFILE_FUNCTION();

		Ref<DirectXRenderCommandBuffer> commandBuffer = renderCommandBuffer.As<DirectXRenderCommandBuffer>();
		Ref<DirectXVertexBuffer> dxVB = vertexBuffer.As<DirectXVertexBuffer>();
		Ref<DirectXIndexBuffer> dxIB = indexBuffer.As<DirectXIndexBuffer>();
		Ref<DirectXPipeline> dxPipeline = pipeline.As<DirectXPipeline>();
		Ref<DirectXMaterial> dxMaterial = material.As<DirectXMaterial>();
		Ref<DirectXRenderer> instance = this;
		Renderer::Submit([instance, commandBuffer, dxVB, dxIB, dxPipeline, dxMaterial, indexCount]()
		{
			SK_PROFILE_SCOPED("DirectXRenderer::RenderGeometry");

			ID3D11DeviceContext* ctx = commandBuffer->GetContext();

			const UINT offset = 0;
			const UINT stride = dxPipeline->GetSpecification().Layout.GetVertexSize();
			ctx->IASetVertexBuffers(0, 1, &dxVB->m_VertexBuffer, &stride, &offset);

			ctx->IASetIndexBuffer(dxIB->m_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);


			Ref<DirectXShader> dxShader = dxPipeline->m_Shader;

			ctx->VSSetShader(dxShader->m_VertexShader, nullptr, 0);
			ctx->PSSetShader(dxShader->m_PixelShader, nullptr, 0);

			ctx->IASetInputLayout(dxPipeline->m_InputLayout);

			instance->RT_PrepareAndBindMaterial(commandBuffer, dxMaterial);

			if (dxPipeline->UsesPushConstant())
			{
				Ref<DirectXConstantBuffer> pushConstantBuffer = dxPipeline->GetLastUpdatedPushConstantBuffer().As<DirectXConstantBuffer>();
				const auto& reflectionData = dxShader->GetReflectionData();
				utils::BindConstantBuffer(ctx, pushConstantBuffer->m_ConstantBuffer, reflectionData.PushConstant.Stage, reflectionData.PushConstant.DXBinding);
			}

			Ref<DirectXFrameBuffer> dxFrameBuffer = dxPipeline->m_FrameBuffer;

			ctx->OMSetRenderTargets(dxFrameBuffer->m_Count, dxFrameBuffer->m_FrameBuffers.data(), dxFrameBuffer->m_DepthStencil);
			ctx->RSSetViewports(1, &dxFrameBuffer->m_Viewport);

			ctx->RSSetState(dxPipeline->m_RasterizerState);
			ctx->OMSetDepthStencilState(dxPipeline->m_DepthStencilState, dxPipeline->m_Specification.StencilRef);
			ctx->OMSetBlendState(dxFrameBuffer->m_BlendState, nullptr, 0xFFFFFFFF);

			ctx->IASetPrimitiveTopology(dxPipeline->m_PrimitveTopology);

			DX11_VALIDATE_CONTEXT(ctx);
			ctx->DrawIndexed(indexCount, 0, 0);
		});
	}

	void DirectXRenderer::RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, Ref<VertexBuffer> vertexBuffer, uint32_t vertexCount)
	{
		SK_PROFILE_FUNCTION();

		Ref<DirectXRenderCommandBuffer> commandBuffer = renderCommandBuffer.As<DirectXRenderCommandBuffer>();
		Ref<DirectXVertexBuffer> dxVB = vertexBuffer.As<DirectXVertexBuffer>();
		Ref<DirectXPipeline> dxPipeline = pipeline.As<DirectXPipeline>();
		Ref<DirectXMaterial> dxMaterial = material.As<DirectXMaterial>();
		Ref<DirectXRenderer> instance = this;
		Renderer::Submit([instance, commandBuffer, dxVB, dxPipeline, dxMaterial, vertexCount]()
		{
			SK_PROFILE_SCOPED("DirectXRenderer::RenderGeometry");

			ID3D11DeviceContext* ctx = commandBuffer->GetContext();

			const UINT offset = 0;
			const UINT stride = dxPipeline->GetSpecification().Layout.GetVertexSize();
			ctx->IASetVertexBuffers(0, 1, &dxVB->m_VertexBuffer, &stride, &offset);

			Ref<DirectXShader> dxShader = dxPipeline->m_Shader;

			ctx->VSSetShader(dxShader->m_VertexShader, nullptr, 0);
			ctx->PSSetShader(dxShader->m_PixelShader, nullptr, 0);

			ctx->IASetInputLayout(dxPipeline->m_InputLayout);

			instance->RT_PrepareAndBindMaterial(commandBuffer, dxMaterial);

			if (dxPipeline->UsesPushConstant())
			{
				Ref<DirectXConstantBuffer> pushConstantBuffer = dxPipeline->GetLastUpdatedPushConstantBuffer().As<DirectXConstantBuffer>();
				const auto& reflectionData = dxShader->GetReflectionData();
				utils::BindConstantBuffer(ctx, pushConstantBuffer->m_ConstantBuffer, reflectionData.PushConstant.Stage, reflectionData.PushConstant.DXBinding);
			}

			Ref<DirectXFrameBuffer> dxFrameBuffer = dxPipeline->m_FrameBuffer;

			ctx->OMSetRenderTargets(dxFrameBuffer->m_Count, dxFrameBuffer->m_FrameBuffers.data(), dxFrameBuffer->m_DepthStencil);
			ctx->RSSetViewports(1, &dxFrameBuffer->m_Viewport);

			ctx->RSSetState(dxPipeline->m_RasterizerState);
			ctx->OMSetDepthStencilState(dxPipeline->m_DepthStencilState, dxPipeline->m_Specification.StencilRef);
			ctx->OMSetBlendState(dxFrameBuffer->m_BlendState, nullptr, 0xFFFFFFFF);

			ctx->IASetPrimitiveTopology(dxPipeline->m_PrimitveTopology);

			DX11_VALIDATE_CONTEXT(ctx);
			ctx->Draw(vertexCount, 0);
		});
	}

	void DirectXRenderer::RenderCube(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Material> material)
	{
		RenderGeometry(commandBuffer, pipeline, material, m_CubeVertexBuffer, m_CubeIndexBuffer, m_CubeIndexBuffer->GetCount());
	}

	void DirectXRenderer::RenderSubmesh(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Mesh> mesh, uint32_t submeshIndex, Ref<MaterialTable> materialTable)
	{
		SK_CORE_VERIFY(false);
	}

	void DirectXRenderer::RenderSubmeshWithMaterial(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Mesh> mesh, Ref<MeshSource> meshSource, uint32_t submeshIndex, Ref<Material> material)
	{
		SK_CORE_VERIFY(mesh);
		//SK_CORE_VERIFY(material);

		Ref instance = this;
		auto dxCommandBuffer = commandBuffer.As<DirectXRenderCommandBuffer>();
		auto dxPipeline = pipeline.As<DirectXPipeline>();
		auto dxMaterial = material.As<DirectXMaterial>();
		Renderer::Submit([instance, dxCommandBuffer, dxPipeline, mesh, meshSource, submeshIndex, dxMaterial]()
		{
			SK_PROFILE_SCOPED("DirectXRenderer::RenderMesh");

			ID3D11DeviceContext* ctx = dxCommandBuffer->GetContext();

			auto vertexBuffer = meshSource->GetVertexBuffer().As<DirectXVertexBuffer>();
			auto indexBuffer = meshSource->GetIndexBuffer().As<DirectXIndexBuffer>();

			const UINT offset = 0;
			const UINT stride = dxPipeline->GetSpecification().Layout.GetVertexSize();
			ctx->IASetVertexBuffers(0, 1, &vertexBuffer->m_VertexBuffer, &stride, &offset);
			ctx->IASetIndexBuffer(indexBuffer->m_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);


			Ref<DirectXShader> dxShader = dxPipeline->m_Shader;
			ctx->VSSetShader(dxShader->m_VertexShader, nullptr, 0);
			ctx->PSSetShader(dxShader->m_PixelShader, nullptr, 0);

			ctx->IASetInputLayout(dxPipeline->m_InputLayout);

			const auto& submeshes = meshSource->GetSubmeshes();
			const auto& submesh = submeshes[submeshIndex];

			instance->RT_PrepareAndBindMaterial(dxCommandBuffer, dxMaterial);

			if (dxPipeline->UsesPushConstant())
			{
				Ref<DirectXConstantBuffer> pushConstantBuffer = dxPipeline->GetLastUpdatedPushConstantBuffer().As<DirectXConstantBuffer>();
				const auto& reflectionData = dxShader->GetReflectionData();
				utils::BindConstantBuffer(ctx, pushConstantBuffer->m_ConstantBuffer, reflectionData.PushConstant.Stage, reflectionData.PushConstant.DXBinding);
			}

			Ref<DirectXFrameBuffer> dxFrameBuffer = dxPipeline->m_FrameBuffer;

			ctx->OMSetRenderTargets(dxFrameBuffer->m_Count, dxFrameBuffer->m_FrameBuffers.data(), dxFrameBuffer->m_DepthStencil);
			ctx->RSSetViewports(1, &dxFrameBuffer->m_Viewport);

			ctx->RSSetState(dxPipeline->m_RasterizerState);
			ctx->OMSetDepthStencilState(dxPipeline->m_DepthStencilState, dxPipeline->m_Specification.StencilRef);
			ctx->OMSetBlendState(dxFrameBuffer->m_BlendState, nullptr, 0xFFFFFFFF);

			ctx->IASetPrimitiveTopology(dxPipeline->m_PrimitveTopology);

			DX11_VALIDATE_CONTEXT(ctx);
			ctx->DrawIndexed(submesh.IndexCount, submesh.BaseIndex, submesh.BaseVertex);
		});
	}

	void DirectXRenderer::CopyImage(Ref<Image2D> sourceImage, Ref<Image2D> destinationImage)
	{
		SK_CORE_VERIFY(sourceImage);
		SK_CORE_VERIFY(destinationImage);

		Renderer::Submit([dxSourceImage = sourceImage.As<DirectXImage2D>(), dxDestImage = destinationImage.As<DirectXImage2D>()]()
		{
			const DirectXImageInfo& sourceInfo = dxSourceImage->GetDirectXImageInfo();
			const DirectXImageInfo& destinationInfo = dxDestImage->GetDirectXImageInfo();

			auto device = DirectXContext::GetCurrentDevice();
			auto cmd = device->AllocateCommandBuffer();
			cmd->CopyResource(destinationInfo.Resource, sourceInfo.Resource);
			device->FlushCommandBuffer(cmd);
		});
	}

	void DirectXRenderer::CopyImage(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> sourceImage, Ref<Image2D> destinationImage)
	{
		SK_CORE_VERIFY(sourceImage);
		SK_CORE_VERIFY(destinationImage);

		Renderer::Submit([dxCommandBuffer = commandBuffer.As<DirectXRenderCommandBuffer>(), dxSourceImage = sourceImage.As<DirectXImage2D>(), dxDestImage = destinationImage.As<DirectXImage2D>()]()
		{
			ID3D11DeviceContext* context = dxCommandBuffer->GetContext();

			const DirectXImageInfo& sourceInfo = dxSourceImage->GetDirectXImageInfo();
			const DirectXImageInfo& destinationInfo = dxDestImage->GetDirectXImageInfo();

			context->CopyResource(destinationInfo.Resource, sourceInfo.Resource);
		});
	}

	void DirectXRenderer::BlitImage(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> sourceImage, Ref<Image2D> destinationImage)
	{
		FrameBufferSpecification framebufferSpecification;
		framebufferSpecification.Width = destinationImage->GetWidth();
		framebufferSpecification.Height = destinationImage->GetHeight();
		framebufferSpecification.Atachments = { destinationImage->GetSpecification().Format };
		framebufferSpecification.DebugName = "BlitImage";

		PipelineSpecification pipelineSpecification;
		pipelineSpecification.BackFaceCulling = true;
		pipelineSpecification.DepthEnabled = false;
		pipelineSpecification.TargetFrameBuffer = FrameBuffer::Create(framebufferSpecification);
		pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("BlitImage");
		pipelineSpecification.Layout = m_QuadVertexLayout;
		pipelineSpecification.DebugName = framebufferSpecification.DebugName;

		RenderPassSpecification renderPassSpecification;
		renderPassSpecification.Pipeline = Pipeline::Create(pipelineSpecification);
		renderPassSpecification.DebugName = pipelineSpecification.DebugName;
		Ref<RenderPass> renderPass = RenderPass::Create(renderPassSpecification);
		renderPass->Set("u_SourceImage", sourceImage);
		SK_CORE_VERIFY(renderPass->Validate());
		renderPass->Bake();

		BeginRenderPass(commandBuffer, renderPass, false);
		RenderFullScreenQuad(commandBuffer, renderPass->GetPipeline(), nullptr);
		EndRenderPass(commandBuffer, renderPass);

		CopyImage(commandBuffer, renderPass->GetOutput(0), destinationImage);
	}

	void DirectXRenderer::RT_EquirectangularToCubemap(Ref<DirectXTexture2D> equirect, Ref<DirectXTextureCube> unfiltered, uint32_t cubemapSize)
	{
		SK_PROFILE_SCOPED("Equirectangular Conversion");
		auto device = DirectXContext::GetCurrentDevice();
		auto* cmd = device->AllocateCommandBuffer();

		Ref<DirectXShader> equirectangularConversionShader = Renderer::GetShaderLibrary()->Get("EquirectangularToCubeMap").As<DirectXShader>();

		const auto& equirectImageInfo = equirect->GetDirectXImageInfo();
		const auto& equirectTexInfo = equirectangularConversionShader->GetResourceInfo("u_EquirectangularTex");
		cmd->CSSetShaderResources(equirectTexInfo.DXBinding, 1, &equirectImageInfo.View);
		cmd->CSSetSamplers(equirectTexInfo.DXSamplerBinding, 1, &equirectImageInfo.Sampler);

		unfiltered->GetImage().As<DirectXImage2D>()->RT_CreateUnorderAccessView(0);
		const auto& cubeMapInfo = equirectangularConversionShader->GetResourceInfo("o_CubeMap");
		cmd->CSSetUnorderedAccessViews(cubeMapInfo.DXBinding, 1, &unfiltered->GetUAV(0), nullptr);

		cmd->CSSetShader(equirectangularConversionShader->m_ComputeShader, nullptr, 0);
		cmd->Dispatch(cubemapSize / 32, cubemapSize / 32, 6);

		ID3D11UnorderedAccessView* nullUAV = nullptr;
		cmd->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr);
		device->FlushCommandBuffer(cmd);

		Renderer::RT_GenerateMips(unfiltered->GetImage());
	}

	void DirectXRenderer::RT_EnvironmentMipFilter(Ref<DirectXTextureCube> unfiltered, Ref<DirectXTextureCube> filtered, uint32_t cubemapSize)
	{
		SK_PROFILE_SCOPED("Environment Mip Filter");
		auto device = DirectXContext::GetCurrentDevice();
		auto cmd = device->AllocateCommandBuffer();
		const uint32_t mipCount = ImageUtils::CalcMipLevels(cubemapSize, cubemapSize);

		Ref<DirectXShader> environmentMipFilterShader = Renderer::GetShaderLibrary()->Get("EnvironmentMipFilter").As<DirectXShader>();
		filtered->GetImage().As<DirectXImage2D>()->RT_CreatePerMipUAV();

		const auto& bufferInfo = environmentMipFilterShader->GetPushConstantInfo();
		Ref<DirectXConstantBuffer> constantBuffer = Ref<DirectXConstantBuffer>::Create(bufferInfo.StructSize);

		const float deltaRoughness = 1.0f / glm::max((float)filtered->GetMipLevelCount() - 1.0f, 1.0f);
		for (uint32_t i = 0, size = cubemapSize; i < mipCount; i++, size /= 2)
		{
			Ref<DirectXImageView> envUnfilterImageView = Ref<DirectXImageView>::Create(unfiltered->GetImage(), i);
			const auto& unfilteredInfo = envUnfilterImageView->GetDirectXViewInfo();

			uint32_t numGroup = glm::max(1u, size / 32);
			float roughness = glm::max(i * deltaRoughness, 0.05f);

			const auto& inputTexInfo = environmentMipFilterShader->GetResourceInfo("inputTexture");
			cmd->CSSetShaderResources(inputTexInfo.DXBinding, 1, &unfilteredInfo.View);
			cmd->CSSetSamplers(inputTexInfo.DXSamplerBinding, 1, &unfilteredInfo.Sampler);

			const auto& outputTexInfo = environmentMipFilterShader->GetResourceInfo("outputTexture");
			cmd->CSSetUnorderedAccessViews(outputTexInfo.DXBinding, 1, &filtered->GetUAV(i), nullptr);

			cmd->CSSetShader(environmentMipFilterShader->m_ComputeShader, nullptr, 0);
			constantBuffer->RT_UploadData(Buffer::FromValue(roughness));
			cmd->Dispatch(numGroup, numGroup, 6);

			ID3D11UnorderedAccessView* nullUAV = nullptr;
			cmd->CSSetUnorderedAccessViews(outputTexInfo.DXBinding, 1, &nullUAV, nullptr);
		}

		device->FlushCommandBuffer(cmd);
	}

	void DirectXRenderer::RT_EnvironmentIrradiance(Ref<DirectXTextureCube> filtered, Ref<DirectXTextureCube> irradiance, uint32_t cubemapSize)
	{
		SK_PROFILE_SCOPED("Environment Irradiance");
		auto device = DirectXContext::GetCurrentDevice();
		auto cmd = device->AllocateCommandBuffer();

		Ref<DirectXShader> environmentIrradianceShader = Renderer::GetShaderLibrary()->Get("EnvironmentIrradiance").As<DirectXShader>();
		irradiance->GetImage().As<DirectXImage2D>()->RT_CreateUnorderAccessView(0);

		const auto& envFilteredImageInfo = filtered->GetDirectXImageInfo();
		const auto& radianceMapInfo = environmentIrradianceShader->GetResourceInfo("u_RadianceMap");
		cmd->CSSetShaderResources(radianceMapInfo.DXBinding, 1, &envFilteredImageInfo.View);
		cmd->CSSetSamplers(radianceMapInfo.DXSamplerBinding, 1, &envFilteredImageInfo.Sampler);

		const auto& irradianceMapInfo = environmentIrradianceShader->GetResourceInfo("o_IrradianceMap");
		cmd->CSSetUnorderedAccessViews(irradianceMapInfo.DXBinding, 1, &irradiance->GetUAV(0), nullptr);

		Ref<DirectXConstantBuffer> samplesBuffer = Ref<DirectXConstantBuffer>::Create();
		const auto& bufferInfo = environmentIrradianceShader->GetResourceInfo("u_Uniforms");
		samplesBuffer->SetSize(bufferInfo.StructSize);
		samplesBuffer->RT_Invalidate();
		samplesBuffer->RT_UploadData(Buffer::FromValue(Renderer::GetConfig().IrradianceMapComputeSamples));
		cmd->CSSetConstantBuffers(bufferInfo.DXBinding, 1, &samplesBuffer->m_ConstantBuffer);

		cmd->CSSetShader(environmentIrradianceShader->m_ComputeShader, nullptr, 0);
		cmd->Dispatch(irradiance->GetWidth() / 32, irradiance->GetHeight() / 32, 6);

		ID3D11UnorderedAccessView* nullUAV = nullptr;
		cmd->CSSetUnorderedAccessViews(irradianceMapInfo.DXBinding, 1, &nullUAV, nullptr);

		device->FlushCommandBuffer(cmd);

		Renderer::RT_GenerateMips(irradiance->GetImage());
	}

	std::pair<Ref<TextureCube>, Ref<TextureCube>> DirectXRenderer::CreateEnvironmentMap(const std::filesystem::path& filepath)
	{
		SK_PROFILE_FUNCTION();
		const uint32_t cubemapSize = Renderer::GetConfig().EnvironmentMapResolution;
		const uint32_t irradianceMapSize = 32;

		Ref<Texture2D> equirectangular = Texture2D::Create(TextureSpecification(), filepath);
		SK_CORE_VERIFY(equirectangular->GetSpecification().Format == ImageFormat::RGBA32Float, "Environment Texture is not HDR!");

		TextureSpecification cubemapSpec;
		cubemapSpec.Format = ImageFormat::RGBA32Float;
		cubemapSpec.Width = cubemapSize;
		cubemapSpec.Height = cubemapSize;
		cubemapSpec.GenerateMips = true;

		cubemapSpec.DebugName = fmt::format("EnvironmentMap Unfiltered {}", filepath);
		Ref<TextureCube> unfiltered = TextureCube::Create(cubemapSpec);
		cubemapSpec.DebugName = fmt::format("EnvironmentMap Filtered {}", filepath);
		Ref<TextureCube> filtered = TextureCube::Create(cubemapSpec);

		cubemapSpec.Width = irradianceMapSize;
		cubemapSpec.Height = irradianceMapSize;
		cubemapSpec.DebugName = fmt::format("IrradianceMap {}", filepath);
		Ref<TextureCube> irradianceMap = TextureCube::Create(cubemapSpec);

		Ref<DirectXRenderer> instance = this;
		Renderer::Submit([instance, equirectangular, unfiltered, filtered, irradianceMap, cubemapSize]()
		{
			instance->RT_EquirectangularToCubemap(equirectangular.As<DirectXTexture2D>(), unfiltered.As<DirectXTextureCube>(), cubemapSize);
			instance->RT_EnvironmentMipFilter(unfiltered.As<DirectXTextureCube>(), filtered.As<DirectXTextureCube>(), cubemapSize);
			instance->RT_EnvironmentIrradiance(filtered.As<DirectXTextureCube>(), irradianceMap.As<DirectXTextureCube>(), cubemapSize);
		});

		return { filtered, irradianceMap };
	}

	std::pair<Ref<TextureCube>, Ref<TextureCube>> DirectXRenderer::RT_CreateEnvironmentMap(const std::filesystem::path& filepath)
	{
		SK_PROFILE_FUNCTION();
		const uint32_t cubemapSize = Renderer::GetConfig().EnvironmentMapResolution;
		const uint32_t irradianceMapSize = 32;

		Ref<Texture2D> equirectangular = Texture2D::Create(TextureSpecification(), filepath);
		SK_CORE_VERIFY(equirectangular->GetSpecification().Format == ImageFormat::RGBA32Float, "Environment Texture is not HDR!");

		TextureSpecification cubemapSpec;
		cubemapSpec.Format = ImageFormat::RGBA32Float;
		cubemapSpec.Width = cubemapSize;
		cubemapSpec.Height = cubemapSize;
		cubemapSpec.GenerateMips = true;

		cubemapSpec.DebugName = fmt::format("EnvironmentMap Unfiltered {}", filepath);
		Ref<TextureCube> unfiltered = TextureCube::Create(cubemapSpec);
		cubemapSpec.DebugName = fmt::format("EnvironmentMap Filtered {}", filepath);
		Ref<TextureCube> filtered = TextureCube::Create(cubemapSpec);

		cubemapSpec.Width = irradianceMapSize;
		cubemapSpec.Height = irradianceMapSize;
		cubemapSpec.DebugName = fmt::format("IrradianceMap {}", filepath);
		Ref<TextureCube> irradianceMap = TextureCube::Create(cubemapSpec);

		RT_EquirectangularToCubemap(equirectangular.As<DirectXTexture2D>(), unfiltered.As<DirectXTextureCube>(), cubemapSize);
		RT_EnvironmentMipFilter(unfiltered.As<DirectXTextureCube>(), filtered.As<DirectXTextureCube>(), cubemapSize);
		RT_EnvironmentIrradiance(filtered.As<DirectXTextureCube>(), irradianceMap.As<DirectXTextureCube>(), cubemapSize);

		return { filtered, irradianceMap };
	}

	Ref<Texture2D> DirectXRenderer::CreateBRDFLUT()
	{
		Ref<Shader> brdfLUTShader = Renderer::GetShaderLibrary()->Get("BRDF_LUT");

		uint32_t imageSize = 256;

		TextureSpecification texSpec;
		texSpec.Format = ImageFormat::RG16Float;
		texSpec.Width = imageSize;
		texSpec.Height = imageSize;
		texSpec.GenerateMips = false;
		texSpec.Filter = FilterMode::Linear;
		texSpec.Wrap = WrapMode::Clamp;
		Ref<Texture2D> brdfLUTTex = Texture2D::Create(texSpec);

		Renderer::Submit([brdfLUTShader, brdfLUTTex, imageSize]()
		{
			auto device = DirectXContext::GetCurrentDevice();
			auto cmd = device->AllocateCommandBuffer();

			Ref<DirectXShader> shader = brdfLUTShader.As<DirectXShader>();
			Ref<DirectXTexture2D> texture = brdfLUTTex.As<DirectXTexture2D>();
			Ref<DirectXImage2D> image = texture->GetImage().As<DirectXImage2D>();
			image->RT_CreateUnorderAccessView(0);
			
			const auto& textureInfo = shader->GetResourceInfo("LUT");
			cmd->CSSetUnorderedAccessViews(textureInfo.DXBinding, 1, &image->GetUAV(0), nullptr);
			cmd->CSSetShader(shader->m_ComputeShader, nullptr, 0);
			cmd->Dispatch(imageSize / 32, imageSize / 32, 1);

			ID3D11UnorderedAccessView* nullUAV = nullptr;
			cmd->CSSetUnorderedAccessViews(textureInfo.DXBinding, 1, &nullUAV, nullptr);

			device->FlushCommandBuffer(cmd);
		});

		return brdfLUTTex;
	}

	void DirectXRenderer::GenerateMips(Ref<Image2D> image)
	{
		SK_PROFILE_FUNCTION();

		Ref<DirectXRenderer> instance = this;
		Renderer::Submit([instance, dxImage = image.As<DirectXImage2D>()]()
		{
			instance->RT_GenerateMips(dxImage);
		});
	}

	void DirectXRenderer::RT_GenerateMips(Ref<Image2D> image)
	{
		SK_PROFILE_FUNCTION();

		const auto& specs = image->GetSpecification();
		SK_CORE_VERIFY(specs.Type == ImageType::Texture || specs.Type == ImageType::TextureCube);

		if (specs.MipLevels == 1)
			return;

		auto device = DirectXContext::GetCurrentDevice();
		auto dxDevice = device->GetDirectXDevice();
		auto cmd = device->AllocateCommandBuffer();

		Ref<DirectXImage2D> dxImage = image.As<DirectXImage2D>();

		D3D11_TEXTURE2D_DESC genMipsResourceDesc = {};
		dxImage->m_Info.Resource->GetDesc(&genMipsResourceDesc);
		genMipsResourceDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		genMipsResourceDesc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;

		ID3D11Texture2D* genMipsResource = nullptr;
		DirectXAPI::CreateTexture2D(dxDevice, genMipsResourceDesc, nullptr, genMipsResource);
		
		D3D11_SHADER_RESOURCE_VIEW_DESC genMipsViewDesc = {};
		dxImage->m_Info.View->GetDesc(&genMipsViewDesc);
		
		ID3D11ShaderResourceView* genMipsView = nullptr;
		DirectXAPI::CreateShaderResourceView(dxDevice, genMipsResource, genMipsViewDesc, genMipsView);

		cmd->CopyResource(genMipsResource, dxImage->m_Info.Resource);
		cmd->GenerateMips(genMipsView);
		cmd->CopyResource(dxImage->m_Info.Resource, genMipsResource);

		genMipsView->Release();
		genMipsResource->Release();

		device->FlushCommandBuffer(cmd);
	}

	void DirectXRenderer::BindFrameBuffer(ID3D11DeviceContext* context, Ref<DirectXFrameBuffer> framebuffer)
	{
		if (!framebuffer)
		{
			ID3D11RenderTargetView* nullFramebuffers[8];
			memset(nullFramebuffers, 0, sizeof(nullFramebuffers));
			ID3D11DepthStencilView* nullDepthStencil = nullptr;
			context->OMSetRenderTargets(8, nullFramebuffers, nullDepthStencil);
			return;
		}

		context->OMSetRenderTargets(framebuffer->m_Count, framebuffer->m_FrameBuffers.data(), framebuffer->m_DepthStencil);
	}

	void DirectXRenderer::RT_PrepareForSwapchainResize()
	{
		//for (auto context : m_CommandBuffers)
		//	context->RT_ClearState();

		Application& app = Application::Get();
		if (app.GetSpecification().EnableImGui)
		{
			DirectXImGuiLayer& imguiLayer = (DirectXImGuiLayer&)app.GetImGuiLayer();
			imguiLayer.m_CommandBuffer->ReleaseCommandList();

			ID3D11DeviceContext* context = imguiLayer.m_CommandBuffer->GetContext();
			context->Flush();
			context->ClearState();
			
			ID3D11CommandList* commandList;
			context->FinishCommandList(false, &commandList);
			DirectXAPI::ReleaseObject(commandList);
		}
	}

	void DirectXRenderer::RT_PrepareAndBindMaterial(Ref<DirectXRenderCommandBuffer> commandBuffer, Ref<DirectXMaterial> material)
	{
		if (!material)
			return;

		SK_CORE_VERIFY(material->Validate());
		material->Prepare();

		material->RT_UploadBuffers();
		RT_BindResources(commandBuffer, material->GetShader(), material->GetBoundResources());
	}

	void DirectXRenderer::RT_BindResources(Ref<DirectXRenderCommandBuffer> commandBuffer, Ref<Shader> shader, std::span<const BoundResource> boundResources)
	{
		ID3D11DeviceContext* context = commandBuffer->GetContext();
		const auto& reflectionData = shader->GetReflectionData();

		for (const auto& input : boundResources)
		{
			switch (input.Type)
			{
				case InputResourceType::Image2D:
				{
					for (uint32_t i = 0; i < input.Input.size(); i++)
					{
						Ref<DirectXImage2D> image = input.Input[i].As<DirectXImage2D>();
						const DirectXImageInfo& info = image->GetDirectXImageInfo();
						const ShaderReflection::Resource& resource = reflectionData.Resources.at(input.Set).at(input.Binding);

						utils::BindShaderResourceView(context, info.View, resource.Stage, resource.DXBinding + i);
						if (ShaderReflection::IsTextureType(resource.Type))
							utils::BindSamplerStage(context, info.Sampler, resource.Stage, resource.DXSamplerBinding + i);
					}
					break;
				}
				case InputResourceType::Texture2D:
				{
					for (uint32_t i = 0; i < input.Input.size(); i++)
					{
						Ref<DirectXTexture2D> texture = input.Input[i].As<DirectXTexture2D>();
						const DirectXImageInfo& info = texture->GetDirectXImageInfo();
						const ShaderReflection::Resource& resource = reflectionData.Resources.at(input.Set).at(input.Binding);

						utils::BindShaderResourceView(context, info.View, resource.Stage, resource.DXBinding + i);
						utils::BindSamplerStage(context, info.Sampler, resource.Stage, resource.DXSamplerBinding + i);
					}
					break;
				}
				case InputResourceType::TextureCube:
				{
					for (uint32_t i = 0; i < input.Input.size(); i++)
					{
						Ref<DirectXTextureCube> textureCube = input.Input[i].As<DirectXTextureCube>();
						const DirectXImageInfo& info = textureCube->GetDirectXImageInfo();
						const ShaderReflection::Resource& resource = reflectionData.Resources.at(input.Set).at(input.Binding);

						utils::BindShaderResourceView(context, info.View, resource.Stage, resource.DXBinding + i);
						utils::BindSamplerStage(context, info.Sampler, resource.Stage, resource.DXSamplerBinding + i);
					}
					break;
				}
				case InputResourceType::ConstantBuffer:
				{
					for (uint32_t i = 0; i < input.Input.size(); i++)
					{
						Ref<DirectXConstantBuffer> constantBuffer = input.Input[i].As<DirectXConstantBuffer>();
						const ShaderReflection::Resource& resource = reflectionData.Resources.at(input.Set).at(input.Binding);

						utils::BindConstantBuffer(context, constantBuffer->m_ConstantBuffer, resource.Stage, resource.DXBinding + i);
					}
					break;
				}
				case InputResourceType::StorageBuffer:
				{
					for (uint32_t i = 0; i < input.Input.size(); i++)
					{
						Ref<DirectXStorageBuffer> storageBuffer = input.Input[i].As<DirectXStorageBuffer>();
						const ShaderReflection::Resource& resource = reflectionData.Resources.at(input.Set).at(input.Binding);

						utils::BindShaderResourceView(context, storageBuffer->m_View, resource.Stage, resource.DXBinding + i);
					}
					break;
				}
			}
		}
	}

	void DirectXRenderer::RT_BindPushConstants(Ref<DirectXRenderCommandBuffer> commandBuffer, Ref<DirectXPipeline> pipeline)
	{
		if (pipeline->UsesPushConstant())
		{
			ID3D11DeviceContext* context = commandBuffer->GetContext();
			Ref<DirectXConstantBuffer> pushConstantBuffer = pipeline->GetLastUpdatedPushConstantBuffer().As<DirectXConstantBuffer>();
			const auto& reflectionData = pipeline->GetSpecification().Shader->GetReflectionData();
			utils::BindConstantBuffer(context, pushConstantBuffer->m_ConstantBuffer, reflectionData.PushConstant.Stage, reflectionData.PushConstant.DXBinding);
		}
	}

	void DirectXRenderer::QueryCapabilities()
	{
		m_Capabilities.MaxMipLeves     = D3D11_REQ_MIP_LEVELS;
		m_Capabilities.MaxAnisotropy   = D3D11_REQ_MAXANISOTROPY;
		m_Capabilities.MinLODBias      = D3D11_MIP_LOD_BIAS_MIN;
		m_Capabilities.MaxLODBias      = D3D11_MIP_LOD_BIAS_MAX;
	}

	void DirectXRenderer::RT_BeginFrequencyQuery()
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_VERIFY(Renderer::IsOnRenderThread());

		if (m_DoFrequencyQuery)
		{
			auto device = DirectXContext::GetCurrentDevice();
			auto queue = device->GetQueue();

			std::scoped_lock lock(device->GetSubmissionMutex());
			queue->Begin(m_FrequencyQuery);
		}
	}

	void DirectXRenderer::RT_EndFrequencyQuery()
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_VERIFY(Renderer::IsOnRenderThread());

		auto device = DirectXContext::GetCurrentDevice();
		auto queue = device->GetQueue();

		std::scoped_lock lock(device->GetSubmissionMutex());

		if (m_DoFrequencyQuery)
			queue->End(m_FrequencyQuery);

		m_DoFrequencyQuery = false;

		D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjointData;
		HRESULT hr = queue->GetData(m_FrequencyQuery, &disjointData, sizeof(D3D11_QUERY_DATA_TIMESTAMP_DISJOINT), D3D11_ASYNC_GETDATA_DONOTFLUSH);
		if (hr == S_OK)
		{
			m_GPUFrequency = disjointData.Frequency;
			m_IsValidFrequency = !disjointData.Disjoint;
			m_DoFrequencyQuery = true;
		}

	}

#if 0
	void DirectXRenderer::RT_FlushDXMessages()
	{
		ID3D11InfoQueue* infoQueue;
		HRESULT hr = m_Device->QueryInterface(&infoQueue);
		DX11_VERIFY(hr);
		if (SUCCEEDED(hr))
		{
			Buffer messageBuffer;
			uint64_t count = infoQueue->GetNumStoredMessages();
			for (uint64_t i = 0; i < count; i++)
			{
				uint64_t messageLength;
				if (HRESULT hr = infoQueue->GetMessage(i, nullptr, &messageLength); FAILED(hr))
				{
					SK_CORE_ERROR_TAG("Renderer", "Failed to receive message from InfoQueue");
					continue;
				}

				messageBuffer.Resize(messageLength, false);
				auto message = messageBuffer.As<D3D11_MESSAGE>();

				SK_DX11_CALL(infoQueue->GetMessage(i, message, &messageLength));

				m_InfoQueue->AddMessage(DXGI_DEBUG_D3D11,
					(DXGI_INFO_QUEUE_MESSAGE_CATEGORY)message->Category,
					(DXGI_INFO_QUEUE_MESSAGE_SEVERITY)message->Severity,
					(DXGI_INFO_QUEUE_MESSAGE_ID)message->ID,
					message->pDescription
				);

			}

			infoQueue->ClearStoredMessages();
			infoQueue->Release();
			messageBuffer.Release();
		}
	}
#endif

}