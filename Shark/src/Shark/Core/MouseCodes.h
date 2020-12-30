#pragma once

namespace Shark {

	using MouseCode = uint16_t;

	namespace Mouse {

		enum : MouseCode
		{
			LeftButton		= 0x01,
			RightButton		= 0x02,
			Cancel			= 0x03,
			Middle	        = 0x04,
			XButton1		= 0x05,
			XButton2		= 0x06
		};

	}

}