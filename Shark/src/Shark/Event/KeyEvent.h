#pragma once
#include "skpch.h"
#include "Shark/Core/Core.h"
#include "Event.h"
#include "Shark/Core/KeyCodes.h"

namespace Shark {

	class KeyEvent : public Event
	{
	public:
		KeyCode GetKeyCode() const { return keycode; }

		virtual std::string ToString() const override
		{
			std::ostringstream oss;
			oss << GetName() << " " << keycode;
			return oss.str();
		}

		static EventTypes GetStaticType() { return EventTypes::KeyEventBase; }

		SK_GET_CATEGORY_FLAGS_FUNC( EventCategoryInput | EventCategoryKeyboard )
	protected:
		KeyEvent( KeyCode keycode )
			:
			keycode( keycode )
		{}
		KeyCode keycode;
	};

	class KeyPressedEvent : public KeyEvent
	{
	public:
		KeyPressedEvent( KeyCode keycode,unsigned int RepeatCount )
			:
			KeyEvent( keycode ),
			RepeatCount( RepeatCount )
		{}
		unsigned int GetRepeatCount() const { return RepeatCount; }

		std::string ToString() const override
		{
			std::ostringstream oss;
			oss << GetName() << " " << keycode << " " << RepeatCount;
			return oss.str();
		}

		SK_EVENT_FUNCTIONS( KeyPressed )
	private:
		unsigned int RepeatCount;
	};

	class KeyReleasedEvent : public KeyEvent
	{
	public:
		KeyReleasedEvent( KeyCode keycode )
			:
			KeyEvent( keycode )
		{}
		SK_EVENT_FUNCTIONS( KeyReleased )
	};

	class KeyCharacterEvent : public KeyEvent
	{
	public:
		KeyCharacterEvent( KeyCode Character )
			:
			KeyEvent( Character )
		{}
		SK_EVENT_FUNCTIONS( KeyCharacter )
	};

}