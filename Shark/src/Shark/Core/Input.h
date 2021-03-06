#pragma once
#include "Shark/Core/Base.h"
#include "Shark/Core/KeyCodes.h"
#include "Shark/Core/MouseCodes.h"

namespace Shark {

	namespace Input {

		bool KeyPressed(KeyCode key);
		
		bool MousePressed(MouseCode button);

		// First is x, Second is y
		// Relative to Window
		std::pair<int, int> MousePos();
		int MousePosX();
		int MousePosY();

		// First is x, Second is y
		// Relative to Screen
		std::pair<int, int> ScreenMousePos();
		int ScreenMousePosX();
		int ScreenMousePosY();

	}

}