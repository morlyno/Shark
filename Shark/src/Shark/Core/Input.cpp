#include "skpch.h"
#include "Input.h"

namespace Shark {

	MouseInput MouseInput::s_inst;
	KeyInput KeyInput::s_inst;

	void MouseInput::OnCaptureInputs( MouseEvent& e )
	{
		EventDispacher dispacher( e );
		dispacher.DispachEvent<MouseMoveEvent>( SK_BIND_EVENT_FN_OF( MouseInput::OnMouseMoved,&s_inst ) );
		dispacher.DispachEvent<MousePressedEvent>( SK_BIND_EVENT_FN_OF( MouseInput::OnMousePressed,&s_inst ) );
		dispacher.DispachEvent<MouseReleasedEvent>( SK_BIND_EVENT_FN_OF( MouseInput::OnMouseReleased,&s_inst ) );
	}

	bool MouseInput::OnMouseMoved( MouseMoveEvent& e )
	{
		x = e.GetX();
		y = e.GetY();
		return false;
	}

	bool MouseInput::OnMousePressed( MousePressedEvent& e )
	{
		x = e.GetX();
		y = e.GetY();
		buttons[e.GetButton()] = true;
		return false;
	}

	bool MouseInput::OnMouseReleased( MouseReleasedEvent& e )
	{
		x = e.GetX();
		y = e.GetY();
		buttons[e.GetButton()] = false;
		return false;
	}

	void KeyInput::OnCaptureInputs( KeyEvent& e )
	{
		EventDispacher dispacher( e );
		dispacher.DispachEvent<KeyPressedEvent>( SK_BIND_EVENT_FN_OF( KeyInput::OnKeyPressed,&s_inst ) );
		dispacher.DispachEvent<KeyReleasedEvent>( SK_BIND_EVENT_FN_OF( KeyInput::OnKeyReleased,&s_inst ) );
	}

	bool KeyInput::OnKeyPressed( KeyPressedEvent& e )
	{
		keys[e.GetKeyCode()] = true;
		return false;
	}

	bool KeyInput::OnKeyReleased( KeyReleasedEvent& e )
	{
		keys[e.GetKeyCode()] = false;
		return false;
	}
}