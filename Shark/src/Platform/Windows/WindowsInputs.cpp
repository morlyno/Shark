#include "skpch.h"
#include "Shark/Core/Input.h"
#include "Shark/Core/Application.h"

namespace Shark {

	bool Input::KeyPressed(KeyCode key)
	{
		return (bool)(GetKeyState(key) >> 8);
	}

	bool Input::MousePressed(MouseCode button)
	{
		return (bool)(GetKeyState(button) >> 8);
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

	std::pair<int, int> Input::GlobalMousePos()
	{
		POINT pos;
		GetCursorPos(&pos);
		return { pos.x,pos.y };
	}

	int Input::GlobalMousePosX()
	{
		return GlobalMousePos().first;
	}

	int Input::GlobalMousePosY()
	{
		return GlobalMousePos().second;
	}

}