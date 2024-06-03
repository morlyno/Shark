#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Event/Event.h"
#include "Shark/Event/EventListener.h"
#include "Shark/Input/MouseButtons.h"
#include "Shark/Render/SwapChain.h"

#undef IsMaximized

namespace Shark {

	struct WindowSpecification
	{
		std::string Title = "Shark Game Engine";
		int Width = 1280;
		int Height = 720;
		bool Decorated = true;
		bool CustomTitlebar = false;
		bool Fullscreen = false;
		bool VSync = true;
	};

	class Window
	{
	public:
		virtual ~Window() = default;

		virtual void SetTitlebarHitTestCallback(const std::function<void(int, int, bool&)>& callback) = 0;

		virtual void SwapBuffers() = 0;
		virtual void ProcessEvents() = 0;

		virtual void KillWindow() = 0;

		virtual void SetFullscreen(bool fullscreen) = 0;
		virtual bool IsFullscreen() const = 0;

		virtual void SetTitle(const std::string& title) = 0;
		virtual const std::string& GetTitle() const = 0;

		virtual void Restore() = 0;
		virtual void Minimize() = 0;
		virtual void Maximize() = 0;

		virtual void CenterWindow() = 0;
		virtual void SetResizable(bool resizable) = 0;
		virtual bool IsResizable() const = 0;

		virtual bool IsFocused() const = 0;
		virtual void SetFocused() = 0;

		virtual bool IsMinimized() const = 0;
		virtual bool IsMaximized() const = 0;

		virtual bool VSyncEnabled() const = 0;
		virtual void EnableVSync(bool enabled) = 0;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual glm::uvec2 GetSize() const = 0;
		virtual glm::ivec2 GetPosition() const = 0;
		virtual glm::vec2 ScreenToWindow(const glm::vec2& screenPos) const = 0;
		virtual glm::vec2 WindowToScreen(const glm::vec2& windowPos) const = 0;

		virtual void SetCursorMode(CursorMode mode) = 0;

		virtual WindowHandle GetHandle() const = 0;
		virtual Ref<SwapChain> GetSwapChain() const = 0;

	public:
		static Scope<Window> Create(const WindowSpecification& specification, Ref<EventListener> listener);
		static Scope<Window> Create(const WindowSpecification& specification, const std::function<void(Event&)>& callback);
	};

}