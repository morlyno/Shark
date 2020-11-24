#pragma once
#include "skpch.h"
#include "Shark/Core.h"
#include "Event.h"

namespace Shark {

	class SHARK_API KeyEvent : public Event
	{
	public:
		unsigned char GetKeyCode() const { return KeyCode; }

		SK_GET_CATEGORY_FLAGS_FUNC( EventCategoryInput | EventCategoryKeyboard )
	protected:
		KeyEvent( unsigned char KeyCode )
			:
			KeyCode( KeyCode )
		{}
		unsigned char KeyCode;
	};

	class SHARK_API KeyPressedEvent : public KeyEvent
	{
	public:
		KeyPressedEvent( unsigned char KeyCode,unsigned int RepeatCount )
			:
			KeyEvent( KeyCode ),
			RepeatCount( RepeatCount )
		{}
		unsigned int GetRepeatCount() const { return RepeatCount; }

		SK_EVENT_FUNCTIONS( KeyPressed )
	private:
		unsigned int RepeatCount;
	};

	class SHARK_API KeyReleasedEvent : public KeyEvent
	{
	public:
		KeyReleasedEvent( unsigned char KeyCode )
			:
			KeyEvent( KeyCode )
		{}
		SK_EVENT_FUNCTIONS( KeyReleased )
	};

	//class SHARK_API KeyCharacterEvent : public KeyEvent
	//{
	//public:
	//	KeyCharacterEvent( unsigned char KeyCode ) // TODO: Change to Character not KeyCode
	//		:
	//		KeyEvent( KeyCode )
	//	{}
	//	SK_EVENT_FUNCTIONS( KeyCharacter )
	//};

}