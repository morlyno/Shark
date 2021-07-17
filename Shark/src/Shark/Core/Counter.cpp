#include "skpch.h"
#include "Counter.h"

namespace Shark {

	struct Entry
	{
		double Time;
		bool Repead;
		std::function<void()> Func;

		bool Paused;
		double TimePassed;
		std::string Tag;

		Entry(double time, bool repead, const std::function<void()>& func, const std::string& tag)
			: Time(time), Repead(repead), Func(func), TimePassed(0.0), Paused(false), Tag(tag)
		{}
	};

	static std::vector<Entry> s_Counters;

	namespace Utils {

		static decltype(auto) Find(const std::string& tag)
		{
			return std::find_if(s_Counters.begin(), s_Counters.end(), [&tag](auto& entry)
			{
				return entry.Tag == tag;
			});
		}
		
	}

	void Counter::Add(const std::string& tag, double time, bool repead, const std::function<void()>& func)
	{
		s_Counters.emplace_back(time, repead, func, tag);
	}

	void Counter::SetPause(const std::string& tag, bool pause)
	{
		const auto& it = Utils::Find(tag);
		SK_CORE_ASSERT(it != s_Counters.end());
		if (it != s_Counters.end())
			it->Paused = pause;
	}

	bool Counter::IsPaused(const std::string& tag)
	{
		const auto& it = Utils::Find(tag);
		SK_CORE_ASSERT(it != s_Counters.end());
		if (it != s_Counters.end())
			return it->Paused;
		return false;
	}

	bool Counter::Remove(const std::string& tag)
	{
		const auto& it = std::find_if(s_Counters.begin(), s_Counters.end(), [&tag](auto& entry) -> bool
		{
			return entry.Tag == tag;
		});

		if (it != s_Counters.end())
		{
			s_Counters.erase(it);
			return true;
		}
		return false;
	}

	void Counter::Update(TimeStep ts)
	{
		for (size_t i = 0; i < s_Counters.size();)
		{
			auto& entry = s_Counters[i];
			if (!entry.Paused)
			{
				entry.TimePassed += ts;
				if (entry.TimePassed >= entry.Time)
				{
					entry.Func();
					entry.TimePassed = fmod(entry.TimePassed, entry.Time);
					if (!entry.Repead)
					{
						s_Counters.erase(s_Counters.begin() + i);
						continue;
					}
				}
			}
			i++;
		}
	}

}
