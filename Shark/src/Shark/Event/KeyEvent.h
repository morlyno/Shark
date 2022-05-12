#pragma once
#include "Shark/Core/Base.h"
#include "Event.h"
#include "Shark/Input/KeyCodes.h"

namespace Shark {

	class KeyPressedEvent : public EventBase<EventType::KeyPressed, EventCategory::Input | EventCategory::Keyboard>
	{
	public:
		KeyPressedEvent(KeyCode keycode, bool isRepeat, bool altPressed)
			: m_KeyCode(keycode), m_IsRepeat(isRepeat), m_AltPressed(altPressed)
		{}

		KeyCode GetKeyCode() const { return m_KeyCode; }
		bool IsRepeat() const { return m_IsRepeat; }
		bool AltPressed() const { return m_AltPressed; }

		std::string ToString() const override { return fmt::format("{}, Key: {} [{}], IsRepeat: {}, AltDown: {}", GetName(), Key::ToString(m_KeyCode), m_KeyCode, m_IsRepeat, m_AltPressed); }

	private:
		KeyCode m_KeyCode;
		bool m_IsRepeat;
		bool m_AltPressed;
	};

	class KeyReleasedEvent : public EventBase<EventType::KeyReleased, EventCategory::Input | EventCategory::Keyboard>
	{
	public:
		KeyReleasedEvent(KeyCode keycode)
			: m_KeyCode(keycode)
		{}

		KeyCode GetKeyCode() const { return m_KeyCode; }
		std::string ToString() const override { return fmt::format("{}, Key: {} [{}]", GetName(), Key::ToString(m_KeyCode), m_KeyCode); }

	private:
		KeyCode m_KeyCode;
	};

	class KeyCharacterEvent : public EventBase<EventType::KeyCharacter, EventCategory::Input | EventCategory::Keyboard>
	{
	public:
		KeyCharacterEvent(KeyCode Character, bool isRepeat)
			: m_KeyCode(Character), m_IsRepeat(isRepeat)
		{}

		KeyCode GetKeyCode() const { return m_KeyCode; }
		bool IsRepeat() const { return m_IsRepeat; }
		std::string ToString() const override { return fmt::format("{0}, Key: {1} [0x{1:x}], IsRepeat: {2}", GetName(), (unsigned char)m_KeyCode, IsRepeat()); }

	private:
		KeyCode m_KeyCode;
		bool m_IsRepeat;
	};

}