#include "skpch.h"
#include "Input.h"

#include "Shark/Core/Application.h"
#include "Shark/Core/Window.h"
#include "Shark/Event/KeyEvent.h"
#include "Shark/Event/MouseEvent.h"
#include "Shark/Utils/PlatformUtils.h"
#include "Shark/Debug/Profiler.h"

namespace Shark {

	std::string_view ToString(KeyState state)
	{
		switch (state)
		{
			case KeyState::None:      return "None";
			case KeyState::Pressed:   return "Pressed";
			case KeyState::Down:      return "Down";
			case KeyState::Released:  return "Released";
		}
		SK_CORE_ASSERT("Unkown KeyState");
		return "Unkown";
	}

	std::string_view ToString(MouseState state)
	{
		switch (state)
		{
			case MouseState::None:           return "None";
			case MouseState::Pressed:        return "Pressed";
			case MouseState::Down:           return "Down";
			case MouseState::Released:       return "Released";
			case MouseState::DoubleClicked:  return "DoubleClicked";
		}
		SK_CORE_ASSERT(false, "Unkown MouseState");
		return "Unkown";
	}

	std::string ToString(CursorMode cursorMode)
	{
		switch (cursorMode)
		{
			case CursorMode::Normal: return "Normal";
			case CursorMode::Hidden: return "Hidden";
			case CursorMode::Locked: return "Locked";
		}

		SK_CORE_ASSERT(false, "Unkown CursorMode");
		return "Unkown";
	}

	struct InputData
	{
		std::unordered_map<KeyCode, KeyState> KeyStates;
		std::unordered_map<MouseButton, MouseState> MouseButtonStates;
		glm::vec2 MouseScroll = glm::vec2(0.0f);
		CursorMode m_CursorMode = CursorMode::Normal;
		glm::vec2 m_MousePosition;
	};
	static InputData* s_InputData = nullptr;

	void Input::Initialize()
	{
		s_InputData = sknew InputData;
	}

	void Input::Shutdown()
	{
		skdelete s_InputData;
		s_InputData = nullptr;
	}

	void Input::TransitionStates()
	{
		SK_PROFILE_FUNCTION();

		for (auto& [key, state] : s_InputData->KeyStates)
		{
			switch (state)
			{
				case KeyState::Pressed:  state = KeyState::Down; break;
				case KeyState::Released: state = KeyState::None; break;
			}
		}

		for (auto& [key, state] : s_InputData->MouseButtonStates)
		{
			switch (state)
			{
				case MouseState::Pressed:       state = MouseState::Down; break;
				case MouseState::Released:      state = MouseState::None; break;
				case MouseState::DoubleClicked: state = MouseState::None; break;
			}
		}

		s_InputData->MouseScroll = glm::vec2(0.0f);
	}

	void Input::OnEvent(Event& event)
	{
		EventDispacher dispacher(event);

		dispacher.DispachEvent<KeyPressedEvent>([](auto& e)
		{
			if (!e.IsRepeat())
				s_InputData->KeyStates[e.GetKeyCode()] = KeyState::Pressed;
			return false;
		});

		dispacher.DispachEvent<KeyReleasedEvent>([](auto& e)
		{
			s_InputData->KeyStates[e.GetKeyCode()] = KeyState::Released;
			return false;
		});

		dispacher.DispachEvent<MouseButtonPressedEvent>([](auto& e)
		{
			s_InputData->MouseButtonStates[e.GetButton()] = MouseState::Pressed;
			return false;
		});

		dispacher.DispachEvent<MouseButtonReleasedEvent>([](auto& e)
		{
			s_InputData->MouseButtonStates[e.GetButton()] = MouseState::Released;
			return false;
		});

		dispacher.DispachEvent<MouseButtonDoubleClickedEvent>([](auto& e)
		{
			s_InputData->MouseButtonStates[e.GetButton()] = MouseState::DoubleClicked;
			return false;
		});

		dispacher.DispachEvent<MouseScrolledEvent>([](auto& e)
		{
			s_InputData->MouseScroll.x = e.GetXOffset();
			s_InputData->MouseScroll.y = e.GetYOffset();
			return false;
		});

		dispacher.DispachEvent<MouseMovedEvent>([](auto& e)
		{
			s_InputData->m_MousePosition.x = e.GetX();
			s_InputData->m_MousePosition.y = e.GetY();
			return false;
		});
	}

	void Input::SetCursorMode(CursorMode mode)
	{
		if (s_InputData->m_CursorMode == mode)
			return;

		auto& window = Application::Get().GetWindow();
		window.SetCursorMode(mode);

		s_InputData->m_CursorMode = mode;
	}

	CursorMode Input::GetCursorMode()
	{
		return s_InputData->m_CursorMode;
	}

	KeyState Input::GetKeyState(KeyCode key)
	{
		return s_InputData->KeyStates[key];
	}

	bool Input::IsKeyPressed(KeyCode key)
	{
		switch (key)
		{
			case KeyCode::Alt:     return IsKeyPressed(KeyCode::LeftAlt) || IsKeyPressed(KeyCode::RightAlt);
			case KeyCode::Shift:   return IsKeyPressed(KeyCode::LeftShift) || IsKeyPressed(KeyCode::RightShift);
			case KeyCode::Control: return IsKeyPressed(KeyCode::LeftControl) || IsKeyPressed(KeyCode::RightControl);
		}

		return s_InputData->KeyStates[key] == KeyState::Pressed;
	}

	bool Input::IsKeyDown(KeyCode key)
	{
		switch (key)
		{
			case KeyCode::Alt:     return IsKeyDown(KeyCode::LeftAlt) || IsKeyDown(KeyCode::RightAlt);
			case KeyCode::Shift:   return IsKeyDown(KeyCode::LeftShift) || IsKeyDown(KeyCode::RightShift);
			case KeyCode::Control: return IsKeyDown(KeyCode::LeftControl) || IsKeyDown(KeyCode::RightControl);
		}

		return s_InputData->KeyStates[key] == KeyState::Down;
	}

	bool Input::IsKeyRelease(KeyCode key)
	{
		switch (key)
		{
			case KeyCode::Alt:     return IsKeyRelease(KeyCode::LeftAlt) || IsKeyRelease(KeyCode::RightAlt);
			case KeyCode::Shift:   return IsKeyRelease(KeyCode::LeftShift) || IsKeyRelease(KeyCode::RightShift);
			case KeyCode::Control: return IsKeyRelease(KeyCode::LeftControl) || IsKeyRelease(KeyCode::RightControl);
		}

		return s_InputData->KeyStates[key] == KeyState::Released;
	}

	MouseState Input::GetMouseState(MouseButton button)
	{
		return s_InputData->MouseButtonStates[button];
	}

	bool Input::IsMousePressed(MouseButton button)
	{
		return s_InputData->MouseButtonStates[button] == MouseState::Pressed;
	}

	bool Input::IsMouseDown(MouseButton button)
	{
		return s_InputData->MouseButtonStates[button] == MouseState::Down;
	}

	bool Input::IsMouseRelease(MouseButton button)
	{
		return s_InputData->MouseButtonStates[button] == MouseState::Released;
	}

	bool Input::IsMouseDoubleClicked(MouseButton button)
	{
		return s_InputData->MouseButtonStates[button] == MouseState::DoubleClicked;
	}

	glm::vec2 Input::GetMouseScroll()
	{
		return s_InputData->MouseScroll;
	}

	float Input::GetXScroll()
	{
		return s_InputData->MouseScroll.x;
	}

	float Input::GetYScroll()
	{
		return s_InputData->MouseScroll.y;
	}

	glm::vec2 Input::GetMousePosition()
	{
		return s_InputData->m_MousePosition;
	}

	float Input::GetXPosition()
	{
		return s_InputData->m_MousePosition.x;
	}

	float Input::GetYPosition()
	{
		return s_InputData->m_MousePosition.y;
	}

}
