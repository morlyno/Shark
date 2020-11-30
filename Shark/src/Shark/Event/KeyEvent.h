#pragma once
#include "skpch.h"
#include "Shark/Core/Core.h"
#include "Event.h"

namespace Shark {

	class KeyEvent : public Event
	{
	public:
		unsigned char GetKeyCode() const { return KeyCode; }

		virtual std::string ToString() const override
		{
			std::ostringstream oss;
			oss << GetName() << " " << KeyCode;
			return oss.str();
		}

		static EventTypes GetStaticType() { return EventTypes::KeyEventBase; }

		SK_GET_CATEGORY_FLAGS_FUNC( EventCategoryInput | EventCategoryKeyboard )
	protected:
		KeyEvent( unsigned char KeyCode )
			:
			KeyCode( KeyCode )
		{}
		unsigned char KeyCode;
	};

	class KeyPressedEvent : public KeyEvent
	{
	public:
		KeyPressedEvent( unsigned char KeyCode,unsigned int RepeatCount )
			:
			KeyEvent( KeyCode ),
			RepeatCount( RepeatCount )
		{}
		unsigned int GetRepeatCount() const { return RepeatCount; }

		std::string ToString() const override
		{
			std::ostringstream oss;
			oss << GetName() << " " << KeyCode << " " << RepeatCount;
			return oss.str();
		}

		SK_EVENT_FUNCTIONS( KeyPressed )
	private:
		unsigned int RepeatCount;
	};

	class KeyReleasedEvent : public KeyEvent
	{
	public:
		KeyReleasedEvent( unsigned char KeyCode )
			:
			KeyEvent( KeyCode )
		{}
		SK_EVENT_FUNCTIONS( KeyReleased )
	};

	class KeyCharacterEvent : public KeyEvent
	{
	public:
		KeyCharacterEvent( unsigned char Character )
			:
			KeyEvent( Character )
		{}
		SK_EVENT_FUNCTIONS( KeyCharacter )
	};

}