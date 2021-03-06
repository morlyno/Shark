#pragma once

#include "Shark/Core/Base.h"

namespace Shark {

	class RendererAPI : public RefCount
	{
	public:
		enum class API
		{
			None = 0, DirectX11 = 1
		};
	public:
		virtual ~RendererAPI() = default;

		virtual void Init() = 0;
		virtual void ShutDown() = 0;
		
		virtual void ResizeSwapChain(uint32_t width, uint32_t height) = 0;
		virtual void SwapBuffers(bool vsync) = 0;
		virtual void BindMainFrameBuffer() = 0;

		// Temp
		virtual void MainFrameBufferSetBlend(bool blend) = 0;

		virtual void DrawIndexed(uint32_t count, uint32_t indexoffset, uint32_t vertexoffset) = 0;
		virtual void Flush() = 0;

		static API GetAPI() { return s_API; }
		static void SetAPI(API api) { s_API = api; }

		static Scope<RendererAPI> Create();
	private:
		static API s_API;
	};

}