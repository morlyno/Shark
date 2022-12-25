#include "skpch.h"
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

		Renderer::Submit([instance]()
		{
			SK_DX11_CALL(CreateDXGIFactory(IID_PPV_ARGS(&instance->m_Factory)));

			IDXGIAdapter* adapter = nullptr;
			SK_DX11_CALL(instance->m_Factory->EnumAdapters(0, &adapter));
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
				&instance->m_Device,
				nullptr,
				&instance->m_ImmediateContext
			));

			adapter->Release();

			instance->QueryCapabilities();
		});

		m_ShaderLib = Ref<ShaderLibrary>::Create();

		m_ShaderLib->Load("Resources/Shaders/Renderer2D_Quad.hlsl");
		m_ShaderLib->Load("Resources/Shaders/Renderer2D_QuadDepthPass.hlsl");
		m_ShaderLib->Load("Resources/Shaders/Renderer2D_Circle.hlsl");
		m_ShaderLib->Load("Resources/Shaders/Renderer2D_CircleDepthPass.hlsl");
		m_ShaderLib->Load("Resources/Shaders/Renderer2D_Line.hlsl");
		m_ShaderLib->Load("Resources/Shaders/Renderer2D_LineDepthPass.hlsl");

		m_ShaderLib->Load("Resources/Shaders/FullScreen.hlsl");
		m_ShaderLib->Load("Resources/Shaders/CompositWidthDepth.hlsl");
		m_ShaderLib->Load("Resources/Shaders/NegativeEffect.hlsl");
		m_ShaderLib->Load("Resources/Shaders/BlurEffect.hlsl");

		uint32_t color = 0xFFFFFFFF;
		m_WhiteTexture = Texture2D::Create(ImageFormat::RGBA8, 1, 1, Buffer::FromValue(color));

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

		Renderer::Submit([instance]()
		{
			// Frequency Query
			D3D11_QUERY_DESC queryDesc;
			queryDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
			queryDesc.MiscFlags = 0;
			SK_DX11_CALL(instance->m_Device->CreateQuery(&queryDesc, &instance->m_FrequencyQuery));
		});

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

		Renderer::SubmitResourceFree([frequencyQuery = m_FrequencyQuery, factory = m_Factory, context = m_ImmediateContext, device = m_Device]()
		{
			if (frequencyQuery)
				frequencyQuery->Release();
			if (factory)
				factory->Release();
			if (context)
				context->Release();
			if (device)
				device->Release();
		});

		m_Swapchain = nullptr;
		m_FrequencyQuery = nullptr;
		m_Factory = nullptr;
		m_ImmediateContext = nullptr;
		m_Device = nullptr;

		Renderer::WaitAndRender();
		s_Instance = nullptr;
	}

	void DirectXRenderer::BeginFrame()
	{
		m_Active = true;

		if (m_SwapchainNeedsResize)
		{
			//m_Swapchain->m_Specs.Widht = m_Width;
			//m_Swapchain->m_Specs.Height = m_Height;
			//m_Swapchain->ReCreateSwapChain();
			ClearAllCommandBuffers();
			m_Swapchain->Resize(m_Width, m_Height);
			m_SwapchainNeedsResize = false;
		}

		Ref<DirectXRenderer> instance = this;
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

		m_Active = false;
	}

	void DirectXRenderer::ResizeSwapChain(uint32_t widht, uint32_t height)
	{
		//RT_ClearAllCommandBuffers();

		m_Width = widht;
		m_Height = height;
		m_SwapchainNeedsResize = true;

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

			instance->RT_PrepareAndBindMaterialForRendering(dxCommandBuffer, dxMaterial, nullptr);

			ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			ctx->DrawIndexed(6, 0, 0);
		});
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

	void DirectXRenderer::RT_GenerateMips(Ref<DirectXImage2D> image)
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

		m_ImmediateContext->CopySubresourceRegion(texture, 0, 0, 0, 0, image->m_Resource, 0, nullptr);
		m_ImmediateContext->GenerateMips(view);
		m_ImmediateContext->CopyResource(image->m_Resource, texture);

		view->Release();
		texture->Release();
	}

	void DirectXRenderer::ClearAllCommandBuffers()
	{
		SK_PROFILE_FUNCTION();

		Ref<DirectXRenderer> instance = this;
		Renderer::Submit([instance]()
		{
			instance->RT_ClearAllCommandBuffers();
		});
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

		SK_CORE_ERROR_TAG("Renderer", WindowsUtils::TranslateHResult(hr));
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
		SK_CORE_VERIFY(Renderer::IsOnRenderThread());

		if (m_DoFrequencyQuery)
			m_ImmediateContext->Begin(m_FrequencyQuery);
	}

	void DirectXRenderer::RT_EndFrequencyQuery()
	{
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

	void DirectXRenderer::RT_ClearAllCommandBuffers()
	{
		for (auto& c : m_CommandBuffers)
			c->ClearState();
		m_ImmediateContext->ClearState();
	}

	void DirectXRenderer::RT_ResizeSwapChain()
	{
		SK_CORE_VERIFY(Renderer::IsOnRenderThread());

		for (const auto& c : m_CommandBuffers)
		{
			c->ClearState();
		}
		m_ImmediateContext->Flush();
		m_ImmediateContext->ClearState();

		//dxSwapchain->RT_ResizeSwapChain(widht, height);
		m_Swapchain->m_Specs.Widht = m_Width;
		m_Swapchain->m_Specs.Height = m_Height;
		m_Swapchain->ReCreateSwapChain();
	}

}