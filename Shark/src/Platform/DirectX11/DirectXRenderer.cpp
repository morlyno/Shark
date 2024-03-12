#include "skpch.h"
#undef GetMessage
#include "DirectXRenderer.h"

#include "Shark/Core/Application.h"
#include "Shark/Core/Timer.h"
#include "Shark/Render/Renderer.h"

#include "Shark/Utils/String.h"
#include "Shark/Debug/Profiler.h"

#include "Platform/DirectX11/DirectXAPI.h"
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

#if SK_ENABLE_VALIDATION && false
#define DX11_VALIDATE_CONTEXT(ctx) DirectXRenderer::Get()->GetDebug()->ValidateContext(ctx);
#else
#define DX11_VALIDATE_CONTEXT(ctx)
#endif

namespace Shark {

	DirectXRenderer* DirectXRenderer::s_Instance = nullptr;

	namespace Utils {

		static std::string GetGPUDescription(IDXGIAdapter* adapter)
		{
			DXGI_ADAPTER_DESC ad = {};
			SK_DX11_CALL(adapter->GetDesc(&ad));
			return String::ToNarrow(std::wstring_view(ad.Description));
		}

	}

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
		SK_PROFILE_FUNCTION();

		Init();
	}

	DirectXRenderer::~DirectXRenderer()
	{
		SK_PROFILE_FUNCTION();

		ShutDown();
	}

	void DirectXRenderer::Init()
	{
		SK_PROFILE_FUNCTION();

		SK_CORE_VERIFY(s_Instance == nullptr);
		s_Instance = this;

		SK_CORE_INFO_TAG("Renderer", "Initializing DirectX Renderer");

		Ref<DirectXRenderer> instance = this;
		Renderer::Submit([this, instance]()
		{
			SK_PROFILE_SCOPED("DirectXRenderer::Init");

			bool enableBreakOnSeverity = false;

			UINT factoryFlags = SK_ENABLE_VALIDATION ? DXGI_CREATE_FACTORY_DEBUG : 0;

			DX11_VERIFY(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&instance->m_InfoQueue)));
			DX11_VERIFY(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&instance->m_Factory)));

			DX11_VERIFY(m_InfoQueue->PushEmptyRetrievalFilter(DXGI_DEBUG_ALL));
			DX11_VERIFY(m_InfoQueue->PushEmptyStorageFilter(DXGI_DEBUG_ALL));

			if (enableBreakOnSeverity)
			{
				DX11_VERIFY(m_InfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true));
				DX11_VERIFY(m_InfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true));
			}

			//RT_CreateInfoQueue();
			RT_CreateDevice();
			QueryCapabilities();
			SK_DX11_CALL(m_Device->QueryInterface(&m_Debug));

			ID3D11InfoQueue* infoQueue = nullptr;
			m_Device->QueryInterface(&infoQueue);

			if (enableBreakOnSeverity)
			{
				DX11_VERIFY(infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true));
				DX11_VERIFY(infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true));
			}
			
			D3D11_MESSAGE_ID deniedMessages[] = { D3D11_MESSAGE_ID_DEVICE_DRAW_SHADERRESOURCEVIEW_NOT_SET };
			D3D11_MESSAGE_SEVERITY deniedSeverities[] = { D3D11_MESSAGE_SEVERITY_MESSAGE, D3D11_MESSAGE_SEVERITY_INFO };

			D3D11_INFO_QUEUE_FILTER filter{};
			filter.DenyList.NumIDs = (UINT)std::size(deniedMessages);
			filter.DenyList.pIDList = deniedMessages;
			filter.DenyList.NumSeverities = (UINT)std::size(deniedSeverities);
			filter.DenyList.pSeverityList = deniedSeverities;
			DX11_VERIFY(infoQueue->PushRetrievalFilter(&filter));
			DX11_VERIFY(infoQueue->PushStorageFilter(&filter));

			infoQueue->Release();

			D3D11_QUERY_DESC queryDesc;
			queryDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
			queryDesc.MiscFlags = 0;
			SK_DX11_CALL(instance->m_Device->CreateQuery(&queryDesc, &instance->m_FrequencyQuery));

			D3D11_SAMPLER_DESC samplerDesc = CD3D11_SAMPLER_DESC(D3D11_DEFAULT);
			samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
			samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
			samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
			samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
			DirectXAPI::CreateSamplerState(instance->m_Device, samplerDesc, instance->m_ClampLinearSampler);
		});

		VertexLayout layout = {
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

		Renderer::Submit([instance]() { SK_CORE_INFO_TAG("Renderer", "Resources Created"); instance->m_ResourceCreated = true; });
	}

	void DirectXRenderer::ShutDown()
	{
		SK_PROFILE_FUNCTION();

		SK_CORE_INFO_TAG("Renderer", "Shuting down DirectX Renderer");

		m_QuadVertexBuffer = nullptr;
		m_QuadIndexBuffer = nullptr;
		m_CubeVertexBuffer = nullptr;
		m_CubeIndexBuffer = nullptr;
		m_ImmediateCommandBuffer = nullptr;

		Renderer::SubmitResourceFree([frequencyQuery = m_FrequencyQuery, factory = m_Factory, context = m_ImmediateContext, device = m_Device, debug = m_Debug, infoQueue = m_InfoQueue, sampler = m_ClampLinearSampler]()
		{
			frequencyQuery->Release();
			factory->Release();
			debug->Release();
			RT_LogMessages(infoQueue);
			infoQueue->Release();
			sampler->Release();
			context->ClearState();
			context->Flush();
			context->Release();

			ULONG count = device->Release();
			SK_CORE_ASSERT(count == 0);
		});

		m_FrequencyQuery = nullptr;
		m_Factory = nullptr;
		m_InfoQueue = nullptr;
		m_Debug = nullptr;
		m_ImmediateContext = nullptr;
		m_Device = nullptr;

		s_Instance = nullptr;
	}

	void DirectXRenderer::BeginFrame()
	{
		Ref<DirectXRenderer> instance = this;
		Renderer::Submit([instance]() { instance->m_RTFrameIndex++; });
		m_FrameIndex++;

		m_ImmediateCommandBuffer->Begin();
		m_GPUTimeQuery = m_ImmediateCommandBuffer->BeginTimestampQuery();

		Renderer::Submit([instance]()
		{
			instance->RT_BeginFrequencyQuery();
		});
	}

	void DirectXRenderer::EndFrame()
	{
		Ref<DirectXRenderer> instance = this;
		Renderer::Submit([this, instance]()
		{
			RT_EndFrequencyQuery();
			RT_FlushDXMessages();
			RT_LogMessages(m_InfoQueue);
		});

		m_ImmediateCommandBuffer->EndTimestampQuery(m_GPUTimeQuery);
		m_ImmediateCommandBuffer->End();
		m_ImmediateCommandBuffer->Execute();
		m_GPUTime = m_ImmediateCommandBuffer->GetTime(m_GPUTimeQuery);
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
		Renderer::Submit([instance, dxCommandBuffer, dxPipeline, dxMaterial, vertexBuffer = m_QuadVertexBuffer, indexBuffer = m_QuadIndexBuffer]()
		{
			SK_PROFILE_SCOPED("DirectXRenderer::RenderFullScreenQuad");

			ID3D11DeviceContext* ctx = dxCommandBuffer->GetContext();

			Ref<DirectXShader> dxShader = dxPipeline->m_Shader;
			ctx->VSSetShader(dxShader->m_VertexShader, nullptr, 0);
			ctx->PSSetShader(dxShader->m_PixelShader, nullptr, 0);

			const UINT offset = 0;
			const UINT stride = dxPipeline->GetSpecification().Layout.GetVertexSize();
			ctx->IASetVertexBuffers(0, 1, &vertexBuffer->m_VertexBuffer, &stride, &offset);
			ctx->IASetIndexBuffer(indexBuffer->m_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
			ctx->IASetInputLayout(dxPipeline->m_InputLayout);

			ctx->RSSetState(dxPipeline->m_RasterizerState);
			ctx->OMSetDepthStencilState(dxPipeline->m_DepthStencilState, 0);

			Ref<DirectXFrameBuffer> dxFrameBuffer = dxPipeline->m_FrameBuffer;
			ctx->OMSetRenderTargets(dxFrameBuffer->m_Count, dxFrameBuffer->m_FrameBuffers.data(), dxFrameBuffer->m_DepthStencil);
			ctx->OMSetBlendState(dxFrameBuffer->m_BlendState, nullptr, 0xFFFFFFFF);
			ctx->RSSetViewports(1, &dxFrameBuffer->m_Viewport);

			instance->RT_PrepareAndBindMaterial(dxCommandBuffer, dxMaterial);
			instance->RT_BindPushConstants(dxCommandBuffer, dxPipeline);
			
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
			ctx->OMSetDepthStencilState(dxPipeline->m_DepthStencilState, 0);
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
			ctx->OMSetDepthStencilState(dxPipeline->m_DepthStencilState, 0);
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
			ctx->OMSetDepthStencilState(dxPipeline->m_DepthStencilState, 0);
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
		SK_CORE_VERIFY(mesh);
		SK_CORE_VERIFY(materialTable);

		Ref instance = this;
		auto dxCommandBuffer = commandBuffer.As<DirectXRenderCommandBuffer>();
		auto dxPipeline = pipeline.As<DirectXPipeline>();
		Renderer::Submit([instance, dxCommandBuffer, dxPipeline, mesh, submeshIndex, materialTable]()
		{
			SK_PROFILE_SCOPED("DirectXRenderer::RenderMesh");

			ID3D11DeviceContext* ctx = dxCommandBuffer->GetContext();

			Ref<MeshSource> meshSource = mesh->GetMeshSource();
			auto vertexBuffer = meshSource->GetVertexBuffer().As<DirectXVertexBuffer>();
			auto indexBuffer = meshSource->GetIndexBuffer().As<DirectXIndexBuffer>();

			const UINT offset = 0;
			const UINT stride = dxPipeline->GetSpecification().Layout.GetVertexSize();
			ctx->IASetVertexBuffers(0, 1, &vertexBuffer->m_VertexBuffer, &stride, &offset);
			ctx->IASetIndexBuffer(indexBuffer->m_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);

			Ref<DirectXShader> dxShader = dxPipeline->m_Shader;
			ctx->VSSetShader(dxShader->m_VertexShader, nullptr, 0);
			ctx->PSSetShader(dxShader->m_PixelShader, nullptr, 0);

			const auto& submeshes = meshSource->GetSubmeshes();
			const auto& submesh = submeshes[submeshIndex];

			Ref<MaterialTable> meshMaterialTable = mesh->GetMaterialTable();
			const auto& materials = meshSource->GetMaterials();

			AssetHandle materialHandle = materialTable->HasMaterial(submesh.MaterialIndex) ? materialTable->GetMaterial(submesh.MaterialIndex) : materialTable->GetMaterial(submesh.MaterialIndex);
			Ref<MaterialAsset> materialAsset = AssetManager::GetAsset<MaterialAsset>(materialHandle);
			Ref<DirectXMaterial> material = materialAsset->GetMaterial().As<DirectXMaterial>();

			instance->RT_PrepareAndBindMaterial(dxCommandBuffer, material);

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
			ctx->OMSetDepthStencilState(dxPipeline->m_DepthStencilState, 0);
			ctx->OMSetBlendState(dxFrameBuffer->m_BlendState, nullptr, 0xFFFFFFFF);

			ctx->IASetPrimitiveTopology(dxPipeline->m_PrimitveTopology);
			ctx->IASetInputLayout(dxPipeline->m_InputLayout);

			DX11_VALIDATE_CONTEXT(ctx);
			ctx->DrawIndexed(submesh.IndexCount, submesh.BaseIndex, submesh.BaseVertex);
		});
	}

	void DirectXRenderer::RenderSubmeshWithMaterial(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Mesh> mesh, uint32_t submeshIndex, Ref<Material> material)
	{
		SK_CORE_VERIFY(mesh);
		SK_CORE_VERIFY(material);

		Ref instance = this;
		auto dxCommandBuffer = commandBuffer.As<DirectXRenderCommandBuffer>();
		auto dxPipeline = pipeline.As<DirectXPipeline>();
		auto dxMaterial = material.As<DirectXMaterial>();
		Renderer::Submit([instance, dxCommandBuffer, dxPipeline, mesh, submeshIndex, dxMaterial]()
		{
			SK_PROFILE_SCOPED("DirectXRenderer::RenderMesh");

			ID3D11DeviceContext* ctx = dxCommandBuffer->GetContext();

			Ref<MeshSource> meshSource = mesh->GetMeshSource();
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
			ctx->OMSetDepthStencilState(dxPipeline->m_DepthStencilState, 0);
			ctx->OMSetBlendState(dxFrameBuffer->m_BlendState, nullptr, 0xFFFFFFFF);

			ctx->IASetPrimitiveTopology(dxPipeline->m_PrimitveTopology);

			DX11_VALIDATE_CONTEXT(ctx);
			ctx->DrawIndexed(submesh.IndexCount, submesh.BaseIndex, submesh.BaseVertex);
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

	void DirectXRenderer::RT_CopyImage(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> sourceImage, Ref<Image2D> destinationImage)
	{
		SK_CORE_VERIFY(sourceImage);
		SK_CORE_VERIFY(destinationImage);

		Ref<DirectXRenderCommandBuffer> dxCommandBuffer = commandBuffer.As<DirectXRenderCommandBuffer>();
		Ref<DirectXImage2D> dxSourceImage = sourceImage.As<DirectXImage2D>();
		Ref<DirectXImage2D> dxDestImage = destinationImage.As<DirectXImage2D>();

		ID3D11DeviceContext* context = dxCommandBuffer->GetContext();

		const DirectXImageInfo& sourceInfo = dxSourceImage->GetDirectXImageInfo();
		const DirectXImageInfo& destinationInfo = dxDestImage->GetDirectXImageInfo();

		context->CopyResource(destinationInfo.Resource, sourceInfo.Resource);
	}

	std::pair<Ref<TextureCube>, Ref<TextureCube>> DirectXRenderer::CreateEnvironmentMap(const std::filesystem::path& filepath)
	{
		const uint32_t cubemapSize = Renderer::GetConfig().EnvironmentMapResolution;
		const uint32_t irradianceMapSize = 32;

		Ref<Texture2D> envEquirect = Texture2D::Create(TextureSpecification(), filepath);
		SK_CORE_VERIFY(envEquirect->GetSpecification().Format == ImageFormat::RGBA32F, "Environment Texture is not HDR!");

		TextureSpecification cubemapSpec;
		cubemapSpec.Format = ImageFormat::RGBA32F;
		cubemapSpec.Width = cubemapSize;
		cubemapSpec.Height = cubemapSize;
		cubemapSpec.GenerateMips = true;

		cubemapSpec.DebugName = fmt::format("EnvironmentMap {}", filepath);
		Ref<TextureCube> envUnfiltered = TextureCube::Create(cubemapSpec);
		Ref<TextureCube> envFiltered = TextureCube::Create(cubemapSpec);

		Ref<Shader> equirectangularConversionShader = Renderer::GetShaderLibrary()->Get("EquirectangularToCubeMap");

		Renderer::Submit([equirectangularConversionShader, envEquirect, envUnfiltered, cubemapSize]()
		{
			ID3D11DeviceContext* context = DirectXRenderer::Get()->GetContext();
			Ref<DirectXShader> shader = equirectangularConversionShader.As<DirectXShader>();

			Ref<DirectXTexture2D> equirect = envEquirect.As<DirectXTexture2D>();
			const auto& equirectImageInfo = equirect->GetDirectXImageInfo();
			const auto& equirectTexInfo = equirectangularConversionShader->GetResourceInfo("u_EquirectangularTex");
			context->CSSetShaderResources(equirectTexInfo.DXBinding, 1, &equirectImageInfo.View);
			context->CSSetSamplers(equirectTexInfo.DXSamplerBinding, 1, &equirectImageInfo.Sampler);

			Ref<DirectXTextureCube> dxEnvCubemap = envUnfiltered.As<DirectXTextureCube>();
			dxEnvCubemap->GetImage().As<DirectXImage2D>()->RT_CreateUnorderAccessView(0);
			const auto& cubeMapInfo = equirectangularConversionShader->GetResourceInfo("o_CubeMap");
			context->CSSetUnorderedAccessViews(cubeMapInfo.DXBinding, 1, &dxEnvCubemap->GetUAV(0), nullptr);

			context->CSSetShader(shader->m_ComputeShader, nullptr, 0);
			context->Dispatch(cubemapSize / 32, cubemapSize / 32, 6);
			DirectXRenderer::Get()->RT_FlushInfoQueue();

			ID3D11UnorderedAccessView* nullUAV = nullptr;
			context->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr);

			Renderer::RT_GenerateMips(dxEnvCubemap->GetImage());
		});

		Ref<Shader> environmentMipFilterShader = Renderer::GetShaderLibrary()->Get("EnvironmentMipFilter");

		Renderer::Submit([environmentMipFilterShader, envUnfiltered, envFiltered, cubemapSize]()
		{
			ID3D11DeviceContext* context = DirectXRenderer::Get()->GetContext();
			Ref<DirectXShader> shader = environmentMipFilterShader.As<DirectXShader>();

			Ref<DirectXTextureCube> envUnfilteredCubemap = envUnfiltered.As<DirectXTextureCube>();
			Ref<DirectXTextureCube> envFilteredCubemap = envFiltered.As<DirectXTextureCube>();
			envFilteredCubemap->GetImage().As<DirectXImage2D>()->RT_CreatePerMipUAV();

			uint32_t mipCount = ImageUtils::CalcMipLevels(cubemapSize, cubemapSize);

			const auto& bufferInfo = environmentMipFilterShader->GetPushConstantInfo();
			Ref<DirectXConstantBuffer> constantBuffer = Ref<DirectXConstantBuffer>::Create(bufferInfo.StructSize);

			const float deltaRoughness = 1.0f / glm::max((float)envFiltered->GetMipLevelCount() - 1.0f, 1.0f);
			for (uint32_t i = 0, size = cubemapSize; i < mipCount; i++, size /= 2)
			{
				Ref<DirectXImageView> envUnfilterImageView = Ref<DirectXImageView>::Create(envUnfiltered->GetImage(), i);

				uint32_t numGroup = glm::max(1u, size / 32);
				float roughness = glm::max(i * deltaRoughness, 0.05f);

				const auto& inputTexInfo = environmentMipFilterShader->GetResourceInfo("inputTexture");
				context->CSSetShaderResources(inputTexInfo.DXBinding, 1, &envUnfilterImageView->m_View);
				context->CSSetSamplers(inputTexInfo.DXSamplerBinding, 1, &envUnfilteredCubemap->GetDirectXImageInfo().Sampler);

				const auto& outputTexInfo = environmentMipFilterShader->GetResourceInfo("outputTexture");
				context->CSSetUnorderedAccessViews(outputTexInfo.DXBinding, 1, &envFilteredCubemap->GetUAV(i), nullptr);

				context->CSSetShader(shader->m_ComputeShader, nullptr, 0);
				constantBuffer->RT_UploadData(Buffer::FromValue(roughness));
				context->Dispatch(numGroup, numGroup, 6);

				ID3D11UnorderedAccessView* nullUAV = nullptr;
				context->CSSetUnorderedAccessViews(outputTexInfo.DXBinding, 1, &nullUAV, nullptr);
			}

		});

		Ref<Shader> environmentIrradianceShader = Renderer::GetShaderLibrary()->Get("EnvironmentIrradiance");

		cubemapSpec.Width = irradianceMapSize;
		cubemapSpec.Height = irradianceMapSize;
		cubemapSpec.DebugName = fmt::format("IrradianceMap {}", filepath);
		Ref<TextureCube> irradianceMap = TextureCube::Create(cubemapSpec);

		Renderer::Submit([environmentIrradianceShader, irradianceMap, envFiltered]()
		{
			ID3D11DeviceContext* context = DirectXRenderer::Get()->GetContext();
			Ref<DirectXShader> shader = environmentIrradianceShader.As<DirectXShader>();
			
			Ref<DirectXTextureCube> envCubemap = envFiltered.As<DirectXTextureCube>();
			Ref<DirectXTextureCube> irradianceCubemap = irradianceMap.As<DirectXTextureCube>();
			irradianceCubemap->GetImage().As<DirectXImage2D>()->RT_CreateUnorderAccessView(0);

			const auto& envFilteredImageInfo = envCubemap->GetDirectXImageInfo();
			const auto& radianceMapInfo = environmentIrradianceShader->GetResourceInfo("u_RadianceMap");
			context->CSSetShaderResources(radianceMapInfo.DXBinding, 1, &envFilteredImageInfo.View);
			context->CSSetSamplers(radianceMapInfo.DXSamplerBinding, 1, &envFilteredImageInfo.Sampler);

			const auto& irradianceMapInfo = environmentIrradianceShader->GetResourceInfo("o_IrradianceMap");
			context->CSSetUnorderedAccessViews(irradianceMapInfo.DXBinding, 1, &irradianceCubemap->GetUAV(0), nullptr);

			Ref<DirectXConstantBuffer> samplesBuffer = Ref<DirectXConstantBuffer>::Create();
			const auto& bufferInfo = environmentIrradianceShader->GetResourceInfo("u_Uniforms");
			samplesBuffer->SetSize(bufferInfo.StructSize);
			samplesBuffer->RT_Invalidate();
			samplesBuffer->RT_UploadData(Buffer::FromValue(Renderer::GetConfig().IrradianceMapComputeSamples));
			context->CSSetConstantBuffers(bufferInfo.DXBinding, 1, &samplesBuffer->m_ConstantBuffer);

			context->CSSetShader(shader->m_ComputeShader, nullptr, 0);
			context->Dispatch(irradianceMap->GetWidth() / 32, irradianceMap->GetHeight() / 32, 6);
			DirectXRenderer::Get()->RT_FlushInfoQueue();

			ID3D11UnorderedAccessView* nullUAV = nullptr;
			context->CSSetUnorderedAccessViews(irradianceMapInfo.DXBinding, 1, &nullUAV, nullptr);

			Renderer::RT_GenerateMips(irradianceMap->GetImage());
		});

		return { envFiltered, irradianceMap };
	}

	Ref<Texture2D> DirectXRenderer::CreateBRDFLUT()
	{
		Ref<Shader> brdfLUTShader = Renderer::GetShaderLibrary()->Get("BRDF_LUT");

		uint32_t imageSize = 256;

		TextureSpecification texSpec;
		texSpec.Format = ImageFormat::RG16F;
		texSpec.Width = imageSize;
		texSpec.Height = imageSize;
		texSpec.GenerateMips = false;
		texSpec.Filter = FilterMode::Linear;
		texSpec.Wrap = WrapMode::Clamp;
		Ref<Texture2D> brdfLUTTex = Texture2D::Create(texSpec);

		Renderer::Submit([brdfLUTShader, brdfLUTTex, imageSize]()
		{
			ID3D11DeviceContext* context = DirectXRenderer::Get()->GetContext();
			Ref<DirectXShader> shader = brdfLUTShader.As<DirectXShader>();
			Ref<DirectXTexture2D> texture = brdfLUTTex.As<DirectXTexture2D>();
			Ref<DirectXImage2D> image = texture->GetImage().As<DirectXImage2D>();
			image->RT_CreateUnorderAccessView(0);
			
			const auto& textureInfo = shader->GetResourceInfo("LUT");
			context->CSSetUnorderedAccessViews(textureInfo.DXBinding, 1, &image->GetUAV(0), nullptr);
			context->CSSetShader(shader->m_ComputeShader, nullptr, 0);
			context->Dispatch(imageSize / 32, imageSize / 32, 1);

			ID3D11UnorderedAccessView* nullUAV = nullptr;
			context->CSSetUnorderedAccessViews(textureInfo.DXBinding, 1, &nullUAV, nullptr);
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

		Ref<DirectXImage2D> dxImage = image.As<DirectXImage2D>();

		D3D11_TEXTURE2D_DESC genMipsResourceDesc = {};
		dxImage->m_Info.Resource->GetDesc(&genMipsResourceDesc);
		genMipsResourceDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		genMipsResourceDesc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;

		ID3D11Texture2D* genMipsResource = nullptr;
		DirectXAPI::CreateTexture2D(m_Device, genMipsResourceDesc, nullptr, genMipsResource);
		
		D3D11_SHADER_RESOURCE_VIEW_DESC genMipsViewDesc = {};
		dxImage->m_Info.View->GetDesc(&genMipsViewDesc);
		
		ID3D11ShaderResourceView* genMipsView = nullptr;
		DirectXAPI::CreateShaderResourceView(m_Device, genMipsResource, genMipsViewDesc, genMipsView);

		m_ImmediateContext->CopyResource(genMipsResource, dxImage->m_Info.Resource);
		m_ImmediateContext->GenerateMips(genMipsView);
		m_ImmediateContext->CopyResource(dxImage->m_Info.Resource, genMipsResource);

		genMipsView->Release();
		genMipsResource->Release();
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

	void DirectXRenderer::HandleError(HRESULT hr)
	{
		if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_HUNG || hr == DXGI_ERROR_DEVICE_RESET)
		{
			HRESULT deviceRemovedHR = m_Device->GetDeviceRemovedReason();
			auto deviceRemovedReason = WindowsUtils::TranslateHResult(deviceRemovedHR);
			SK_CORE_CRITICAL_TAG("Renderer", deviceRemovedReason);
			return;
		}

		RT_FlushDXMessages();
		RT_LogMessages(m_InfoQueue);

		static bool BreakOnError = true;
		if (BreakOnError)
		{
			SK_DEBUG_BREAK();
		}

		//SK_CORE_ERROR_TAG("Renderer", WindowsUtils::TranslateHResult(hr));
	}

	void DirectXRenderer::RT_FlushInfoQueue()
	{
		SK_CORE_VERIFY(Renderer::IsOnRenderThread());
		RT_FlushDXMessages();
		RT_LogMessages(m_InfoQueue);
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

		m_ImmediateContext->Flush();
		m_ImmediateContext->ClearState();
	}

	void DirectXRenderer::ReportLiveObejcts()
	{
		IDXGIDebug1* debug;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug))))
		{
			debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL);

			IDXGIInfoQueue* infoQueue;
			if (SUCCEEDED(debug->QueryInterface(IID_PPV_ARGS(&infoQueue))))
			{
				RT_LogMessages(infoQueue);
				infoQueue->Release();
			}
			debug->Release();
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

	void DirectXRenderer::RT_BindResources(Ref<DirectXRenderCommandBuffer> commandBuffer, Ref<Shader> shader, const std::vector<BoundResource>& boundResources)
	{
		ID3D11DeviceContext* context = commandBuffer->GetContext();
		const auto& reflectionData = shader->GetReflectionData();

		for (const auto& input : boundResources)
		{
			switch (input.Type)
			{
				case InputResourceType::Image2D:
				{
					Ref<DirectXImage2D> image = input.Input.As<DirectXImage2D>();
					const DirectXImageInfo& info = image->GetDirectXImageInfo();
					const ShaderReflection::Resource& resource = reflectionData.Resources.at(input.Set).at(input.Binding);

					utils::BindShaderResourceView(context, info.View, resource.Stage, resource.DXBinding);
					if (ShaderReflection::IsTextureType(resource.Type))
						utils::BindSamplerStage(context, info.Sampler, resource.Stage, resource.DXSamplerBinding);
					break;
				}
				case InputResourceType::Texture2D:
				{
					Ref<DirectXTexture2D> texture = input.Input.As<DirectXTexture2D>();
					const DirectXImageInfo& info = texture->GetDirectXImageInfo();
					const ShaderReflection::Resource& resource = reflectionData.Resources.at(input.Set).at(input.Binding);

					utils::BindShaderResourceView(context, info.View, resource.Stage, resource.DXBinding);
					utils::BindSamplerStage(context, info.Sampler, resource.Stage, resource.DXSamplerBinding);
					break;
				}
				case InputResourceType::TextureCube:
				{
					Ref<DirectXTextureCube> textureCube = input.Input.As<DirectXTextureCube>();
					const DirectXImageInfo& info = textureCube->GetDirectXImageInfo();
					const ShaderReflection::Resource& resource = reflectionData.Resources.at(input.Set).at(input.Binding);

					utils::BindShaderResourceView(context, info.View, resource.Stage, resource.DXBinding);
					utils::BindSamplerStage(context, info.Sampler, resource.Stage, resource.DXSamplerBinding);
					break;
				}
				case InputResourceType::ConstantBuffer:
				{
					Ref<DirectXConstantBuffer> constantBuffer = input.Input.As<DirectXConstantBuffer>();
					const ShaderReflection::Resource& resource = reflectionData.Resources.at(input.Set).at(input.Binding);

					utils::BindConstantBuffer(context, constantBuffer->m_ConstantBuffer, resource.Stage, resource.DXBinding);
					break;
				}
				case InputResourceType::StorageBuffer:
				{
					Ref<DirectXStorageBuffer> storageBuffer = input.Input.As<DirectXStorageBuffer>();
					const ShaderReflection::Resource& resource = reflectionData.Resources.at(input.Set).at(input.Binding);

					utils::BindShaderResourceView(context, storageBuffer->m_View, resource.Stage, resource.DXBinding);
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
			m_ImmediateContext->Begin(m_FrequencyQuery);
	}

	void DirectXRenderer::RT_EndFrequencyQuery()
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_VERIFY(Renderer::IsOnRenderThread());

		if (m_DoFrequencyQuery)
			m_ImmediateContext->End(m_FrequencyQuery);

		m_DoFrequencyQuery = false;

		D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjointData;
		HRESULT hr = m_ImmediateContext->GetData(m_FrequencyQuery, &disjointData, sizeof(D3D11_QUERY_DATA_TIMESTAMP_DISJOINT), D3D11_ASYNC_GETDATA_DONOTFLUSH);
		if (hr == S_OK)
		{
			m_GPUFrequency = disjointData.Frequency;
			m_IsValidFrequency = !disjointData.Disjoint;
			m_DoFrequencyQuery = true;
		}
	}

	void DirectXRenderer::RT_CreateDevice()
	{
		SK_CORE_VERIFY(Renderer::IsOnRenderThread());

		//SK_DX11_CALL(CreateDXGIFactory(IID_PPV_ARGS(&m_Factory)));

		IDXGIAdapter* adapter = nullptr;
		SK_DX11_CALL(m_Factory->EnumAdapters(0, &adapter));
		SK_CORE_INFO_TAG("Renderer", "GPU Selected ({0})", Utils::GetGPUDescription(adapter));

		UINT createdeviceFalgs = 0u;
#if SK_ENABLE_VALIDATION
		createdeviceFalgs |= D3D11_CREATE_DEVICE_DEBUG;
#endif

		SK_DX11_CALL(D3D11CreateDevice(
			adapter,
			D3D_DRIVER_TYPE_UNKNOWN,
			nullptr,
			createdeviceFalgs,
			nullptr,
			0,
			D3D11_SDK_VERSION,
			&m_Device,
			nullptr,
			&m_ImmediateContext
		));

		adapter->Release();

		m_ImmediateCommandBuffer = Ref<DirectXRenderCommandBuffer>::Create(CommandBufferType::Immediate, m_ImmediateContext);
	}

	void DirectXRenderer::RT_CreateInfoQueue()
	{
		SK_CORE_VERIFY(Renderer::IsOnRenderThread());

		typedef HRESULT(WINAPI* DXGIGetDebugInterface)(REFIID, void**);

		const auto hModDxgiDebug = LoadLibraryEx(L"dxgidebug.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
		if (!hModDxgiDebug)
			return;

		DXGIGetDebugInterface dxgiGetDebugInterface = (DXGIGetDebugInterface)GetProcAddress(hModDxgiDebug, "DXGIGetDebugInterface");
		if (!dxgiGetDebugInterface)
			return;

		SK_DX11_CALL(dxgiGetDebugInterface(__uuidof(IDXGIInfoQueue), (void**)&m_InfoQueue));

		SK_DX11_CALL(m_InfoQueue->PushEmptyRetrievalFilter(DXGI_DEBUG_ALL));
		SK_DX11_CALL(m_InfoQueue->PushEmptyStorageFilter(DXGI_DEBUG_ALL));

		SK_DX11_CALL(m_InfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true));
		SK_DX11_CALL(m_InfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true));
	}

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

	void DirectXRenderer::RT_LogMessages(IDXGIInfoQueue* infoQueue)
	{
		SK_PROFILE_FUNCTION();

		const auto& producer = DXGI_DEBUG_ALL;

		Buffer messageBuffer;
		uint64_t count = infoQueue->GetNumStoredMessages(producer);
		for (uint64_t i = 0; i < count; i++)
		{
			uint64_t messageLength;
			SK_DX11_CALL(infoQueue->GetMessage(producer, i, nullptr, &messageLength));

			messageBuffer.Resize(messageLength, false);
			auto message = messageBuffer.As<DXGI_INFO_QUEUE_MESSAGE>();

			SK_DX11_CALL(infoQueue->GetMessage(producer, i, message, &messageLength));

			switch (message->Severity)
			{
				case DXGI_INFO_QUEUE_MESSAGE_SEVERITY_MESSAGE: SK_CORE_TRACE_TAG(Tag::None, "{}", message->pDescription); break;
				case DXGI_INFO_QUEUE_MESSAGE_SEVERITY_INFO: SK_CORE_INFO_TAG(Tag::None, "{}", message->pDescription); break;
				case DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING: SK_CORE_WARN_TAG(Tag::None, "{}", message->pDescription); break;
				case DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR: SK_CORE_ERROR_TAG(Tag::None, "{}", message->pDescription); break;
				case DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION: SK_CORE_CRITICAL_TAG(Tag::None, "{}", message->pDescription); break;
				default: SK_CORE_WARN_TAG(Tag::None, "Unkown Severity! {}", message->pDescription); break;
			}
		}

		infoQueue->ClearStoredMessages(producer);
		messageBuffer.Release();
	}

}