#include "skpch.h"
#include "Window.h"

#include "Platform/Windows/WindowsWindow.h"

namespace Shark {

#if SK_PLATFORM_WINDOWS
	Scope<Window> Window::Create(const WindowProps& properties)
	{
		return Scope<WindowsWindow>::Create(properties);
	}
#endif

}
