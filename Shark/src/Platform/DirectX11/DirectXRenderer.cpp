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
#include <dxgidebug.h>

#if SK_ENABLE_VALIDATION
#define DX11_VALIDATE_CONTEXT(ctx) DirectXRenderer::Get()->GetDebug()->ValidateContext(ctx);
#endif

namespace Shark {

	DirectXRenderer* DirectXRenderer::s_Instance = nullptr;

	namespace Utils {

		static std::string GetGPUDescription(IDXGIAdapter* adapter)
		{
			DXGI_ADAPTER_DESC ad = {};
			SK_DX11_CALL(adapter->GetDesc(&ad));
			return String::ToNarrowCopy(std::wstring_view(ad.Description));
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
			RT_CreateInfoQueue();
			RT_CreateDevice();
			QueryCapabilities();
			SK_DX11_CALL(m_Device->QueryInterface(&m_Debug));


			D3D11_QUERY_DESC queryDesc;
			queryDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
			queryDesc.MiscFlags = 0;
			SK_DX11_CALL(instance->m_Device->CreateQuery(&queryDesc, &instance->m_FrequencyQuery));
		});

		m_ShaderLib = Ref<ShaderLibrary>::Create();
		m_ShaderLib->Load("Resources/Shaders/Renderer2D_Quad.hlsl");
		m_ShaderLib->Load("Resources/Shaders/Renderer2D_QuadTransparent.hlsl");
		m_ShaderLib->Load("Resources/Shaders/Renderer2D_QuadDepthPass.hlsl");
		m_ShaderLib->Load("Resources/Shaders/Renderer2D_Circle.hlsl");
		m_ShaderLib->Load("Resources/Shaders/Renderer2D_CircleTransparent.hlsl");
		m_ShaderLib->Load("Resources/Shaders/Renderer2D_CircleDepthPass.hlsl");
		m_ShaderLib->Load("Resources/Shaders/Renderer2D_Line.hlsl");
		m_ShaderLib->Load("Resources/Shaders/Renderer2D_LineDepthPass.hlsl");
		m_ShaderLib->Load("Resources/Shaders/Renderer2D_Composite.hlsl");
		m_ShaderLib->Load("Resources/Shaders/Renderer2D_Text.hlsl");

		m_ShaderLib->Load("Resources/Shaders/FullScreen.hlsl");
		m_ShaderLib->Load("Resources/Shaders/CompositWidthDepth.hlsl");
		m_ShaderLib->Load("Resources/Shaders/NegativeEffect.hlsl");
		m_ShaderLib->Load("Resources/Shaders/BlurEffect.hlsl");

		m_WhiteTexture = Texture2D::Create(ImageFormat::RGBA8, 1, 1, Buffer::FromValue(0xFFFFFFFF));

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

		Renderer::Submit([instance]() { SK_CORE_INFO_TAG("Renderer", "Resources Created"); instance->m_ResourceCreated = true; });
	}

	void DirectXRenderer::ShutDown()
	{
		SK_PROFILE_FUNCTION();

		m_ShaderLib = nullptr;
		m_WhiteTexture = nullptr;
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

		m_FrequencyQuery = nullptr;
		m_Factory = nullptr;
		m_InfoQueue = nullptr;
		m_Debug = nullptr;
		m_ImmediateContext = nullptr;
		m_Device = nullptr;

		Renderer::WaitAndRender();
		s_Instance = nullptr;
	}

	void DirectXRenderer::BeginFrame()
	{
		m_Active = true;

		Ref<DirectXRenderer> instance = this;
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
			RT_LogMessages(m_InfoQueue);
		});

		m_Active = false;
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
			ctx->IASetInputLayout(dxShader->m_InputLayout);

			ctx->RSSetState(dxPipeline->m_RasterizerState);
			ctx->OMSetDepthStencilState(dxPipeline->m_DepthStencilState, 0);

			Ref<DirectXFrameBuffer> dxFrameBuffer = dxPipeline->m_FrameBuffer;
			ctx->OMSetRenderTargets(dxFrameBuffer->m_Count, dxFrameBuffer->m_FrameBuffers.data(), dxFrameBuffer->m_DepthStencil);
			ctx->OMSetBlendState(dxFrameBuffer->m_BlendState, nullptr, 0xFFFFFFFF);
			ctx->RSSetViewports(1, &dxFrameBuffer->m_Viewport);

			for (const auto& [name, binding] : dxMaterial->m_BindingMap)
			{
				auto image = dxMaterial->m_ImageMap.at(name);
				auto view = image->GetViewNative();
				ctx->PSSetShaderResources(binding, 1, &view);
			}

			//instance->RT_PrepareAndBindMaterialForRendering(dxCommandBuffer, dxMaterial, nullptr);

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

			// TODO(moro): move InputLayout to Pipeline
			ctx->IASetInputLayout(dxShader->m_InputLayout);

			Ref<DirectXFrameBuffer> dxFrameBuffer = dxPipeline->m_FrameBuffer;

			ctx->OMSetRenderTargets(dxFrameBuffer->m_Count, dxFrameBuffer->m_FrameBuffers.data(), dxFrameBuffer->m_DepthStencil);
			ctx->RSSetViewports(1, &dxFrameBuffer->m_Viewport);

			ctx->RSSetState(dxPipeline->m_RasterizerState);
			ctx->OMSetDepthStencilState(dxPipeline->m_DepthStencilState, 0);
			ctx->OMSetBlendState(dxFrameBuffer->m_BlendState, nullptr, 0xFFFFFFFF);

			ctx->IASetPrimitiveTopology(dxPipeline->m_PrimitveTopology);
		});
	}

	void DirectXRenderer::RenderBatch(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Material> material, Ref<ConstantBufferSet> constantBufferSet, uint32_t indexCount, uint32_t startIndex)
	{
		Ref<DirectXRenderCommandBuffer> commandBuffer = renderCommandBuffer.As<DirectXRenderCommandBuffer>();
		Ref<DirectXMaterial> dxMaterial = material.As<DirectXMaterial>();
		Ref<DirectXConstantBufferSet> dxCBSet = constantBufferSet.As<DirectXConstantBufferSet>();
		Ref<DirectXRenderer> instance = this;
		Renderer::Submit([instance, commandBuffer, dxMaterial, dxCBSet, indexCount, startIndex]()
		{
			SK_PROFILE_SCOPED("DirectXRenderer::RenderBatch");

			ID3D11DeviceContext* ctx = commandBuffer->GetContext();
			instance->RT_PrepareAndBindMaterialForRendering(commandBuffer, dxMaterial, dxCBSet);
			DX11_VALIDATE_CONTEXT(ctx);
			ctx->DrawIndexed(indexCount, startIndex, 0);
		});
	}

	void DirectXRenderer::EndBatch(Ref<RenderCommandBuffer> renderCommandBuffer)
	{
	}

	void DirectXRenderer::RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, Ref<ConstantBufferSet> constantBufferSet, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, uint32_t indexCount)
	{
		SK_PROFILE_FUNCTION();

		Ref<DirectXRenderCommandBuffer> commandBuffer = renderCommandBuffer.As<DirectXRenderCommandBuffer>();
		Ref<DirectXVertexBuffer> dxVB = vertexBuffer.As<DirectXVertexBuffer>();
		Ref<DirectXIndexBuffer> dxIB = indexBuffer.As<DirectXIndexBuffer>();
		Ref<DirectXPipeline> dxPipeline = pipeline.As<DirectXPipeline>();
		Ref<DirectXMaterial> dxMaterial = material.As<DirectXMaterial>();
		Ref<DirectXConstantBufferSet> dxCBSet = constantBufferSet.As<DirectXConstantBufferSet>();
		Ref<DirectXRenderer> instance = this;
		Renderer::Submit([instance, commandBuffer, dxVB, dxIB, dxPipeline, dxMaterial, dxCBSet, indexCount]()
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

			// TODO(moro): move InputLayout to Pipeline
			ctx->IASetInputLayout(dxShader->m_InputLayout);

			instance->RT_PrepareAndBindMaterialForRendering(commandBuffer, dxMaterial, dxCBSet);

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

	void DirectXRenderer::RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, Ref<ConstantBufferSet> constantBufferSet, Ref<VertexBuffer> vertexBuffer, uint32_t vertexCount)
	{
		SK_PROFILE_FUNCTION();

		Ref<DirectXRenderCommandBuffer> commandBuffer = renderCommandBuffer.As<DirectXRenderCommandBuffer>();
		Ref<DirectXVertexBuffer> dxVB = vertexBuffer.As<DirectXVertexBuffer>();
		Ref<DirectXPipeline> dxPipeline = pipeline.As<DirectXPipeline>();
		Ref<DirectXMaterial> dxMaterial = material.As<DirectXMaterial>();
		Ref<DirectXConstantBufferSet> dxCBSet = constantBufferSet.As<DirectXConstantBufferSet>();
		Ref<DirectXRenderer> instance = this;
		Renderer::Submit([instance, commandBuffer, dxVB, dxPipeline, dxMaterial, dxCBSet, vertexCount]()
		{
			SK_PROFILE_SCOPED("DirectXRenderer::RenderGeometry");

			ID3D11DeviceContext* ctx = commandBuffer->GetContext();

			const UINT offset = 0;
			const UINT stride = dxVB->m_Layout.GetVertexSize();
			ctx->IASetVertexBuffers(0, 1, &dxVB->m_VertexBuffer, &stride, &offset);


			Ref<DirectXShader> dxShader = dxPipeline->m_Shader;

			ctx->VSSetShader(dxShader->m_VertexShader, nullptr, 0);
			ctx->PSSetShader(dxShader->m_PixelShader, nullptr, 0);

			// TODO(moro): move InputLayout to Pipeline
			ctx->IASetInputLayout(dxShader->m_InputLayout);

			instance->RT_PrepareAndBindMaterialForRendering(commandBuffer, dxMaterial, dxCBSet);

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

		auto dxImage = image.As<DirectXImage2D>();
		m_ImmediateContext->CopySubresourceRegion(texture, 0, 0, 0, 0, dxImage->m_Resource, 0, nullptr);
		m_ImmediateContext->GenerateMips(view);
		m_ImmediateContext->CopyResource(dxImage->m_Resource, texture);

		view->Release();
		texture->Release();
	}

	void DirectXRenderer::AddCommandBuffer(const Weak<DirectXRenderCommandBuffer>& commandBuffer)
	{
		m_CommandBuffers.insert(commandBuffer.Raw());
	}

	void DirectXRenderer::RemoveCommandBuffer(const Weak<DirectXRenderCommandBuffer>& commandBuffer)
	{
		auto entry = m_CommandBuffers.find(commandBuffer.Raw());
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

	void DirectXRenderer::RT_PrepareAndBindMaterialForRendering(Ref<DirectXRenderCommandBuffer> renderCommandBuffer, Ref<DirectXMaterial> material, Ref<DirectXConstantBufferSet> constantBufferSet)
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_VERIFY(Renderer::IsOnRenderThread());

		ID3D11DeviceContext* ctx = renderCommandBuffer->GetContext();

		for (auto&& [name, textureArray] : material->m_ResourceMap)
		{
			ctx->PSSetShaderResources(textureArray->m_StartOffset, textureArray->m_Count, textureArray->m_Views.data());
			ctx->PSSetSamplers(textureArray->m_StartOffset, textureArray->m_Count, textureArray->m_Samplers.data());
		}

		for (auto&& [slot, data] : material->m_ConstantBufferData)
			material->m_ConstnatBufferSet->RT_Set(slot, Buffer(data.Data(), data.Size())); // TODO: maby convert to uint64_t

		if (constantBufferSet)
		{
			auto& cbMap = constantBufferSet->m_CBMap;
			Ref<DirectXConstantBufferSet> matCBSet = material->m_ConstnatBufferSet;
			for (auto& [slot, cb] : matCBSet->m_CBMap)
			{
				const auto it = cbMap.find(slot);
				Ref<DirectXConstantBuffer> c = (it != cbMap.end()) ? it->second : cb;
				ctx->VSSetConstantBuffers(c->m_Slot, 1, &c->m_ConstBuffer);
			}
		}
		else
		{
			Ref<DirectXConstantBufferSet> matCBSet = material->m_ConstnatBufferSet;
			for (auto& [slot, cb] : matCBSet->m_CBMap)
				ctx->VSSetConstantBuffers(cb->m_Slot, 1, &cb->m_ConstBuffer);
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

		SK_DX11_CALL(CreateDXGIFactory(IID_PPV_ARGS(&m_Factory)));

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