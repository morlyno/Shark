#pragma once
#include "Shark/Core/Base.h"
#include "Shark/Input/KeyCodes.h"
#include "Shark/Input/MouseButtons.h"

namespace Shark {

	namespace Input {

		bool KeyPressed(KeyCode key);
		
		bool MousePressed(MouseButton::Type button);

		// Relative to Window
		glm::ivec2 MousePos();
		int MousePosX();
		int MousePosY();

		// Relative to Screen
		glm::ivec2 GlobalMousePos();
		int GlobalMousePosX();
		int GlobalMousePosY();

	}

}