#include "skpch.h"
#include "Window.h"

#include "Platform/Windows/WindowsWindow.h"

namespace Shark {

#if SK_PLATFORM_WINDOWS

	Window* Window::GetFromHandle(WindowHandle handle)
	{
		return (WindowsWindow*)GetWindowLongPtrA((HWND)handle, GWLP_USERDATA);
	}

	Scope<Window> Window::Create(const WindowSpecification& specification, Ref<EventListener> listener)
	{
		return Scope<WindowsWindow>::Create(specification, listener);
	}

	Scope<Window> Window::Create(const WindowSpecification& specification, const std::function<void(Event&)>& callback)
	{
		return Create(specification, Ref<EventListener>::Create(callback));
	}

#endif

}
