#include "skpch.h"
#include "Input.h"

#include "Shark/Core/Application.h"
#include "Shark/Core/Window.h"
#include "Shark/Event/KeyEvent.h"
#include "Shark/Event/MouseEvent.h"
#include "Shark/Utils/PlatformUtils.h"
#include "Shark/Debug/Profiler.h"

namespace Shark {

	struct KeyStatus
	{
		uint32_t RepeateCount = 0;
		KeyState State = KeyState::None;
		bool Repeate = false;
	};

	struct InputData
	{
		std::map<KeyCode, KeyStatus> KeyStates;
		std::map<MouseButton, MouseState> MouseButtonStates;
		glm::vec2 MouseScroll = glm::vec2(0.0f);
		glm::vec2 m_MousePosition;
		CursorMode m_CursorMode = CursorMode::Normal;
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

		std::erase_if(s_InputData->KeyStates, [](const auto& entry) { return entry.second.State == KeyState::Released; });
		std::erase_if(s_InputData->MouseButtonStates, [](const auto& entry) { return entry.second == MouseState::Released || entry.second == MouseState::DoubleClicked; });

		for (auto& [key, status] : s_InputData->KeyStates)
		{
			status.Repeate = false;
			if (status.State == KeyState::Pressed)
				status.State = KeyState::Down;
		}

		for (auto& [button, state] : s_InputData->MouseButtonStates)
		{
			if (state == MouseState::Pressed)
				state = MouseState::Down;
		}

		s_InputData->MouseScroll = glm::vec2(0.0f);
	}

	void Input::OnEvent(Event& event)
	{
		EventDispacher dispacher(event);

		dispacher.DispachEvent<KeyPressedEvent>([](auto& e)
		{
			auto& status = s_InputData->KeyStates[e.GetKeyCode()];
			if (!e.IsRepeat())
				status.State = KeyState::Pressed;
			else
			{
				status.RepeateCount++;
				status.Repeate = true;
			}
			return false;
		});

		dispacher.DispachEvent<KeyReleasedEvent>([](auto& e)
		{
			s_InputData->KeyStates[e.GetKeyCode()].State = KeyState::Released;
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

		auto& app = Application::Get();
		if (app.GetSpecification().EnableImGui)
		{
			if (mode == CursorMode::Locked)
				ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
			else
				ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
		}

		auto& window = app.GetWindow();
		window.SetCursorMode(mode);

		s_InputData->m_CursorMode = mode;
	}

	CursorMode Input::GetCursorMode()
	{
		return s_InputData->m_CursorMode;
	}

	KeyState Input::GetKeyState(KeyCode key)
	{
		if (!s_InputData->KeyStates.contains(key))
			return KeyState::None;
		return s_InputData->KeyStates.at(key).State;
	}

	bool Input::IsRepeated(KeyCode key)
	{
		return s_InputData->KeyStates.at(key).Repeate;
	}

	bool Input::IsKeyPressed(KeyCode key)
	{
		switch (key)
		{
			case KeyCode::Alt:     return IsKeyPressed(KeyCode::LeftAlt) || IsKeyPressed(KeyCode::RightAlt);
			case KeyCode::Shift:   return IsKeyPressed(KeyCode::LeftShift) || IsKeyPressed(KeyCode::RightShift);
			case KeyCode::Control: return IsKeyPressed(KeyCode::LeftControl) || IsKeyPressed(KeyCode::RightControl);
		}

		if (!s_InputData->KeyStates.contains(key))
			return false;

		return s_InputData->KeyStates.at(key).State == KeyState::Pressed;
	}

	bool Input::IsKeyPressed(KeyCode key, bool allowRepeate)
	{
		if (IsKeyPressed(key))
			return true;

		if (!allowRepeate)
			return false;

		const auto KeyDownAndRepeated = [](KeyCode key)
		{
			if (!s_InputData->KeyStates.contains(key))
				return false;

			auto& status = s_InputData->KeyStates.at(key);
			return status.State == KeyState::Down && status.Repeate;
		};

		switch (key)
		{
			case KeyCode::Alt:     return KeyDownAndRepeated(KeyCode::LeftAlt) || KeyDownAndRepeated(KeyCode::RightAlt);
			case KeyCode::Shift:   return KeyDownAndRepeated(KeyCode::LeftShift) || KeyDownAndRepeated(KeyCode::RightShift);
			case KeyCode::Control: return KeyDownAndRepeated(KeyCode::LeftControl) || KeyDownAndRepeated(KeyCode::RightControl);
		}

		return KeyDownAndRepeated(key);
	}

	bool Input::IsKeyDown(KeyCode key)
	{
		switch (key)
		{
			case KeyCode::Alt:     return IsKeyDown(KeyCode::LeftAlt) || IsKeyDown(KeyCode::RightAlt);
			case KeyCode::Shift:   return IsKeyDown(KeyCode::LeftShift) || IsKeyDown(KeyCode::RightShift);
			case KeyCode::Control: return IsKeyDown(KeyCode::LeftControl) || IsKeyDown(KeyCode::RightControl);
		}

		if (!s_InputData->KeyStates.contains(key))
			return false;

		return s_InputData->KeyStates.at(key).State == KeyState::Down;
	}

	bool Input::IsKeyRelease(KeyCode key)
	{
		switch (key)
		{
			case KeyCode::Alt:     return IsKeyRelease(KeyCode::LeftAlt) || IsKeyRelease(KeyCode::RightAlt);
			case KeyCode::Shift:   return IsKeyRelease(KeyCode::LeftShift) || IsKeyRelease(KeyCode::RightShift);
			case KeyCode::Control: return IsKeyRelease(KeyCode::LeftControl) || IsKeyRelease(KeyCode::RightControl);
		}

		if (!s_InputData->KeyStates.contains(key))
			return false;

		return s_InputData->KeyStates.at(key).State == KeyState::Released;
	}

	MouseState Input::GetMouseState(MouseButton button)
	{
		if (!s_InputData->MouseButtonStates.contains(button))
			return MouseState::None;
		return s_InputData->MouseButtonStates.at(button);
	}

	bool Input::IsMousePressed(MouseButton button)
	{
		if (!s_InputData->MouseButtonStates.contains(button))
			return false;
		return s_InputData->MouseButtonStates.at(button) == MouseState::Pressed;
	}

	bool Input::IsMouseDown(MouseButton button)
	{
		if (!s_InputData->MouseButtonStates.contains(button))
			return false;
		return s_InputData->MouseButtonStates.at(button) == MouseState::Down;
	}

	bool Input::IsMouseRelease(MouseButton button)
	{
		if (!s_InputData->MouseButtonStates.contains(button))
			return false;
		return s_InputData->MouseButtonStates.at(button) == MouseState::Released;
	}

	bool Input::IsMouseDoubleClicked(MouseButton button)
	{
		if (!s_InputData->MouseButtonStates.contains(button))
			return false;
		return s_InputData->MouseButtonStates.at(button) == MouseState::DoubleClicked;
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

#if 0
	const std::map<KeyCode, KeyState>& Input::GetKeyStates()
	{
		return s_InputData->KeyStates;
	}
#endif

	const std::map<MouseButton, MouseState>& Input::GetMouseButtonStates()
	{
		return s_InputData->MouseButtonStates;
	}

}
