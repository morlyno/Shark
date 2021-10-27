#pragma once

#include "Shark/Render/RendererAPI.h"
#include "Platform/DirectX11/DirectXSwapChain.h"

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

		virtual void BeginRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<FrameBuffer> framebuffer) override;
		virtual void EndRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer) override;

		virtual void RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<FrameBuffer> frameBuffer, Ref<Shader> shaders, Ref<ConstantBufferSet> constantBufferSet, Ref<Texture2DArray> textureArray, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, uint32_t indexCount, PrimitveTopology topology) override;

		virtual Ref<FrameBuffer> GetFinaleCompositFrameBuffer() const override { return m_SwapChain->GetMainFrameBuffer(); }

		virtual void ResizeSwapChain(uint32_t width, uint32_t height) override { m_SwapChain->Resize(width, height); }
		virtual void SwapBuffers(bool vsync) override { m_SwapChain->SwapBuffers(vsync); }
		virtual void BindMainFrameBuffer() override;

		// Temp
		virtual void SetBlendForImgui(bool blend) override;

		Ref<DirectXSwapChain> GetSwapChain() const { return m_SwapChain; }
		static void AddRenderCommandBuffer(Weak<DirectXRenderCommandBuffer> commandBuffer);
		static void RemoveRenderCommandBuffer(Weak<DirectXRenderCommandBuffer> commandBuffer);
		void Flush();

	public:
		static Weak<DirectXRenderer>    Get()                  { return s_Instance; }
		static ID3D11Device*            GetDevice()            { return s_Instance->m_Device; }
		static ID3D11DeviceContext*     GetContext()           { return s_Instance->m_ImmediateContext; }
		static IDXGIFactory*            GetFactory()           { return s_Instance->m_Factory; }

	private:
		static DirectXRenderer* s_Instance;

		IDXGIFactory* m_Factory = nullptr;
		ID3D11Device* m_Device = nullptr;
		ID3D11DeviceContext* m_ImmediateContext = nullptr;
		Ref<DirectXSwapChain> m_SwapChain = nullptr;

		ID3D11BlendState* m_ImGuiBlendState = nullptr;

		std::vector<Ref<DirectXRenderCommandBuffer>> m_CommandBuffers;
	};

}