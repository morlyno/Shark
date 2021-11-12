#pragma once
#include "Shark/Core/Base.h"
#include "Event.h"
#include "Shark/Core/KeyCodes.h"

namespace Shark {

	class KeyEvent : public Event
	{
	public:
		KeyCode GetKeyCode() const { return m_KeyCode; }

		virtual std::string ToString() const override
		{
			std::ostringstream oss;
			oss << GetName() << " " << m_KeyCode;
			return oss.str();
		}
		SK_GET_CATEGORY_FLAGS_FUNC(EventCategoryInput | EventCategoryKeyboard)
	protected:
		KeyEvent(KeyCode keycode)
			: m_KeyCode(keycode)
		{}
		KeyCode m_KeyCode;
	};

	class KeyPressedEvent : public KeyEvent
	{
	public:
		KeyPressedEvent(KeyCode keycode, uint32_t RepeatCount, bool altPressed)
			: KeyEvent(keycode), m_RepeatCount(RepeatCount), m_AltPressed(altPressed)
		{}
		uint32_t GetRepeatCount() const { return m_RepeatCount; }
		bool AltPressed() const { return m_AltPressed; }

		std::string ToString() const override
		{
			return fmt::format("{}, KeyCode: {}, RepeatCount: {}, AltDown: {}", GetName(), KeyToString(m_KeyCode), m_RepeatCount, m_AltPressed);
		}

		SK_EVENT_FUNCTIONS(KeyPressed)
	private:
		uint32_t m_RepeatCount;
		bool m_AltPressed;
	};

	class KeyReleasedEvent : public KeyEvent
	{
	public:
		KeyReleasedEvent(KeyCode keycode)
			:
			KeyEvent(keycode)
		{}
		SK_EVENT_FUNCTIONS(KeyReleased)
	};

	class KeyCharacterEvent : public KeyEvent
	{
	public:
		KeyCharacterEvent(KeyCode Character)
			:
			KeyEvent(Character)
		{}
		SK_EVENT_FUNCTIONS(KeyCharacter)
	};

}