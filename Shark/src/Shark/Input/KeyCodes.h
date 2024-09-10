#pragma once

#include <magic_enum.hpp>

namespace Shark {
	
	enum class KeyCode : uint16_t
	{
		None                       = 0,

		BackSpace                  = 0x08,
		Tab                        = 0x09,

		Clear                      = 0x0C,
		Return                     = 0x0D,

		Shift                      = 0x10,
		Control                    = 0x11,
		Alt                        = 0x12,
		Pause                      = 0x13,
		Capital                    = 0x14, // CapsLock

		// Kana                       = 0x15,
		// Hangeul                    = 0x15,  /* old name - should be here for compatibility */
		// Hangul                     = 0x15,
		// Ime_on                     = 0x16,
		// Junja                      = 0x17,
		// Final                      = 0x18,
		// Hanja                      = 0x19,
		// Kanji                      = 0x19,
		// ImeOff                     = 0x1A,

		Escape                     = 0x1B,

		Convert                    = 0x1C,
		Nonconvert                 = 0x1D,
		Accept                     = 0x1E,
		Modechange                 = 0x1F,
							       
		Space                      = 0x20,
		PageUp                     = 0x21,
		PageDown                   = 0x22,
		End                        = 0x23,
		Home                       = 0x24,
		LeftArrow                  = 0x25,
		UpArrow                    = 0x26,
		RightArrow                 = 0x27,
		DownArrow                  = 0x28,
		Select                     = 0x29,
		Print                      = 0x2A,
		Execute                    = 0x2B,
		Snapshot                   = 0x2C,
		Insert                     = 0x2D,
		Delete                     = 0x2E,
		Help                       = 0x2F,

		D0                         = 0x30,
		D1                         = 0x31,
		D2                         = 0x32,
		D3                         = 0x33,
		D4                         = 0x34,
		D5                         = 0x35,
		D6                         = 0x36,
		D7                         = 0x37,
		D8                         = 0x38,
		D9                         = 0x39,

		A                          = 0x41,
		B                          = 0x42,
		C                          = 0x43,
		D                          = 0x44,
		E                          = 0x45,
		F                          = 0x46,
		G                          = 0x47,
		H                          = 0x48,
		I                          = 0x49,
		J                          = 0x4A,
		K                          = 0x4B,
		L                          = 0x4C,
		M                          = 0x4D,
		N                          = 0x4E,
		O                          = 0x4F,
		P                          = 0x50,
		Q                          = 0x51,
		R                          = 0x52,
		S                          = 0x53,
		T                          = 0x54,
		U                          = 0x55,
		V                          = 0x56,
		W                          = 0x57,
		X                          = 0x58,
		Y                          = 0x59,
		Z                          = 0x5A,

		LeftWindows                = 0x5B,
		RightWindows               = 0x5C,
		Applications               = 0x5D,

		Sleep                      = 0x5F,
							       
		Numpad0                    = 0x60,
		Numpad1                    = 0x61,
		Numpad2                    = 0x62,
		Numpad3                    = 0x63,
		Numpad4                    = 0x64,
		Numpad5                    = 0x65,
		Numpad6                    = 0x66,
		Numpad7                    = 0x67,
		Numpad8                    = 0x68,
		Numpad9                    = 0x69,
		Multiply                   = 0x6A,
		Add                        = 0x6B,
		Separator                  = 0x6C,
		Subtract                   = 0x6D,
		Decimal                    = 0x6E,
		Divide                     = 0x6F,
		F1                         = 0x70,
		F2                         = 0x71,
		F3                         = 0x72,
		F4                         = 0x73,
		F5                         = 0x74,
		F6                         = 0x75,
		F7                         = 0x76,
		F8                         = 0x77,
		F9                         = 0x78,
		F10                        = 0x79,
		F11                        = 0x7A,
		F12                        = 0x7B,
		F13                        = 0x7C,
		F14                        = 0x7D,
		F15                        = 0x7E,
		F16                        = 0x7F,
		F17                        = 0x80,
		F18                        = 0x81,
		F19                        = 0x82,
		F20                        = 0x83,
		F21                        = 0x84,
		F22                        = 0x85,
		F23                        = 0x86,
		F24                        = 0x87,

		NumLock                    = 0x90,
		Scroll                     = 0x91,

		LeftShift                  = 0xA0,
		RightShift                 = 0xA1,
		LeftControl                = 0xA2,
		RightControl               = 0xA3,
		LeftAlt                   = 0xA4,
		RightAlt                  = 0xA5,

		BrowserBack                = 0xA6,
		BrowserForward             = 0xA7,
		BrowserRefresh             = 0xA8,
		BrowserStop                = 0xA9,
		BrowserSearch              = 0xAA,
		BrowserFavorites           = 0xAB,
		BrowserHome                = 0xAC,
						           
		VolumeMute                 = 0xAD,
		VolumeDown                 = 0xAE,
		VolumeUp                   = 0xAF,
		MediaNext                  = 0xB0,
		MediaPrev                  = 0xB1,
		MediaStop                  = 0xB2,
		MediaPlay                  = 0xB3,
		LaunchMail                 = 0xB4,
		LaunchMediaSelect          = 0xB5,
		LaunchApp1                 = 0xB6,
		LaunchApp2                 = 0xB7,

		Oem1                       = 0xBA,   // ';:' for US  | 'üÜ' for German
		OemPlus                    = 0xBB,   // '+' any country
		OemComma                   = 0xBC,   // ',' any country
		OemMinus                   = 0xBD,   // '-' any country
		OemPeriod                  = 0xBE,   // '.' any country
		Oem2                       = 0xBF,   // '/?' for US  | '#'' for German
		Oem3                       = 0xC0,   // '`~' for US  | 'öÖ' for German
						           
		Oem4                       = 0xDB,  //  '[{' for US  | 'ß?' for German
		Oem5                       = 0xDC,  //  '\|' for US  | '^°' for German
		Oem6                       = 0xDD,  //  ']}' for US  | '´`' for German
		Oem7                       = 0xDE,  //  ''"' for US  | 'äÄ' for German
		Oem8                       = 0xDF,
						           
		OemAX                      = 0xE1,  //  'AX' key on Japanese AX kbd
		Oem102                     = 0xE2,  //  "<>" or "\|" on RT 102-key kbd.
		IcoHelp                    = 0xE3,  //  Help key on ICO
		Ico00                      = 0xE4,  //  00 key on ICO

		ProcessKey                 = 0xE5,
		Packet                     = 0xE7,

		Attnention                 = 0xF6,
		CrSel                      = 0xF7,
		ExSel                      = 0xF8,
		EraseEndOfFile             = 0xF9,
		Play                       = 0xFA,
		Zoom                       = 0xFB,
		NoName                     = 0xFC,
		Pa1                        = 0xFD,
		OemClear                   = 0xFE
	};
	
	inline std::string ToString(KeyCode key)
	{
		switch (key)
		{
			case KeyCode::BackSpace:         return "BackSpace";
			case KeyCode::Tab:               return "Tab";
			case KeyCode::Clear:             return "Clear";
			case KeyCode::Return:            return "Return";
			case KeyCode::Shift:             return "Shift";
			case KeyCode::Control:           return "Control";
			case KeyCode::Alt:               return "Alt";
			case KeyCode::Pause:             return "Pause";
			case KeyCode::Capital:           return "Capital";
			case KeyCode::Escape:            return "Escape";
			case KeyCode::Convert:           return "Convert";
			case KeyCode::Nonconvert:        return "Nonconvert";
			case KeyCode::Accept:            return "Accept";
			case KeyCode::Modechange:        return "Modechange";
			case KeyCode::Space:             return "Space";
			case KeyCode::PageUp:            return "PageUp";
			case KeyCode::PageDown:          return "PageDown";
			case KeyCode::End:               return "End";
			case KeyCode::Home:              return "Home";
			case KeyCode::LeftArrow:         return "LeftArrow";
			case KeyCode::UpArrow:           return "UpArrow";
			case KeyCode::RightArrow:        return "RightArrow";
			case KeyCode::DownArrow:         return "DownArrow";
			case KeyCode::Select:            return "Select";
			case KeyCode::Print:             return "Print";
			case KeyCode::Execute:           return "Execute";
			case KeyCode::Snapshot:          return "Snapshot";
			case KeyCode::Insert:            return "Insert";
			case KeyCode::Delete:            return "Delete";
			case KeyCode::Help:              return "Help";
			case KeyCode::D0:                return "D0";
			case KeyCode::D1:                return "D1";
			case KeyCode::D2:                return "D2";
			case KeyCode::D3:                return "D3";
			case KeyCode::D4:                return "D4";
			case KeyCode::D5:                return "D5";
			case KeyCode::D6:                return "D6";
			case KeyCode::D7:                return "D7";
			case KeyCode::D8:                return "D8";
			case KeyCode::D9:                return "D9";
			case KeyCode::A:                 return "A";
			case KeyCode::B:                 return "B";
			case KeyCode::C:                 return "C";
			case KeyCode::D:                 return "D";
			case KeyCode::E:                 return "E";
			case KeyCode::F:                 return "F";
			case KeyCode::G:                 return "G";
			case KeyCode::H:                 return "H";
			case KeyCode::I:                 return "I";
			case KeyCode::J:                 return "J";
			case KeyCode::K:                 return "K";
			case KeyCode::L:                 return "L";
			case KeyCode::M:                 return "M";
			case KeyCode::N:                 return "N";
			case KeyCode::O:                 return "O";
			case KeyCode::P:                 return "P";
			case KeyCode::Q:                 return "Q";
			case KeyCode::R:                 return "R";
			case KeyCode::S:                 return "S";
			case KeyCode::T:                 return "T";
			case KeyCode::U:                 return "U";
			case KeyCode::V:                 return "V";
			case KeyCode::W:                 return "W";
			case KeyCode::X:                 return "X";
			case KeyCode::Y:                 return "Y";
			case KeyCode::Z:                 return "Z";
			case KeyCode::LeftWindows:       return "LeftWindows";
			case KeyCode::RightWindows:      return "RightWindows";
			case KeyCode::Applications:      return "Applications";
			case KeyCode::Sleep:             return "Sleep";
			case KeyCode::Numpad0:           return "Numpad0";
			case KeyCode::Numpad1:           return "Numpad1";
			case KeyCode::Numpad2:           return "Numpad2";
			case KeyCode::Numpad3:           return "Numpad3";
			case KeyCode::Numpad4:           return "Numpad4";
			case KeyCode::Numpad5:           return "Numpad5";
			case KeyCode::Numpad6:           return "Numpad6";
			case KeyCode::Numpad7:           return "Numpad7";
			case KeyCode::Numpad8:           return "Numpad8";
			case KeyCode::Numpad9:           return "Numpad9";
			case KeyCode::Multiply:          return "Multiply";
			case KeyCode::Add:               return "Add";
			case KeyCode::Separator:         return "Separator";
			case KeyCode::Subtract:          return "Subtract";
			case KeyCode::Decimal:           return "Decimal";
			case KeyCode::Divide:            return "Divide";
			case KeyCode::F1:                return "F1";
			case KeyCode::F2:                return "F2";
			case KeyCode::F3:                return "F3";
			case KeyCode::F4:                return "F4";
			case KeyCode::F5:                return "F5";
			case KeyCode::F6:                return "F6";
			case KeyCode::F7:                return "F7";
			case KeyCode::F8:                return "F8";
			case KeyCode::F9:                return "F9";
			case KeyCode::F10:               return "F10";
			case KeyCode::F11:               return "F11";
			case KeyCode::F12:               return "F12";
			case KeyCode::F13:               return "F13";
			case KeyCode::F14:               return "F14";
			case KeyCode::F15:               return "F15";
			case KeyCode::F16:               return "F16";
			case KeyCode::F17:               return "F17";
			case KeyCode::F18:               return "F18";
			case KeyCode::F19:               return "F19";
			case KeyCode::F20:               return "F20";
			case KeyCode::F21:               return "F21";
			case KeyCode::F22:               return "F22";
			case KeyCode::F23:               return "F23";
			case KeyCode::F24:               return "F24";
			case KeyCode::NumLock:           return "NumLock";
			case KeyCode::Scroll:            return "Scroll";
			case KeyCode::LeftShift:         return "LeftShift";
			case KeyCode::RightShift:        return "RightShift";
			case KeyCode::LeftControl:       return "LeftControl";
			case KeyCode::RightControl:      return "RightControl";
			case KeyCode::LeftAlt:           return "LeftAlt";
			case KeyCode::RightAlt:          return "RightAlt";
			case KeyCode::BrowserBack:       return "BrowserBack";
			case KeyCode::BrowserForward:    return "BrowserForward";
			case KeyCode::BrowserRefresh:    return "BrowserRefresh";
			case KeyCode::BrowserStop:       return "BrowserStop";
			case KeyCode::BrowserSearch:     return "BrowserSearch";
			case KeyCode::BrowserFavorites:  return "BrowserFavorites";
			case KeyCode::BrowserHome:       return "BrowserHome";
			case KeyCode::VolumeMute:        return "VolumeMute";
			case KeyCode::VolumeDown:        return "VolumeDown";
			case KeyCode::VolumeUp:          return "VolumeUp";
			case KeyCode::MediaNext:         return "MediaNext";
			case KeyCode::MediaPrev:         return "MediaPrev";
			case KeyCode::MediaStop:         return "MediaStop";
			case KeyCode::MediaPlay:         return "MediaPlay";
			case KeyCode::LaunchMail:        return "LaunchMail";
			case KeyCode::LaunchMediaSelect: return "LaunchMediaSelect";
			case KeyCode::LaunchApp1:        return "LaunchApp1";
			case KeyCode::LaunchApp2:        return "LaunchApp2";
			case KeyCode::Oem1:              return "Oem1";
			case KeyCode::OemPlus:           return "OemPlus";
			case KeyCode::OemComma:          return "OemComma";
			case KeyCode::OemMinus:          return "OemMinus";
			case KeyCode::OemPeriod:         return "OemPeriod";
			case KeyCode::Oem2:              return "Oem2";
			case KeyCode::Oem3:              return "Oem3";
			case KeyCode::Oem4:              return "Oem4";
			case KeyCode::Oem5:              return "Oem5";
			case KeyCode::Oem6:              return "Oem6";
			case KeyCode::Oem7:              return "Oem7";
			case KeyCode::Oem8:              return "Oem8";
			case KeyCode::OemAX:             return "OemAX";
			case KeyCode::Oem102:            return "Oem102";
			case KeyCode::IcoHelp:           return "IcoHelp";
			case KeyCode::Ico00:             return "Ico00";
			case KeyCode::ProcessKey:        return "ProcessKey";
			case KeyCode::Packet:            return "Packet";
			case KeyCode::Attnention:        return "Attnention";
			case KeyCode::CrSel:             return "CrSel";
			case KeyCode::ExSel:             return "ExSel";
			case KeyCode::EraseEndOfFile:    return "EraseEndOfFile";
			case KeyCode::Play:              return "Play";
			case KeyCode::Zoom:              return "Zoom";
			case KeyCode::NoName:            return "NoName";
			case KeyCode::Pa1:               return "Pa1";
			case KeyCode::OemClear:          return "OemClear";
		}

		return "Unkown";
	}

	struct ModifierKeys
	{
		bool Shift = false;
		bool Alt = false;
		bool Control = false;
	};

}

namespace magic_enum::customize {
	template<>
	struct enum_range<Shark::KeyCode> {
		static constexpr int min = 0;
		static constexpr int max = 255;
	};
}
