#include "skpch.h"
#include "Window.h"

#include "Platform/Windows/WindowsWindow.h"

namespace Shark {

#ifdef SK_PLATFORM_WINDOWS
	Scope<Window> Window::Create(const WindowProps& properties)
	{
		return CreateScope<WindowsWindow>(properties);
	}
#endif

}
