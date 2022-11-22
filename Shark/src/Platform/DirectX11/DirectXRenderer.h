#pragma once

#include "Shark/Render/RendererAPI.h"
#include "Platform/DirectX11/DirectXBuffers.h"
#include "Platform/DirectX11/DirectXGPUTimer.h"

#include "Platform/DirectX11/DirectXMaterial.h"
#include "Platform/DirectX11/DirectXConstantBuffer.h"
#include "Platform/DirectX11/DirectXRenderCommandBuffer.h"

#include <set>
#include <d3d11.h>

#define SK_DX11_CALL(call) { HRESULT hr = (call); if (FAILED(hr)) { auto renderer = ::Shark::DirectXRenderer::Get(); renderer->HandleError(hr); } }

namespace Shark {

	class DirectXRenderer : public RendererAPI
	{
	public:
		DirectXRenderer();
		virtual ~DirectXRenderer();

		virtual void Init() override;
		virtual void ShutDown() override;

		virtual void NewFrame() override;

		virtual void RenderFullScreenQuad(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Material> material) override;

		virtual void RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, Ref<ConstantBufferSet> constantBufferSet, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, uint32_t indexCount) override;
		virtual void RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, Ref<ConstantBufferSet> constantBufferSet, Ref<VertexBuffer> vertexBuffer, uint32_t vertexCount) override;

		virtual void GenerateMips(Ref<Image2D> image) override;
		void GenerateMips(DirectXImage2D* image);

		virtual void ClearAllCommandBuffers() override;
		virtual const RendererCapabilities& GetCapabilities() const override { return m_Capabilities; }

		virtual Ref<ShaderLibrary> GetShaderLib() override { return m_ShaderLib; }
		virtual Ref<Texture2D> GetWhiteTexture() override { return m_WhiteTexture; }

		void AddCommandBuffer(const Weak<DirectXRenderCommandBuffer>& commandBuffer);
		void RemoveCommandBuffer(const Weak<DirectXRenderCommandBuffer>& commandBuffer);

		uint64_t GetGPUFrequncy() const { return m_GPUFrequency; }
		uint64_t HasValidFrequncy() const { return m_IsValidFrequency; }

		void HandleError(HRESULT hr);

	private:
		void PrepareAndBindMaterialForRendering(Ref<DirectXRenderCommandBuffer> renderCommandBuffer, Ref<DirectXMaterial> material, Ref<DirectXConstantBufferSet> constantBufferSet);
		void QueryCapabilities();

	public:
		static Ref<DirectXRenderer>     Get()                  { return s_Instance; }
		static ID3D11Device*            GetDevice()            { return s_Instance->m_Device; }
		static ID3D11DeviceContext*     GetContext()           { return s_Instance->m_ImmediateContext; }
		static IDXGIFactory*            GetFactory()           { return s_Instance->m_Factory; }

	private:
		static DirectXRenderer* s_Instance;

		bool m_IsFirstFrame = true;

		IDXGIFactory* m_Factory = nullptr;
		ID3D11Device* m_Device = nullptr;
		ID3D11DeviceContext* m_ImmediateContext = nullptr;

		std::unordered_set<DirectXRenderCommandBuffer*> m_CommandBuffers;

		Ref<ShaderLibrary> m_ShaderLib;
		Ref<Texture2D> m_WhiteTexture;

		Ref<DirectXVertexBuffer> m_QuadVertexBuffer;
		Ref<DirectXIndexBuffer> m_QuadIndexBuffer;

		ID3D11Query* m_FrequencyQuery = nullptr;
		uint64_t m_GPUFrequency = 0;
		bool m_IsValidFrequency = false;
		bool m_FrequencyQueryActive = false;

		bool m_NeedsResize = false;
		uint32_t m_WindowWidth = 0, m_WindowHeight = 0;

		RendererCapabilities m_Capabilities;

	};


}