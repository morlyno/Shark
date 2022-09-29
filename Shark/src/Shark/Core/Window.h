#pragma once
#include "Shark/Core/Base.h"
#include "Shark/Event/Event.h"
#include "Shark/Event/EventListener.h"

#include "Shark/Render/SwapChain.h"

namespace Shark {

	struct WindowProps
	{
		std::string Name = "Shark Game Engine";
		int Width = 1280;
		int Height = 720;
		bool VSync = true;
		bool Maximized = false;
		Ref<EventListener> EventListener;

		WindowProps() = default;
		WindowProps(const std::string& name, int width, int height, bool VSync)
			: Name(name), Width(width), Height(height), VSync(VSync)
		{}
	};

	class Window
	{
	public:
		virtual ~Window() = default;
		virtual void CreateSwapChain() = 0;

		virtual void ProcessEvents() const = 0;

		virtual inline uint32_t GetWidth() const = 0;
		virtual inline uint32_t GetHeight() const = 0;
		virtual const glm::uvec2& GetSize() const = 0;
		virtual const glm::ivec2& GetPos() const = 0;
		virtual void ScreenToClient(glm::ivec2& screenPos) const = 0;

		virtual inline WindowHandle GetHandle() const = 0;
		virtual Ref<SwapChain> GetSwapChain() const = 0;

		virtual inline bool IsVSync() const = 0;
		virtual void SetVSync(bool VSync) = 0;
		virtual bool IsFocused() const = 0;

		virtual void Kill() = 0;

		virtual void Maximize() = 0;

		static Scope<Window> Create(const WindowProps& properties = WindowProps());
	};

}