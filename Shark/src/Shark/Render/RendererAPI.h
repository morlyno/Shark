#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/Texture.h"

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

		virtual void GetFramebufferContent(const Ref<Texture2D>& texture) = 0;

		virtual void SetClearColor(const float color[4]) = 0;
		virtual const float* GetClearColor() const = 0;
		virtual void ClearBuffer() = 0;
		virtual void SwapBuffer(bool VSync) = 0;

		virtual void SetBlendState(bool blend) = 0;
		virtual bool GeBlendState() const = 0;

		virtual void DrawIndexed(uint32_t count) = 0;

		virtual void OnResize(int width, int height) = 0;

		static API GetAPI() { return s_API; }

		static Scope<RendererAPI> Create();
	private:
		static API s_API;
	};

}