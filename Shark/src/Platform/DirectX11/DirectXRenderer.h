#pragma once

#include "Shark/Render/RendererAPI.h"
#include "Platform/DirectX11/DirectXSwapChain.h"
#include "Platform/DirectX11/DirectXBuffers.h"
#include "Platform/DirectX11/DirectXGPUTimer.h"

#include "Platform/DirectX11/DirectXMaterial.h"
#include "Platform/DirectX11/DirectXConstantBuffer.h"

#include <set>
#include <d3d11.h>

namespace Shark {

	class DirectXRenderer : public RendererAPI
	{
	public:
		DirectXRenderer();
		virtual ~DirectXRenderer();

		virtual void Init() override;
		virtual void ShutDown() override;

		virtual void NewFrame() override;

		virtual void RenderFullScreenQuad(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Image2D> image) override;
		virtual void RenderFullScreenQuadWidthDepth(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Image2D> image, Ref<Image2D> depthImage) override;

		virtual void RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<FrameBuffer> frameBuffer, Ref<Shader> shaders, Ref<ConstantBufferSet> constantBufferSet, Ref<Texture2DArray> textureArray, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, uint32_t indexCount, PrimitveTopology topology) override;
		virtual void RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<ConstantBufferSet> constantBufferSet, Ref<Texture2DArray> textureArray, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, uint32_t indexCount, PrimitveTopology topology) override;
		virtual void RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<ConstantBufferSet> constantBufferSet, Ref<Texture2DArray> textureArray, Ref<VertexBuffer> vertexBuffer, uint32_t vertexCount, PrimitveTopology topology) override;
		virtual void RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, Ref<ConstantBufferSet> constantBufferSet, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, uint32_t indexCount, PrimitveTopology topology) override;

		virtual Ref<ShaderLibrary> GetShaderLib() override { return m_ShaderLib; }
		virtual Ref<Texture2D> GetWhiteTexture() override { return m_WhiteTexture; }
		virtual Ref<GPUTimer> GetPresentTimer() override { return m_PresentTimer; }

		Ref<FrameBuffer> GetFinaleCompositFrameBuffer() const { return m_SwapChain->GetMainFrameBuffer(); }

		virtual void ResizeSwapChain(uint32_t width, uint32_t height) override { m_SwapChainNeedsResize = true; m_WindowWidth = width; m_WindowHeight = height; }
		virtual void Present(bool vsync) override;
		virtual void BindMainFrameBuffer() override;


		Ref<DirectXSwapChain> GetSwapChain() const { return m_SwapChain; }
		static void AddRenderCommandBuffer(Weak<DirectXRenderCommandBuffer> commandBuffer);
		static void RemoveRenderCommandBuffer(Weak<DirectXRenderCommandBuffer> commandBuffer);

		uint64_t GetGPUFrequncy() const { return m_GPUFrequency; }
		uint64_t HasValidFrequncy() const { return m_IsValidFrequency; }

		void Flush();

		void PrepareAndBindMaterialForRendering(Ref<DirectXRenderCommandBuffer> renderCommandBuffer, Ref<DirectXMaterial> material, Ref<DirectXConstantBufferSet> constantBufferSet);

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
		Ref<DirectXSwapChain> m_SwapChain = nullptr;

		std::vector<Ref<DirectXRenderCommandBuffer>> m_CommandBuffers;

		Ref<ShaderLibrary> m_ShaderLib;
		Ref<Texture2D> m_WhiteTexture;

		Ref<DirectXVertexBuffer> m_QuadVertexBuffer;
		Ref<DirectXIndexBuffer> m_QuadIndexBuffer;

		ID3D11SamplerState* m_ClampSampler;

		ID3D11Query* m_FrequencyQuery = nullptr;
		uint64_t m_GPUFrequency = 0;
		bool m_IsValidFrequency = false;
		bool m_FrequencyQueryActive = false;

		bool m_SwapChainNeedsResize = false;
		uint32_t m_WindowWidth = 0, m_WindowHeight = 0;

		Ref<DirectXGPUTimer> m_PresentTimer;

	};


}