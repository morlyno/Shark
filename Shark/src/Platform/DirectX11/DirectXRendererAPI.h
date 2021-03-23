#pragma once

#include "Shark/Render/RendererAPI.h"
#include "Shark/Core/Window.h"
#include <d3d11.h>

namespace Shark {

	struct APIContext
	{
		ID3D11Device* Device;
		ID3D11DeviceContext* Context;

		APIContext() = delete;
		APIContext(ID3D11Device* device, ID3D11DeviceContext* context)
			: Device(device), Context(context) {}
		~APIContext() { /*Device->Release(); Context->Release();*/ }
	};

	class DirectXRendererAPI : public RendererAPI
	{
	public:
		virtual void Init(const Window& window) override;
		virtual void ShutDown() override;

		virtual void SwapBuffer(bool VSync) override;

		virtual void SetBlendState(bool blend) override;

		virtual void DrawIndexed(uint32_t count) override;
		virtual void Flush() override;

		ID3D11Device* GetDevice() { return m_Device; }
		ID3D11DeviceContext* GetContext() { return m_Context; }

		virtual Ref<VertexBuffer> CreateVertexBuffer(const VertexLayout& layout, bool dynamic = false) override;
		virtual Ref<VertexBuffer> CreateVertexBuffer(const VertexLayout& layout, void* data, uint32_t size, bool dynamic = false) override;
		virtual Ref<IndexBuffer> CreateIndexBuffer(uint32_t* indices, uint32_t count) override;
		virtual Ref<Shaders> CreateShaders(const std::string& filepath) override;
		virtual Ref<Shaders> CreateShaders(const std::string& vertexshaderSrc, const std::string& pixelshaderSrc) override;
		virtual Ref<Texture2D> CreateTexture2D(const SamplerSpecification& sampler, const std::string& filepath) override;
		virtual Ref<Texture2D> CreateTexture2D(const SamplerSpecification& sampler, uint32_t width, uint32_t height, uint32_t flatcolor) override;
		virtual Ref<Texture2D> CreateTexture2D(const SamplerSpecification& sampler, uint32_t width, uint32_t height, void* data) override;
		virtual Ref<FrameBuffer> CreateFrameBuffer(const FrameBufferSpecification& specs) override;
		virtual Ref<Viewport> CreateViewport(uint32_t width, uint32_t height) override;
	private:
		IDXGIFactory* m_Factory = nullptr;
		ID3D11Device* m_Device = nullptr;
		ID3D11DeviceContext* m_Context = nullptr;
		IDXGISwapChain* m_SwapChain = nullptr;
		ID3D11BlendState* m_BlendState = nullptr;
		ID3D11BlendState* m_BlendStateNoAlpha = nullptr;
	};

}