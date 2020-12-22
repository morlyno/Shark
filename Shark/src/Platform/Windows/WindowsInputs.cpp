#include "skpch.h"
#include "Shark/Core/Input.h"
#include "Shark/Core/Application.h"

namespace Shark {

	bool Input::KeyButtonPressed( KeyCode key )
	{
		return (bool)(GetKeyState( key ) >> 8);
	}

	bool Input::MouseButtonPressed( MouseCode button )
	{
		return (bool)(GetKeyState( button ) >> 8);
	}

	std::pair<int,int> Input::MousePos()
	{
		POINT pos;
		GetCursorPos( &pos );
		auto& window = Application::Get().GetWindow();
		ScreenToClient( (HWND)window.GetHandle(),&pos );
		return { pos.x,pos.y };
	}

	int Input::MousePosX()
	{
		return MousePos().first;
	}

	int Input::MousePosY()
	{
		return MousePos().second;
	}

	std::pair<int,int> Input::ScreenMousePos()
	{
		POINT pos;
		GetCursorPos( &pos );
		return { pos.x,pos.y };
	}

	int Input::ScreenMousePosX()
	{
		return ScreenMousePos().first;
	}

	int Input::ScreenMousePosY()
	{
		return ScreenMousePos().second;
	}

}