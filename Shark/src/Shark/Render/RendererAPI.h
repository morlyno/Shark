#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/Buffers.h"
#include "Shark/Render/Shaders.h"
#include "Shark/Render/Texture.h"
#include "Shark/Render/FrameBuffer.h"
#include "Shark/Render/Viewport.h"
#include "Shark/Render/SwapChain.h"

namespace Shark {

	class RendererAPI
	{
	public:
		enum class API
		{
			None = 0, DirectX11 = 1
		};
	public:
		virtual ~RendererAPI() = default;

		virtual void Init(const class Window& window) = 0;
		virtual void ShutDown() = 0;
		
		virtual void DrawIndexed(uint32_t count) = 0;
		virtual void Flush() = 0;

		// Temp ///////////////////////////////////
		virtual void SetBlendState(bool blend) = 0;
		///////////////////////////////////////////

		static API GetAPI() { return s_API; }

		static Scope<RendererAPI> Create();

		virtual Ref<VertexBuffer> CreateVertexBuffer(const VertexLayout& layout, const Buffer& data, bool dynamic = false) = 0;
		virtual Ref<IndexBuffer> CreateIndexBuffer(const Buffer& data, bool dynamic) = 0;
		virtual Ref<Shaders> CreateShaders(const std::string& filepath) = 0;
		virtual Ref<Shaders> CreateShaders(const std::string& vertexshaderSrc, const std::string& pixelshaderSrc) = 0;
		virtual Ref<Texture2D> CreateTexture2D(const SamplerSpecification& sampler, const std::string& filepath) = 0;
		virtual Ref<Texture2D> CreateTexture2D(const SamplerSpecification& sampler, uint32_t width, uint32_t height, uint32_t flatcolor) = 0;
		virtual Ref<Texture2D> CreateTexture2D(const SamplerSpecification& sampler, uint32_t width, uint32_t height, const Buffer& data) = 0;
		virtual Ref<FrameBuffer> CreateFrameBuffer(const FrameBufferSpecification& specs) = 0;
		virtual Ref<Viewport> CreateViewport(uint32_t widht, uint32_t height) = 0;
		virtual Ref<SwapChain> CreateSwapChain(const SwapChainSpecifications& specs) = 0;
	private:
		static API s_API;
	};

}