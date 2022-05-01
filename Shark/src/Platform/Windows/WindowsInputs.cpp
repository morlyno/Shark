#include "skpch.h"
#include "Shark/Core/Input.h"
#include "Shark/Core/Application.h"

namespace Shark {

	namespace utils {

		int MapMouseButtonToVK(MouseButton::Type button)
		{
			switch (button)
			{
				case MouseButton::Left:    return VK_LBUTTON;
				case MouseButton::Right:   return VK_RBUTTON;
				case MouseButton::Middle:  return VK_MBUTTON;
				case MouseButton::Thumb01: return VK_XBUTTON1;
				case MouseButton::Thumb02: return VK_XBUTTON2;
				case MouseButton::Invalid: return 0;
			}
			return 0;
		}

	}

	bool Input::KeyPressed(KeyCode key)
	{
		return (bool)(GetKeyState(key) >> 8);
	}

	bool Input::MousePressed(MouseButton::Type button)
	{
		const int virtualKey = utils::MapMouseButtonToVK(button);
		return (bool)(GetKeyState(virtualKey) >> 8);
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