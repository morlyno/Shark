#pragma once

#include "Shark/Core/Base.h"
#include "RendererAPI.h"

namespace Shark {

	class RendererCommand
	{
	public:
		static inline void Init() { s_RendererAPI->Init(); }
		static inline void ShutDown() { s_RendererAPI->ShutDown(); }

		static inline void DrawIndexed(uint32_t count) { s_RendererAPI->DrawIndexed(count); }

		static inline Ref<RendererAPI> GetRendererAPI() { return s_RendererAPI; }
	private:
		static Ref<RendererAPI> s_RendererAPI;
	};

}