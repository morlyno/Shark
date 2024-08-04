#include "skpch.h"
#include "TimeStep.h"

namespace Shark {

	std::string TimeStep::ToString() const
	{
		if (m_Time > 60.0f)
		{
			float minits = m_Time / 60.0f;
			return fmt::format("{0:3.6f}m", minits);
		}

		float t = m_Time;
		if (t > 1.0f)
			return fmt::format("{0:3.6f}s", t);

		t *= 1000.0f;
		if (t > 0.1f)
			return fmt::format("{0:3.6f}ms", t);

		t *= 1000.0f;
		if (t > 0.1f)
			return fmt::format("{0:3.6f}us", t);

		t *= 1000.0f;
		return fmt::format("{0:3.6f}ns", t);
	}

	std::string TimeStep::ToString(uint32_t precision) const
	{
		if (m_Time > 60.0f)
		{
			float minits = m_Time / 60.0f;
			return fmt::format("{:.{}f}m", minits, precision);
		}

		float t = m_Time;
		if (t > 1.0f)
			return fmt::format("{:.{}f}s", t, precision);

		t *= 1000.0f;
		if (t > 0.1f)
			return fmt::format("{:.{}f}ms", t, precision);

		t *= 1000.0f;
		if (t > 0.1f)
			return fmt::format("{:.{}f}us", t, precision);

		t *= 1000.0f;
		return fmt::format("{:.{}f}ns", t, precision);
	}

}
