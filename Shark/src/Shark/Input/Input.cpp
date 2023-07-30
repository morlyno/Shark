#include "skpch.h"
#include "Input.h"

#include "Shark/Core/Application.h"
#include "Shark/Event/KeyEvent.h"
#include "Shark/Event/MouseEvent.h"
#include "Shark/UI/UI.h"

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
			case CursorMode::Show: return "Show";
			case CursorMode::Hide: return "Hide";
			case CursorMode::HideKeepInPlace: return "HideKeepInPlace";
		}

		SK_CORE_ASSERT(false, "Unkown CursorMode");
		return "Unkown";
	}

	struct InputData
	{
		std::unordered_map<KeyCode, KeyState> KeyStates;
		std::unordered_map<MouseButton, MouseState> MouseButtonStates;
		float MouseScroll = 0.0f;
		glm::vec2 MouseDelta = glm::vec2(0.0f);
		CursorMode CursorMode = CursorMode::Show;
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

		s_InputData->MouseScroll = 0.0f;
		s_InputData->MouseDelta = glm::ivec2(0);
	}

	void OnKeyEvent(KeyEvent& event)
	{
		if (event.IsRepeat())
			return;

		switch (event.GetEventType())
		{
			case EventType::KeyPressed: s_InputData->KeyStates[event.GetKeyCode()] = KeyState::Pressed; break;
			case EventType::KeyReleased: s_InputData->KeyStates[event.GetKeyCode()] = KeyState::Released; break;
		}
	}

	void OnMouseEvent(MouseEvent& event)
	{
		switch (event.GetEventType())
		{
			case EventType::MouseButtonPressed: s_InputData->MouseButtonStates[event.GetButton()] = MouseState::Pressed; break;
			case EventType::MouseButtonReleasd: s_InputData->MouseButtonStates[event.GetButton()] = MouseState::Released; break;
			case EventType::MouseButtonDoubleClicked: s_InputData->MouseButtonStates[event.GetButton()] = MouseState::DoubleClicked; break;
			case EventType::MouseScrolled: s_InputData->MouseScroll = event.As<MouseScrolledEvent>().GetDelta(); break;
			case EventType::MouseMovedRelative: s_InputData->MouseDelta = event.As<MouseMovedRelativeEvent>().GetMouseDelta(); break;
		}
	}

	void Input::OnEvent(Event& event)
	{
		if (event.IsInCategory(EventCategory::Keyboard))
			OnKeyEvent((KeyEvent&)event);
		if (event.IsInCategory(EventCategory::Mouse))
			OnMouseEvent((MouseEvent&)event);
	}

	void Input::SetDefaultCursorMode()
	{
		CURSORINFO cursorInfo;
		cursorInfo.cbSize = sizeof(CURSORINFO);
		GetCursorInfo(&cursorInfo);
		if (cursorInfo.flags & CURSOR_SUPPRESSED)
			while (ShowCursor(TRUE));

		if (cursorInfo.flags & CURSOR_SHOWING)
		{
			ShowCursor(TRUE);
			while (ShowCursor(FALSE));
		}

		ClipCursor(NULL);
		s_InputData->CursorMode = CursorMode::Show;
	}

	void Input::SetCursorMode(CursorMode mode)
	{
		if (s_InputData->CursorMode == mode)
			return;

		const bool isCursorHidden = s_InputData->CursorMode != CursorMode::Show;

		switch (mode)
		{
			case CursorMode::Show:
				if (isCursorHidden)
					ShowCursor(TRUE);
				ClipCursor(NULL);
				SK_CORE_TRACE_TAG("Input", "CursorMode Changed ({} => {})", ToString(s_InputData->CursorMode), "Show");
				break;
			case CursorMode::Hide:
				if (!isCursorHidden)
					ShowCursor(FALSE);
				ClipCursor(NULL);
				SK_CORE_TRACE_TAG("Input", "CursorMode Changed ({} => {})", ToString(s_InputData->CursorMode), "Hide");
				break;
			case CursorMode::HideKeepInPlace:
			{
				if (!isCursorHidden)
					ShowCursor(FALSE);
				glm::vec2 mousePoint = GetScreenMousePosition();
				RECT rect;
				rect.left = rect.right = mousePoint.x;
				rect.top = rect.bottom = mousePoint.y;
				ClipCursor(&rect);
				SK_CORE_TRACE_TAG("Input", "CursorMode Changed ({} => {})", ToString(s_InputData->CursorMode), "HideKeepInPlace");
				break;
			}
		}
		s_InputData->CursorMode = mode;
	}

	CursorMode Input::GetCursorMode()
	{
		return s_InputData->CursorMode;
	}

	bool Input::IsKeyDownAsync(KeyCode key)
	{
		return GetAsyncKeyState((int)key) & 0xff00;
	}

	bool Input::IsMouseDownAsync(MouseButton button)
	{
		return GetAsyncKeyState((int)button) & 0xff00;
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

	float Input::GetMouseScroll()
	{
		return s_InputData->MouseScroll;
	}

	glm::ivec2 Input::GetMouseDelta()
	{
		return s_InputData->MouseDelta;
	}

#if SK_PLATFORM_WINDOWS
	glm::ivec2 Input::GetMousePosition()
	{
		POINT pos;
		GetCursorPos(&pos);
		auto& window = Application::Get().GetWindow();
		return window.ScreenToWindow({ pos.x, pos.y });
	}

	glm::ivec2 Input::GetScreenMousePosition()
	{
		POINT pos;
		GetCursorPos(&pos);
		return { pos.x,pos.y };
	}
#endif

}
