#include "skpch.h"
#undef GetMessage
#include "DirectXRenderer.h"

#include "Shark/Core/Application.h"
#include "Shark/Core/Timer.h"
#include "Shark/Render/Renderer.h"

#include "Shark/Utils/String.h"
#include "Shark/Debug/Profiler.h"

#include "Platform/DirectX11/DirectXRenderCommandBuffer.h"
#include "Platform/DirectX11/DirectXFrameBuffer.h"
#include "Platform/DirectX11/DirectXShader.h"
#include "Platform/DirectX11/DirectXConstantBuffer.h"
#include "Platform/DirectX11/DirectXTexture.h"
#include "Platform/DirectX11/DirectXBuffers.h"
#include "Platform/DirectX11/DirectXPipeline.h"
#include "Platform/DirectX11/DirectXSwapChain.h"
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

		m_QuadVertexBuffer = Ref<DirectXVertexBuffer>::Create(layout, (uint32_t)sizeof(vertices), false, Buffer::FromArray(vertices));
		m_QuadIndexBuffer = Ref<DirectXIndexBuffer>::Create((uint32_t)std::size(indices), false, Buffer::FromArray(indices));

		m_GPUTimer = Ref<DirectXGPUTimer>::Create("GPU");

		Renderer::Submit([instance]() { SK_CORE_INFO_TAG("Renderer", "Resources Created"); instance->m_ResourceCreated = true; });
	}

	void DirectXRenderer::ShutDown()
	{
		SK_PROFILE_FUNCTION();

		SK_CORE_INFO_TAG("Renderer", "Shuting down DirectX Renderer");

		m_QuadVertexBuffer = nullptr;
		m_QuadIndexBuffer = nullptr;

		SK_CORE_ASSERT(m_CommandBuffers.empty(), "All RenderCommandBuffers need to be destroy befor Renderer shuts down");

		Renderer::SubmitResourceFree([frequencyQuery = m_FrequencyQuery, factory = m_Factory, context = m_ImmediateContext, device = m_Device, debug = m_Debug, infoQueue = m_InfoQueue]()
		{
			frequencyQuery->Release();
			factory->Release();
			debug->Release();
			RT_LogMessages(infoQueue);
			infoQueue->Release();
			context->Release();
			device->Release();
		});

		m_GPUTimer = nullptr;
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
		Renderer::Submit([instance]()
		{
			instance->m_GPUTimer->RT_StartQuery(instance->m_ImmediateContext);
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
			instance->m_GPUTimer->RT_EndQuery(m_ImmediateContext);
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
			const UINT stride = vertexBuffer->m_Layout.GetVertexSize();
			ctx->IASetVertexBuffers(0, 1, &vertexBuffer->m_VertexBuffer, &stride, &offset);
			ctx->IASetIndexBuffer(indexBuffer->m_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
			ctx->IASetInputLayout(dxPipeline->m_InputLayout);

			ctx->RSSetState(dxPipeline->m_RasterizerState);
			ctx->OMSetDepthStencilState(dxPipeline->m_DepthStencilState, 0);

			Ref<DirectXFrameBuffer> dxFrameBuffer = dxPipeline->m_FrameBuffer;
			ctx->OMSetRenderTargets(dxFrameBuffer->m_Count, dxFrameBuffer->m_FrameBuffers.data(), dxFrameBuffer->m_DepthStencil);
			ctx->OMSetBlendState(dxFrameBuffer->m_BlendState, nullptr, 0xFFFFFFFF);
			ctx->RSSetViewports(1, &dxFrameBuffer->m_Viewport);

			instance->RT_PrepareAndBindMaterialForRendering(dxCommandBuffer, dxMaterial);

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
			const UINT stride = dxVB->m_Layout.GetVertexSize();
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
			instance->RT_PrepareAndBindMaterialForRendering(commandBuffer, dxMaterial);
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
			const UINT stride = dxVB->m_Layout.GetVertexSize();
			ctx->IASetVertexBuffers(0, 1, &dxVB->m_VertexBuffer, &stride, &offset);

			ctx->IASetIndexBuffer(dxIB->m_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);


			Ref<DirectXShader> dxShader = dxPipeline->m_Shader;

			ctx->VSSetShader(dxShader->m_VertexShader, nullptr, 0);
			ctx->PSSetShader(dxShader->m_PixelShader, nullptr, 0);

			ctx->IASetInputLayout(dxPipeline->m_InputLayout);

			instance->RT_PrepareAndBindMaterialForRendering(commandBuffer, dxMaterial);

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
			const UINT stride = dxVB->m_Layout.GetVertexSize();
			ctx->IASetVertexBuffers(0, 1, &dxVB->m_VertexBuffer, &stride, &offset);


			Ref<DirectXShader> dxShader = dxPipeline->m_Shader;

			ctx->VSSetShader(dxShader->m_VertexShader, nullptr, 0);
			ctx->PSSetShader(dxShader->m_PixelShader, nullptr, 0);

			ctx->IASetInputLayout(dxPipeline->m_InputLayout);

			instance->RT_PrepareAndBindMaterialForRendering(commandBuffer, dxMaterial);

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

	void DirectXRenderer::RenderSubmesh(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Mesh> mesh, uint32_t submeshIndex, Ref<Pipeline> pipeline)
	{
		SK_PROFILE_FUNCTION();

		Ref instance = this;
		Renderer::Submit([instance, commandBuffer = renderCommandBuffer.As<DirectXRenderCommandBuffer>(), mesh, submeshIndex, dxPipeline = pipeline.As<DirectXPipeline>()]()
		{
			SK_PROFILE_SCOPED("DirectXRenderer::RenderMesh");

			ID3D11DeviceContext* ctx = commandBuffer->GetContext();

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

			Ref<MaterialTable> materialTable = mesh->GetMaterialTable();
			Ref<DirectXMaterial> material = materialTable->GetMaterial(submesh.MaterialIndex).As<DirectXMaterial>();

			instance->RT_PrepareAndBindMaterialForRendering(commandBuffer, material);

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

	void DirectXRenderer::RenderSubmesh(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Mesh> mesh, uint32_t submeshIndex, Ref<ConstantBuffer> sceneData, Ref<ConstantBuffer> meshData, Ref<ConstantBuffer> lightData)
	{
		SK_PROFILE_FUNCTION();

		Ref instance = this;
		auto dxCommandBuffer = commandBuffer.As<DirectXRenderCommandBuffer>();
		auto dxPipeline = pipeline.As<DirectXPipeline>();
		auto cbSceneData = sceneData.As<DirectXConstantBuffer>();
		auto cbMeshData = meshData.As<DirectXConstantBuffer>();
		auto cbLight = lightData.As<DirectXConstantBuffer>();
		Renderer::Submit([instance, dxCommandBuffer, dxPipeline, mesh, submeshIndex, cbSceneData, cbMeshData, cbLight]()
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

			Ref<MaterialTable> materialTable = mesh->GetMaterialTable();
			Ref<MaterialTable> sourceMaterialTable = meshSource->GetMaterialTable();

			Ref<DirectXMaterial> material = materialTable->HasMaterial(submesh.MaterialIndex) ?
				                            materialTable->GetMaterial(submesh.MaterialIndex)->GetMaterial().As<DirectXMaterial>() :
				                            sourceMaterialTable->GetMaterial(submesh.MaterialIndex)->GetMaterial().As<DirectXMaterial>();

			instance->RT_PrepareAndBindMaterialForRendering(dxCommandBuffer, material);

			ctx->VSSetConstantBuffers(cbSceneData->GetBinding(), 1, &cbSceneData->m_ConstantBuffer);
			ctx->VSSetConstantBuffers(cbMeshData->GetBinding(), 1, &cbMeshData->m_ConstantBuffer);
			if (cbLight)
				ctx->PSSetConstantBuffers(cbLight->GetBinding(), 1, &cbLight->m_ConstantBuffer);

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

	void DirectXRenderer::RenderSubmeshWithMaterial(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Mesh> mesh, uint32_t submeshIndex, Ref<Material> material, Ref<ConstantBuffer> sceneData)
	{
		SK_PROFILE_FUNCTION();

		Ref instance = this;
		auto dxCommandBuffer = commandBuffer.As<DirectXRenderCommandBuffer>();
		auto dxPipeline = pipeline.As<DirectXPipeline>();
		auto dxMaterial = material.As<DirectXMaterial>();
		auto cbSceneData = sceneData.As<DirectXConstantBuffer>();
		Renderer::Submit([instance, dxCommandBuffer, dxPipeline, mesh, submeshIndex, dxMaterial, cbSceneData]()
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

			Ref<MaterialTable> materialTable = mesh->GetMaterialTable();
			Ref<MaterialTable> sourceMaterialTable = meshSource->GetMaterialTable();

			instance->RT_PrepareAndBindMaterialForRendering(dxCommandBuffer, dxMaterial);

			ctx->VSSetConstantBuffers(cbSceneData->GetBinding(), 1, &cbSceneData->m_ConstantBuffer);

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
		SK_CORE_VERIFY(Renderer::IsOnRenderThread());

		const auto& specs = image->GetSpecification();

		if (specs.MipLevels == 1)
			return;

		D3D11_TEXTURE2D_DESC textureDesc{};
		textureDesc.Width = specs.Width;
		textureDesc.Height = specs.Height;
		textureDesc.MipLevels = specs.MipLevels;
		textureDesc.ArraySize = 1;
		textureDesc.Format = utils::ImageFormatToD3D11ForResource(specs.Format);
		textureDesc.SampleDesc.Count = 1;
		textureDesc.Usage = D3D11_USAGE_DEFAULT;
		textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		textureDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

		ID3D11Texture2D* texture = nullptr;
		SK_DX11_CALL(m_Device->CreateTexture2D(&textureDesc, nullptr, &texture));
		if (!texture)
			return;

		D3D_SET_OBJECT_NAME_A(texture, "GenerateMips Texture");

		D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc{};
		viewDesc.Format = textureDesc.Format;
		viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		viewDesc.Texture2D.MipLevels = -1;
		viewDesc.Texture2D.MostDetailedMip = 0;
		ID3D11ShaderResourceView* view = nullptr;
		SK_DX11_CALL(m_Device->CreateShaderResourceView(texture, &viewDesc, &view));
		if (!view)
		{
			texture->Release();
			return;
		}

		D3D_SET_OBJECT_NAME_A(view, "GenerateMips View");

		auto dxImage = image.As<DirectXImage2D>();
		m_ImmediateContext->CopySubresourceRegion(texture, 0, 0, 0, 0, dxImage->m_Resource, 0, nullptr);
		m_ImmediateContext->GenerateMips(view);
		m_ImmediateContext->CopyResource(dxImage->m_Resource, texture);

		view->Release();
		texture->Release();
	}

	void DirectXRenderer::BindFrameBuffer(Ref<DirectXRenderCommandBuffer> commandBuffer, Ref<DirectXFrameBuffer> framebuffer)
	{
		auto context = commandBuffer->GetContext();
		context->OMSetRenderTargets(framebuffer->m_Count, framebuffer->m_FrameBuffers.data(), framebuffer->m_DepthStencil);
	}

	void DirectXRenderer::AddCommandBuffer(Weak<DirectXRenderCommandBuffer> commandBuffer)
	{
		m_CommandBuffers.insert(commandBuffer.Raw());
	}

	void DirectXRenderer::RemoveCommandBuffer(DirectXRenderCommandBuffer* commandBuffer)
	{
		auto entry = m_CommandBuffers.find(commandBuffer);
		if (entry != m_CommandBuffers.end())
			m_CommandBuffers.erase(entry);
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
		//SK_CORE_VERIFY(Renderer::IsOnRenderThread());
		for (auto context : m_CommandBuffers)
			context->RT_ClearState();
		m_ImmediateContext->ClearState();
		m_ImmediateContext->Flush();
	}

	void DirectXRenderer::ReportLiveObejcts()
	{
		IDXGIDebug1* debug;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug))))
		{
			debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);

			IDXGIInfoQueue* infoQueue;
			if (SUCCEEDED(debug->QueryInterface(IID_PPV_ARGS(&infoQueue))))
			{
				RT_LogMessages(infoQueue);
				infoQueue->Release();
			}
			debug->Release();
		}
	}

	void DirectXRenderer::RT_PrepareAndBindMaterialForRendering(Ref<DirectXRenderCommandBuffer> commandBuffer, Ref<DirectXMaterial> material)
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_VERIFY(Renderer::IsOnRenderThread());

		if (!material)
		{
			SK_CORE_ERROR_TAG("Renderer", "Tried to bind Material but Material is null");
			return;
		}

		ID3D11DeviceContext* context = commandBuffer->GetContext();

		material->RT_UpdateDirtyBuffers();
		for (const auto& [name, cbData] : material->m_ConstantBuffers)
		{
			Ref<DirectXConstantBuffer> constantBuffer = cbData.Buffer;
			if (!constantBuffer)
				continue;

			switch (cbData.Stage)
			{
				case ShaderReflection::ShaderStage::Vertex:
					context->VSSetConstantBuffers(constantBuffer->m_Binding, 1, &constantBuffer->m_ConstantBuffer);
					break;
			
				case ShaderReflection::ShaderStage::Pixel:
					context->PSSetConstantBuffers(constantBuffer->m_Binding, 1, &constantBuffer->m_ConstantBuffer);
					break;
			}
		}

		const auto bindShaderResourceView = [context](ShaderReflection::ShaderStage stage, uint32_t binding, ID3D11ShaderResourceView* view)
		{
			if (!view)
				return;

			switch (stage)
			{
				case ShaderReflection::ShaderStage::Vertex: context->VSSetShaderResources(binding, 1, &view); break;
				case ShaderReflection::ShaderStage::Pixel: context->PSSetShaderResources(binding, 1, &view); break;
			}
		};

		const auto bindSampler = [context](ShaderReflection::ShaderStage stage, uint32_t binding, ID3D11SamplerState* sampler)
		{
			if (!sampler)
				return;

			switch (stage)
			{
				case ShaderReflection::ShaderStage::Vertex: context->VSSetSamplers(binding, 1, &sampler); break;
				case ShaderReflection::ShaderStage::Pixel: context->PSSetSamplers(binding, 1, &sampler); break;
			}
		};

		Ref<DirectXTexture2D> whiteTexture = Renderer::GetWhiteTexture().As<DirectXTexture2D>();
		ID3D11ShaderResourceView* whiteTextureView = whiteTexture->GetViewNative();
		ID3D11SamplerState* whiteTextureSampler = whiteTexture->GetSamplerNative();

		for (const auto& [name, resource] : material->m_Resources)
		{
			switch (resource.Type)
			{
				case ShaderReflection::ResourceType::Texture2D:
					bindShaderResourceView(resource.Stage, resource.Binding, resource.Image ? resource.Image->m_View : whiteTextureView);
					break;

				case ShaderReflection::ResourceType::Sampler:
					bindSampler(resource.Stage, resource.Binding, resource.Sampler ? resource.Sampler->m_Sampler : whiteTextureSampler);
					break;

				case ShaderReflection::ResourceType::Sampler2D:
					bindShaderResourceView(resource.Stage, resource.Binding, resource.Image ? resource.Image->m_View : whiteTextureView);
					bindSampler(resource.Stage, resource.Binding, resource.Sampler ? resource.Sampler->m_Sampler : whiteTextureSampler);
					break;

				default:
					SK_CORE_ASSERT(false, "ResourceType {} not Implemented!", ToString(resource.Type));
					break;
			}
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