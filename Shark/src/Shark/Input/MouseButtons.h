#pragma once

namespace Shark {

	namespace MouseButton {

		enum Type : uint16_t
		{
			Left        = 0x01,
			Right       = 0x02,
			Middle      = 0x04,
			Thumb01     = 0x05,
			Thumb02     = 0x06,

			Invalid     = 0x00
		};

	}

	inline std::string ToString(MouseButton::Type mouseButton)
	{
		switch (mouseButton)
		{
			case MouseButton::Left:     return "Left";
			case MouseButton::Right:    return "Right";
			case MouseButton::Middle:   return "Middle";
			case MouseButton::Thumb01:  return "Thumb01";
			case MouseButton::Thumb02:  return "Thumb02";
			case MouseButton::Invalid:  return "Invalid";
		}

		return "Unkown";
	}

}