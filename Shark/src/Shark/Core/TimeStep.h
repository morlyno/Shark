#pragma once

#include <StrToNum.h>

namespace Shark {

	class TimeStep
	{
	public:
		TimeStep(float time = 0.0f)
			: m_Time(time)
		{}

		template<typename Rep, typename Period>
		TimeStep(std::chrono::duration<Rep, Period> dur)
			: TimeStep(TimeStep::FromDuration(dur))
		{}

		operator float() const { return m_Time; }

		float Seconds() const { return m_Time; }
		float MilliSeconds() const { return m_Time * 1000.0f; }
		float MicroSeconds() const { return m_Time * (1000.0f * 1000.0f); }
		float NanoSeconds() const { return m_Time * (1000.0f * 1000.0f * 1000.0f); }

		float s() const { return Seconds(); }
		float ms() const { return MilliSeconds(); }
		float us() const { return MicroSeconds(); }
		float ns() const { return NanoSeconds(); }

	public:
		std::string ToString() const;
		std::string ToString(uint32_t precision) const;

	public:
		TimeStep operator+(const TimeStep& rhs) const { return m_Time + rhs.m_Time; }
		TimeStep operator-(const TimeStep& rhs) const { return m_Time - rhs.m_Time; }
		TimeStep operator*(const TimeStep& rhs) const { return m_Time * rhs.m_Time; }
		TimeStep operator/(const TimeStep& rhs) const { return m_Time / rhs.m_Time; }
		TimeStep operator*(float scale) const { return m_Time * scale; }
		TimeStep operator/(float scale) const { return m_Time / scale; }

		TimeStep& operator+=(const TimeStep& rhs) { m_Time += rhs.m_Time; return *this; }
		TimeStep& operator-=(const TimeStep& rhs) { m_Time -= rhs.m_Time; return *this; }
		TimeStep& operator*=(const TimeStep& rhs) { m_Time *= rhs.m_Time; return *this; }
		TimeStep& operator/=(const TimeStep& rhs) { m_Time /= rhs.m_Time; return *this; }
		TimeStep& operator*=(float scale) { m_Time *= scale; return *this; }
		TimeStep& operator/=(float scale) { m_Time /= scale; return *this; }

	public:
		static TimeStep FromSeconds(float seconds) { return TimeStep(seconds); }
		static TimeStep FromMilliSeconds(float ms) { return TimeStep(ms * 0.001f); }
		static TimeStep FromMicroSeconds(float us) { return TimeStep(us * (0.001f * 0.001f)); }
		static TimeStep FromNanoSeconds(float ns)  { return TimeStep(ns * (0.001f * 0.001f * 0.001f)); }

		static TimeStep FromDuration(std::chrono::seconds t)      { return TimeStep::FromSeconds((float)t.count()); }
		static TimeStep FromDuration(std::chrono::milliseconds t) { return TimeStep::FromMilliSeconds((float)t.count()); }
		static TimeStep FromDuration(std::chrono::microseconds t) { return TimeStep::FromMicroSeconds((float)t.count()); }
		static TimeStep FromDuration(std::chrono::nanoseconds t)  { return TimeStep::FromNanoSeconds((float)t.count()); }

	private:
		float m_Time;
	};

}

template<>
struct fmt::formatter<Shark::TimeStep>
{
private:
	uint32_t m_Precision = 6;

public:
	constexpr auto parse(format_parse_context& ctx) -> format_parse_context::iterator
	{
		auto end = std::find(ctx.begin(), ctx.end(), '}');
		if (end != ctx.end())
		{
			std::string_view range = { ctx.begin(), end };
			auto dot = range.find('.');
			if (dot == std::string_view::npos)
				return ctx.end();

			auto result = stn::StrToInt32(range.substr(dot + 1));
			m_Precision = result.value_or(6);
		}
		return ctx.end();
	}

	template<typename FormatContext>
	auto format(const Shark::TimeStep& timestep, FormatContext& ctx) const -> FormatContext::iterator
	{
		return fmt::format_to(ctx.out(), "{0}", timestep.ToString(m_Precision));
	}
};