#pragma once
#include "Shark/Core/Base.h"
#include "Event.h"
#include "Shark/Core/KeyCodes.h"

namespace Shark {

	class KeyPressedEvent : public EventBase<EventTypes::KeyPressed, EventCategoryInput | EventCategoryKeyboard>
	{
	public:
		KeyPressedEvent(KeyCode keycode, uint32_t RepeatCount, bool altPressed)
			: m_KeyCode(keycode), m_RepeatCount(RepeatCount), m_AltPressed(altPressed)
		{}

		KeyCode GetKeyCode() const { return m_KeyCode; }
		uint32_t GetRepeatCount() const { return m_RepeatCount; }
		bool AltPressed() const { return m_AltPressed; }

		std::string ToString() const override { return fmt::format("{}, Key: {} [{}], RepeatCount: {}, AltDown: {}", GetName(), KeyToString(m_KeyCode), m_KeyCode, m_RepeatCount, m_AltPressed); }

	private:
		KeyCode m_KeyCode;
		uint32_t m_RepeatCount;
		bool m_AltPressed;
	};

	class KeyReleasedEvent : public EventBase<EventTypes::KeyReleased, EventCategoryInput | EventCategoryKeyboard>
	{
	public:
		KeyReleasedEvent(KeyCode keycode)
			: m_KeyCode(keycode)
		{}

		KeyCode GetKeyCode() const { return m_KeyCode; }
		std::string ToString() const override { return fmt::format("{}, Key: {} [{}]", GetName(), KeyToString(m_KeyCode), m_KeyCode); }

	private:
		KeyCode m_KeyCode;
	};

	class KeyCharacterEvent : public EventBase<EventTypes::KeyCharacter, EventCategoryInput | EventCategoryKeyboard>
	{
	public:
		KeyCharacterEvent(KeyCode Character)
			: m_KeyCode(Character)
		{}

		KeyCode GetKeyCode() const { return m_KeyCode; }
		std::string ToString() const override { return fmt::format("{}, Key: {} [{}]", GetName(), KeyToString(m_KeyCode), m_KeyCode); }

	private:
		KeyCode m_KeyCode;
	};

}