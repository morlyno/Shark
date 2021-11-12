#pragma once

namespace Shark {

	using KeyCode = uint16_t;

	namespace Key {

		enum : KeyCode
		{
			Erase					= 0x08,
			Tab						= 0x09,
			NumpadOff5				= 0x0C,
			Enter					= 0x0D,
			LeftShift				= 0x10,
			Control					= 0x11,
			Alt						= 0x12,
			Pause					= 0x13,
			Cap						= 0x14,
			Escape					= 0x1b,

			Space					= 0x20,
			BildUp					= 0x21,
			BildDown				= 0x22,
			End						= 0x23,
			Pos1					= 0x24,
			Left					= 0x25,
			Up						= 0x26,
			Right					= 0x27,
			Down					= 0x28,
			Insert					= 0x2D,
			Entf					= 0x2E,

			D0						= 0x30,
			D1						= 0x31,
			D2						= 0x32,
			D3						= 0x33,
			D4						= 0x34,
			D5						= 0x35,
			D6						= 0x36,
			D7						= 0x37,
			D8						= 0x38,
			D9						= 0x39,

			A						= 0x41,
			B						= 0x42,
			C						= 0x43,
			D						= 0x44,
			E						= 0x45,
			F						= 0x46,
			G						= 0x47,
			H						= 0x48,
			I						= 0x49,
			J						= 0x4A,
			K						= 0x4B,
			L						= 0x4C,
			M						= 0x4D,
			N						= 0x4E,
			O						= 0x4F,
			P						= 0x50,
			Q						= 0x51,
			R						= 0x52,
			S						= 0x53,
			T						= 0x54,
			U						= 0x55,
			V						= 0x56,
			W						= 0x57,
			X						= 0x58,
			Y						= 0x59,
			Z						= 0x5A,

			Windows					= 0x5B,

			Numpad0					= 0x60,
			Numpad1					= 0x61,
			Numpad2					= 0x62,
			Numpad3					= 0x63,
			Numpad4					= 0x64,
			Numpad5					= 0x65,
			Numpad6					= 0x66,
			Numpad7					= 0x67,
			Numpad8					= 0x68,
			Numpad9					= 0x69,

			NumpadMul				= 0x6A,
			NumpadPlus				= 0x6B,
			NumpadMinus				= 0x6D,
			NumpadComma				= 0x6E,
			NumpadDived				= 0x6F,

			F1						= 0x70,
			F2						= 0x71,
			F3						= 0x72,
			F4						= 0x73,
			F5						= 0x74,
			F6						= 0x75,
			F7						= 0x76,
			F8						= 0x77,
			F9						= 0x78,
			F10						= 0x79,
			F11						= 0x7A,
			F12						= 0x7B,

			Numpad					= 0x90,

			Plus					= 0xBB,
			Comma					= 0xBC,
			Dot						= 0xBE,
			Minus					= 0xBD,
			Hastag					= 0xBF,


		};

	}

	inline std::string KeyToString(KeyCode key)
	{
		switch (key)
		{
			case Key::Erase:               return "Erase";
			case Key::Tab:                 return "Tab";
			case Key::NumpadOff5:   	   return "NumpadOff5";
			case Key::Enter:			   return "Enter";
			case Key::LeftShift:		   return "LeftShift";
			case Key::Control:			   return "Control";
			case Key::Alt:				   return "Alt";
			case Key::Pause:			   return "Pause";
			case Key::Cap:				   return "Cap";
			case Key::Escape:			   return "Escape";
			case Key::Space:			   return "Space";
			case Key::BildUp:			   return "BildUp";
			case Key::BildDown:			   return "BildDown";
			case Key::End:				   return "End";
			case Key::Pos1:				   return "Pos1";
			case Key::Left:				   return "Left";
			case Key::Up:				   return "Up";
			case Key::Right:			   return "Right";
			case Key::Down:                return "Down";
			case Key::Insert:              return "Insert";
			case Key::Entf:                return "Entf";
			case Key::D0:                  return "D0";
			case Key::D1:                  return "D1";
			case Key::D2:                  return "D2";
			case Key::D3:                  return "D3";
			case Key::D4:                  return "D4";
			case Key::D5:                  return "D5";
			case Key::D6:                  return "D6";
			case Key::D7:                  return "D7";
			case Key::D8:                  return "D8";
			case Key::D9:                  return "D9";
			case Key::A:                   return "A";
			case Key::B:                   return "B";
			case Key::C:                   return "C";
			case Key::D:                   return "D";
			case Key::E:                   return "E";
			case Key::F:                   return "F";
			case Key::G:                   return "G";
			case Key::H:                   return "H";
			case Key::I:                   return "I";
			case Key::J:                   return "J";
			case Key::K:                   return "K";
			case Key::L:                   return "L";
			case Key::M:                   return "M";
			case Key::N:                   return "N";
			case Key::O:                   return "O";
			case Key::P:                   return "P";
			case Key::Q:                   return "Q";
			case Key::R:                   return "R";
			case Key::S:                   return "S";
			case Key::T:                   return "T";
			case Key::U:                   return "U";
			case Key::V:                   return "V";
			case Key::W:                   return "W";
			case Key::X:                   return "X";
			case Key::Y:                   return "Y";
			case Key::Z:                   return "Z";
			case Key::Windows:             return "Windows";
			case Key::Numpad0:             return "Numpad0";
			case Key::Numpad1:             return "Numpad1";
			case Key::Numpad2:             return "Numpad2";
			case Key::Numpad3:             return "Numpad3";
			case Key::Numpad4:             return "Numpad4";
			case Key::Numpad5:             return "Numpad5";
			case Key::Numpad6:             return "Numpad6";
			case Key::Numpad7:             return "Numpad7";
			case Key::Numpad8:             return "Numpad8";
			case Key::Numpad9:             return "Numpad9";
			case Key::NumpadMul:           return "NumpadMul";
			case Key::NumpadPlus:          return "NumpadPlus";
			case Key::NumpadMinus:         return "NumpadMinus";
			case Key::NumpadComma:         return "NumpadComma";
			case Key::NumpadDived:         return "NumpadDived";
			case Key::F1:                  return "F1";
			case Key::F2:                  return "F2";
			case Key::F3:                  return "F3";
			case Key::F4:                  return "F4";
			case Key::F5:                  return "F5";
			case Key::F6:                  return "F6";
			case Key::F7:                  return "F7";
			case Key::F8:                  return "F8";
			case Key::F9:                  return "F9";
			case Key::F10:                 return "F10";
			case Key::F11:                 return "F11";
			case Key::F12:                 return "F12";
			case Key::Numpad:              return "Numpad";
			case Key::Plus:                return "Plus";
			case Key::Comma:               return "Comma";
			case Key::Dot:                 return "Dot";
			case Key::Minus:               return "Minus";
			case Key::Hastag:              return "Hastag";
		}
		SK_CORE_ASSERT(false, "Unkown Key");
		return "Unkown";
	}

}