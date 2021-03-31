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
		
		virtual void DrawIndexed(uint32_t count) = 0;
		virtual void Flush() = 0;

		static API GetAPI() { return s_API; }

		static Ref<RendererAPI> Create();
	private:
		static API s_API;
	};

}