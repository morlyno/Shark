#pragma once
#include "Shark/Core/Base.h"
#include "Event.h"
#include "Shark/Input/KeyCodes.h"

namespace Shark {

	class KeyPressedEvent : public EventBase<EventType::KeyPressed, EventCategory::Input | EventCategory::Keyboard>
	{
	public:
		KeyPressedEvent(KeyCode keycode, bool isRepeat, const ModifierKeys& modifierKeys)
			: m_KeyCode(keycode), m_IsRepeat(isRepeat), m_ModifierKeys(modifierKeys) {}

		KeyCode GetKeyCode() const { return m_KeyCode; }
		bool IsRepeat() const { return m_IsRepeat; }
		const ModifierKeys& GetModifierKeys() const { return m_ModifierKeys; }

		std::string ToString() const override
		{
			return fmt::format("{}, Key: {} [{}], IsRepeat: {}, (Alt: {}, Shift: {}, Control: {})",
							   GetName(),
							   m_KeyCode,
							   (uint16_t)m_KeyCode,
							   m_IsRepeat,
							   m_ModifierKeys.Alt,
							   m_ModifierKeys.Shift,
							   m_ModifierKeys.Control);
		}

	private:
		KeyCode m_KeyCode;
		bool m_IsRepeat;
		ModifierKeys m_ModifierKeys;
	};

	class KeyReleasedEvent : public EventBase<EventType::KeyReleased, EventCategory::Input | EventCategory::Keyboard>
	{
	public:
		KeyReleasedEvent(KeyCode keycode, const ModifierKeys& modifierKeys)
			: m_KeyCode(keycode), m_ModifierKeys(modifierKeys) {}

		KeyCode GetKeyCode() const { return m_KeyCode; }
		const ModifierKeys& GetModifierKeys() const { return m_ModifierKeys; }

		std::string ToString() const override
		{
			return fmt::format("{}, Key: {} [{}], (Alt: {}, Shift: {}, Control: {})",
							   GetName(),
							   m_KeyCode,
							   (uint16_t)m_KeyCode,
							   m_ModifierKeys.Alt,
							   m_ModifierKeys.Shift,
							   m_ModifierKeys.Control);
		}

	private:
		KeyCode m_KeyCode;
		ModifierKeys m_ModifierKeys;
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