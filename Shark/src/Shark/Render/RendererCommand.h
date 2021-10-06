#pragma once

#include "Shark/Core/Base.h"
#include "RendererAPI.h"

namespace Shark {

	class RendererCommand
	{
	public:
		static inline void Init() { s_RendererAPI = RendererAPI::Create(); s_RendererAPI->Init(); }
		static inline void ShutDown() { s_RendererAPI->ShutDown(); s_RendererAPI = nullptr; }

		static inline void ResizeSwapChain(uint32_t width, uint32_t height) { s_RendererAPI->ResizeSwapChain(width, height); }
		static inline void SwapBuffers(bool vsync) { s_RendererAPI->SwapBuffers(vsync); }
		static inline void BindMainFrameBuffer() { s_RendererAPI->BindMainFrameBuffer(); }

		// Temp
		static inline void MainFrameBufferSetBlend(bool blend) { s_RendererAPI->MainFrameBufferSetBlend(blend); }

		static inline void Draw(uint32_t vertexCount, PrimitveTopology topology) { s_RendererAPI->Draw(vertexCount, topology); }
		static inline void DrawIndexed(uint32_t indexCount, PrimitveTopology topology) { s_RendererAPI->DrawIndexed(indexCount, topology); }
		static inline void Flush() { s_RendererAPI->Flush(); }

		static inline RendererAPI& GetRendererAPI() { return *s_RendererAPI; }
	private:
		static Scope<RendererAPI> s_RendererAPI;
	};

}