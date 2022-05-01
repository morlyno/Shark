#pragma once

namespace Shark {

	namespace MouseButton {

		enum Type : uint16_t
		{
			Left,
			Right,
			Middle,
			Thumb01,
			Thumb02,

			Invalid
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