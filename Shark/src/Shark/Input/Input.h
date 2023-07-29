#pragma once
#include "Shark/Core/Base.h"
#include "Shark/Event/Event.h"
#include "Shark/Input/KeyCodes.h"
#include "Shark/Input/MouseButtons.h"

namespace Shark {

	enum class KeyState : uint16_t
	{
		None = 0,
		Pressed,
		Down,
		Released
	};

	enum class MouseState : uint16_t
	{
		None = 0,
		Pressed,
		Down,
		Released,
		DoubleClicked
	};

	enum class CursorMode
	{
		Show,
		Hide,
		HideKeepInPlace
	};

	class Input
	{
	public:
		static void Initialize();
		static void Shutdown();

		static void TransitionStates();
		static void OnEvent(Event& event);

		static void SetCursorMode(CursorMode mode);
		static CursorMode GetCursorMode();

		static bool IsKeyDownAsync(KeyCode key);
		static bool IsMouseDownAsync(MouseButton button);

		static KeyState GetKeyState(KeyCode key);
		static bool IsKeyPressed(KeyCode key);
		static bool IsKeyDown(KeyCode key);
		static bool IsKeyRelease(KeyCode key);

		static MouseState GetMouseState(MouseButton button);
		static bool IsMousePressed(MouseButton button);
		static bool IsMouseDown(MouseButton button);
		static bool IsMouseRelease(MouseButton button);
		static bool IsMouseDoubleClicked(MouseButton button);

		static float GetMouseScroll();

		static glm::ivec2 GetMouseDelta();
		static glm::ivec2 GetMousePosition();
		static glm::ivec2 GetScreenMousePosition();
	};

	std::string ToString(CursorMode cursorMode);

}
