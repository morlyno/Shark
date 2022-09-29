#pragma once
#include "Shark/Core/Base.h"
#include "Event.h"
#include "Shark/Input/KeyCodes.h"

namespace Shark {

	class KeyEvent : public Event
	{
	public:
		virtual KeyCode GetKeyCode() const { return KeyCode::None; };
		virtual bool IsRepeat() const { return false; }
	};

	class KeyPressedEvent : public EventBase<KeyEvent, EventType::KeyPressed, EventCategory::Input | EventCategory::Keyboard>
	{
	public:
		KeyPressedEvent(KeyCode keycode, bool isRepeat)
			: m_KeyCode(keycode), m_IsRepeat(isRepeat)
		{}

		virtual KeyCode GetKeyCode() const override { return m_KeyCode; }
		virtual bool IsRepeat() const override { return m_IsRepeat; }

		std::string ToString() const override { return fmt::format("{}, Key: {} [{}], IsRepeat: {}", GetName(), Shark::ToString(m_KeyCode), (uint16_t)m_KeyCode, m_IsRepeat); }

	private:
		KeyCode m_KeyCode;
		bool m_IsRepeat;
	};

	class KeyReleasedEvent : public EventBase<KeyEvent, EventType::KeyReleased, EventCategory::Input | EventCategory::Keyboard>
	{
	public:
		KeyReleasedEvent(KeyCode keycode)
			: m_KeyCode(keycode)
		{}

		virtual KeyCode GetKeyCode() const override { return m_KeyCode; }
		std::string ToString() const override { return fmt::format("{}, Key: {} [{}]", GetName(), Shark::ToString(m_KeyCode), (uint16_t)m_KeyCode); }

	private:
		KeyCode m_KeyCode;
	};

#if 0
	class KeyCharacterEvent : public EventBase<KeyEvent, EventType::KeyCharacter, EventCategory::Input | EventCategory::Keyboard>
	{
	public:
		KeyCharacterEvent(char16_t utf16Char, bool isRepeat)
			: m_UTF16Char(utf16Char), m_IsRepeat(isRepeat)
		{}

		char16_t GetCharacter() const { return m_UTF16Char; }
		virtual bool IsRepeat() const override { return m_IsRepeat; }
		std::string ToString() const override { return fmt::format("{0}, Key: {1} [0x{1:x}], IsRepeat: {2}", GetName(), (wchar_t)m_UTF16Char, IsRepeat()); }

	private:
		char16_t m_UTF16Char;
		bool m_IsRepeat;
	};
#endif

}