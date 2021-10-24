#pragma once

#include "Shark/Render/RendererAPI.h"
#include "Platform/DirectX11/DirectXSwapChain.h"
#include <d3d11.h>

namespace Shark {

	class DirectXRendererAPI : public RendererAPI
	{
	public:
		virtual void Init() override;
		virtual void ShutDown() override;

		virtual void ResizeSwapChain(uint32_t width, uint32_t height) override { m_SwapChain->Resize(width, height); }
		virtual void SwapBuffers(bool vsync) override { m_SwapChain->SwapBuffers(vsync); }
		virtual void BindMainFrameBuffer() override { m_SwapChain->Bind(); }

		// Temp
		virtual void SetBlendForImgui(bool blend) override;

		virtual void Draw(uint32_t vertexCount, PrimitveTopology topology) override;
		virtual void DrawIndexed(uint32_t indexCount, PrimitveTopology topology) override;

		Ref<DirectXSwapChain> GetSwapChain() const { return m_SwapChain; }

		void SetActiveContext(ID3D11DeviceContext* ctx) { m_ActiveContext = ctx; }

	public:
		static Weak<DirectXRendererAPI> Get()                  { return s_Instance; }
		static ID3D11Device*            GetDevice()            { return s_Instance->m_Device; }
		static ID3D11DeviceContext*     GetContext()           { return s_Instance->m_ActiveContext; }
		static ID3D11DeviceContext*     GetImmediateContext()  { return s_Instance->m_ImmediateContext; }
		static IDXGIFactory*            GetFactory()           { return s_Instance->m_Factory; }

	private:
		static DirectXRendererAPI* s_Instance;

		IDXGIFactory* m_Factory = nullptr;
		ID3D11Device* m_Device = nullptr;
		ID3D11DeviceContext* m_ImmediateContext = nullptr;
		ID3D11DeviceContext* m_ActiveContext = nullptr;
		Ref<DirectXSwapChain> m_SwapChain = nullptr;

		ID3D11BlendState* m_ImGuiBlendState = nullptr;
	};

}