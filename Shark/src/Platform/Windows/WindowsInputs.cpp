#include "skpch.h"
#include "Shark/Input/Input.h"
#include "Shark/Core/Application.h"

namespace Shark {

	bool Input::KeyPressed(KeyCode key)
	{
		return GetKeyState(key) & 0xFF00;
	}

	bool Input::MousePressed(MouseButton::Type button)
	{
		return GetKeyState(button) & 0xFF00;
	}

	glm::ivec2 Input::MousePos()
	{
		POINT pos;
		GetCursorPos(&pos);
		auto& window = Application::Get().GetWindow();
		ScreenToClient((HWND)window.GetHandle(), &pos);
		return { pos.x,pos.y };
	}

	int Input::MousePosX()
	{
		return MousePos().x;
	}

	int Input::MousePosY()
	{
		return MousePos().y;
	}

	glm::ivec2 Input::GlobalMousePos()
	{
		POINT pos;
		GetCursorPos(&pos);
		return { pos.x,pos.y };
	}

	int Input::GlobalMousePosX()
	{
		return GlobalMousePos().y;
	}

	int Input::GlobalMousePosY()
	{
		return GlobalMousePos().x;
	}

}