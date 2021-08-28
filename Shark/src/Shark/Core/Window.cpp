#include "skpch.h"
#include "Window.h"

#include "Platform/Windows/WindowsWindow.h"

#include "Shark/Debug/Instrumentor.h"

namespace Shark {

#if SK_PLATFORM_WINDOWS
	Scope<Window> Window::Create(const WindowProps& properties)
	{
		SK_PROFILE_FUNCTION();

		return Scope<WindowsWindow>::Create(properties);
	}
#endif

}
