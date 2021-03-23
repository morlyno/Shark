#pragma once

#include "Shark/Core/Base.h"
#include "RendererAPI.h"

namespace Shark {

	class RendererCommand
	{
	public:
		static inline void Init(const class Window& window) { s_RendererAPI->Init(window); }
		static inline void ShutDown() { s_RendererAPI->ShutDown(); }

		static inline void SwapBuffer(bool VSync) { s_RendererAPI->SwapBuffer(VSync); }

		static inline void SetBlendState(bool blend) { s_RendererAPI->SetBlendState(blend); }

		static inline void DrawIndexed(uint32_t count) { s_RendererAPI->DrawIndexed(count); }

		static inline RendererAPI& GetRendererAPI() { return *s_RendererAPI; }

		static inline Ref<VertexBuffer> CreateVertexBuffer(const VertexLayout& layout, bool dynamic = false) { return s_RendererAPI->CreateVertexBuffer(layout, dynamic); }
		static inline Ref<VertexBuffer> CreateVertexBuffer(const VertexLayout& layout, void* data, uint32_t size, bool dynamic = false) { return s_RendererAPI->CreateVertexBuffer(layout, data, size, dynamic); }
		static inline Ref<IndexBuffer> CreateIndexBuffer(uint32_t* indices, uint32_t count) { return s_RendererAPI->CreateIndexBuffer(indices, count); }
		static inline Ref<Shaders> CreateShaders(const std::string& filepath) { return s_RendererAPI->CreateShaders(filepath); }
		static inline Ref<Shaders> CreateShaders(const std::string& vertexshaderSrc, const std::string& pixelshaderSrc) { return s_RendererAPI->CreateShaders(vertexshaderSrc, pixelshaderSrc); }
		static inline Ref<Texture2D> CreateTexture2D(const SamplerSpecification& sampler, const std::string& filepath) { return s_RendererAPI->CreateTexture2D(sampler, filepath); }
		static inline Ref<Texture2D> CreateTexture2D(const SamplerSpecification& sampler, uint32_t width, uint32_t height, uint32_t flatcolor) { return s_RendererAPI->CreateTexture2D(sampler, width, height, flatcolor); }
		static inline Ref<Texture2D> CreateTexture2D(const SamplerSpecification& sampler, uint32_t width, uint32_t height, void* data) { return s_RendererAPI->CreateTexture2D(sampler, width, height, data); }
		static inline Ref<FrameBuffer> CreateFrameBuffer(const FrameBufferSpecification& specs) { return s_RendererAPI->CreateFrameBuffer(specs); }
		static inline Ref<Viewport> CreateViewport(uint32_t width, uint32_t height) { return s_RendererAPI->CreateViewport(width, height); }
	private:
		static Scope<RendererAPI> s_RendererAPI;
	};

}