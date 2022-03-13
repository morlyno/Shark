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
		glm::ivec2 MousePos();
		int MousePosX();
		int MousePosY();

		// First is x, Second is y
		// Relative to Screen
		std::pair<int, int> GlobalMousePos();
		int GlobalMousePosX();
		int GlobalMousePosY();

	}

}