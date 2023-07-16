#pragma once

#include "Shark/Core/CommandQueue.h"
#include "Shark/Render/RendererAPI.h"
#include "Platform/DirectX11/DirectXBuffers.h"
#include "Platform/DirectX11/DirectXGPUTimer.h"

#include "Platform/DirectX11/DirectXMaterial.h"
#include "Platform/DirectX11/DirectXConstantBuffer.h"
#include "Platform/DirectX11/DirectXRenderCommandBuffer.h"
#include "Platform/DirectX11/DirectXSwapChain.h"

#include <set>
#include <d3d11.h>

#define SK_DX11_CALL(call) { HRESULT hr = (call); if (FAILED(hr)) { auto renderer = ::Shark::DirectXRenderer::Get(); renderer->HandleError(hr); } }
#define DX11_VERIFY(call_or_hresult)\
{\
	HRESULT _hresult = (call_or_hresult);\
	if (FAILED(_hresult))\
	{\
		auto renderer = ::Shark::DirectXRenderer::Get();\
		renderer->HandleError(_hresult);\
	}\
}

#define SK_ENABLE_INFOQUEUE SK_ENABLE_VALIDATION

struct IDXGIInfoQueue;

namespace Shark {

	class DirectXRenderer : public RendererAPI
	{
	public:
		DirectXRenderer();
		virtual ~DirectXRenderer();

		virtual void Init() override;
		virtual void ShutDown() override;

		virtual void BeginFrame() override;
		virtual void EndFrame() override;

		virtual void RenderFullScreenQuad(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Material> material) override;

		virtual void BeginBatch(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer) override;
		virtual void RenderBatch(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Material> material, Ref<ConstantBufferSet> constantBufferSet, uint32_t indexCount, uint32_t startIndex) override;
		virtual void EndBatch(Ref<RenderCommandBuffer> renderCommandBuffer) override;

		virtual void RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, Ref<ConstantBufferSet> constantBufferSet, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, uint32_t indexCount) override;
		virtual void RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, Ref<ConstantBufferSet> constantBufferSet, Ref<VertexBuffer> vertexBuffer, uint32_t vertexCount) override;

		virtual void RenderMesh(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Mesh> mesh, Ref<Pipeline> pipeline, Ref<ConstantBufferSet> constantBufferSet) override;

		virtual void GenerateMips(Ref<Image2D> image) override;
		virtual void RT_GenerateMips(Ref<Image2D> image) override;

		virtual const RendererCapabilities& GetCapabilities() const override { return m_Capabilities; }

		virtual Ref<ShaderLibrary> GetShaderLib() override { return m_ShaderLib; }
		virtual Ref<Texture2D> GetWhiteTexture() override { return m_WhiteTexture; }

		void AddCommandBuffer(Weak<DirectXRenderCommandBuffer> commandBuffer);
		void RemoveCommandBuffer(DirectXRenderCommandBuffer* commandBuffer);

		uint64_t GetGPUFrequncy() const { return m_GPUFrequency; }
		uint64_t HasValidFrequncy() const { return m_IsValidFrequency; }

		void HandleError(HRESULT hr);

		virtual bool IsInsideFrame() const override { return m_Active; }
		virtual bool ResourcesCreated() const override { return m_ResourceCreated; }
		void RT_FlushInfoQueue();
		void RT_PrepareForSwapchainResize();

		static void ReportLiveObejcts();

	private:
		void RT_PrepareAndBindMaterialForRendering(Ref<DirectXRenderCommandBuffer> renderCommandBuffer, Ref<DirectXMaterial> material, Ref<DirectXConstantBufferSet> constantBufferSet);
		void QueryCapabilities();

		void RT_BeginFrequencyQuery();
		void RT_EndFrequencyQuery();

		void RT_CreateDevice();
		void RT_CreateInfoQueue();
		void RT_FlushDXMessages();
		static void RT_LogMessages(IDXGIInfoQueue* infoQueue);

	public:
		static Ref<DirectXRenderer>     Get()                  { return s_Instance; }
		static ID3D11Device*            GetDevice()            { return s_Instance->m_Device; }
		static ID3D11DeviceContext*     GetContext()           { return s_Instance->m_ImmediateContext; }
		static IDXGIFactory*            GetFactory()           { return s_Instance->m_Factory; }
		ID3D11Debug* GetDebug() { return m_Debug; }

	private:
		static DirectXRenderer* s_Instance;

		bool m_ResourceCreated = false;
		bool m_Active = false;

		IDXGIFactory* m_Factory = nullptr;
		ID3D11Device* m_Device = nullptr;
		ID3D11DeviceContext* m_ImmediateContext = nullptr;
		ID3D11Debug* m_Debug = nullptr;
		IDXGIInfoQueue* m_InfoQueue = nullptr;

		std::unordered_set<DirectXRenderCommandBuffer*> m_CommandBuffers;

		Ref<ShaderLibrary> m_ShaderLib;
		Ref<Texture2D> m_WhiteTexture;

		Ref<DirectXVertexBuffer> m_QuadVertexBuffer;
		Ref<DirectXIndexBuffer> m_QuadIndexBuffer;

		ID3D11Query* m_FrequencyQuery = nullptr;
		uint64_t m_GPUFrequency = 0;
		bool m_IsValidFrequency = false;
		bool m_FrequencyQueryActive = false;
		bool m_DoFrequencyQuery = true;

		RendererCapabilities m_Capabilities;
	};


}