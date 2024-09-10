#pragma once

namespace Shark {

	enum class MouseButton : uint16_t
	{
		None     = 0x00,

		Left        = 0x01,
		Right       = 0x02,
		Middle      = 0x04,
		Thumb01     = 0x05,
		Thumb02     = 0x06
	};

	enum class CursorMode
	{
		Normal,
		Hidden,
		Locked
	};

}