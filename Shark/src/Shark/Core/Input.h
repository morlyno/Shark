#pragma once
#include "skpch.h"
#include "Shark/Core/Core.h"
#include "Shark/Event/Event.h"
#include "Shark/Event/MouseEvent.h"
#include "Shark/Event/KeyEvent.h"
#include "Shark/Core/KeyCodes.h"
#include "Shark/Core/MouseCodes.h"

namespace Shark {

	class MouseInput
	{
	public:
		static inline bool ButtonPressed( MouseCode button ) { return s_inst.buttons[button]; }
		static inline std::pair<int,int> GetPos() { return { s_inst.x,s_inst.y }; }
		static inline int GetX() { return s_inst.x; }
		static inline int GetY() { return s_inst.y; }

		static void OnCaptureInputs( MouseEvent& e );
	private:
		bool OnMouseMoved( MouseMoveEvent& e );
		bool OnMousePressed( MousePressedEvent& e );
		bool OnMouseReleased( MouseReleasedEvent& e );
	private:
		static MouseInput s_inst;
		int x,y;
		std::bitset<5> buttons;
	};

	class KeyInput
	{
	public:
		static inline bool KeyPressed( KeyCode key ) { return s_inst.keys[key]; }

		static void OnCaptureInputs( KeyEvent& e );
	private:
		bool OnKeyPressed( KeyPressedEvent& e );
		bool OnKeyReleased( KeyReleasedEvent& e );
	private:
		static KeyInput s_inst;
		std::bitset<0x100> keys;
	};
}