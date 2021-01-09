#pragma once

#include "Shark/Core/Base.h"
#include "RendererAPI.h"

namespace Shark {

	class RendererCommand
	{
	public:
		static inline void Init(const class Window& window) { s_RendererAPI->Init(window); }
		static inline void ShutDown() { s_RendererAPI->ShutDown(); }

		static inline void Resize(int width, int height) { s_RendererAPI->OnResize(width, height); }

		static inline void SwapBuffer(bool VSync) { s_RendererAPI->SwapBuffer(VSync); }
		static inline void SetClearColor(const float color[4]) { s_RendererAPI->SetClearColor(color); }
		static inline void ClearBuffer() { s_RendererAPI->ClearBuffer(); }

		static inline void DrawIndexed(uint32_t count) { s_RendererAPI->DrawIndexed(count); }

		static inline RendererAPI& GetRendererAPI() { return *s_RendererAPI; }
	private:
		static Scope<RendererAPI> s_RendererAPI;
	};

}