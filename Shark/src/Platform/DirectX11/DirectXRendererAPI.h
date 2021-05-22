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
		virtual void MainFrameBufferSetBlend(bool blend) override { m_SwapChain->GetMainFrameBuffer()->SetBlend(blend); }

		virtual void DrawIndexed(uint32_t count, uint32_t indexoffset, uint32_t vertexoffset) override;
		virtual void Flush() override;

		Ref<DirectXSwapChain> GetSwapChain() const { return m_SwapChain; }

	public:
		static DirectXRendererAPI&   Get()         { return *s_Instance; }
		static ID3D11Device*         GetDevice()   { return s_Instance->m_Device; }
		static ID3D11DeviceContext*  GetContext()  { return s_Instance->m_Context; }
		static IDXGIFactory*         GetFactory()  { return s_Instance->m_Factory; }

	private:
		static DirectXRendererAPI* s_Instance;

		IDXGIFactory* m_Factory = nullptr;
		ID3D11Device* m_Device = nullptr;
		ID3D11DeviceContext* m_Context = nullptr;

		Ref<DirectXSwapChain> m_SwapChain = nullptr;

	};

}