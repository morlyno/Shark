#pragma once
#include "Shark/Core/Core.h"
#include "Shark/Core/KeyCodes.h"
#include "Shark/Core/MouseCodes.h"

namespace Shark {

	class Input
	{
	public:
		static bool KeyButtonPressed(KeyCode key);

		static bool MouseButtonPressed(MouseCode button);

		// First is x, Second is y
		// Relative to Window
		static std::pair<int, int> MousePos();
		static int MousePosX();
		static int MousePosY();

		// First is x, Second is y
		// Relative to Screen
		static std::pair<int, int> ScreenMousePos();
		static int ScreenMousePosX();
		static int ScreenMousePosY();
	};

}