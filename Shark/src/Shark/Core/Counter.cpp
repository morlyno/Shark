#include "skpch.h"
#include "Counter.h"

namespace Shark {

	struct Entry
	{
		double Time;
		bool Repead;
		std::function<void()> Func;

		bool Active;
		double TimePassed;
		std::string Tag;

		Entry(double time, bool repead, const std::function<void()>& func, const std::string& tag)
			: Time(time), Repead(repead), Func(func), TimePassed(0.0), Active(false), Tag(tag)
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

	void Counter::SetActivce(const std::string& tag, bool active)
	{
		const auto& it = Utils::Find(tag);
		SK_CORE_ASSERT(it != s_Counters.end());
		if (it != s_Counters.end())
			it->Active = active;
	}

	bool Counter::IsPaused(const std::string& tag)
	{
		const auto& it = Utils::Find(tag);
		SK_CORE_ASSERT(it != s_Counters.end());
		if (it != s_Counters.end())
			return it->Active;
		return false;
	}

	double Counter::GetTime(const std::string& tag)
	{
		const auto& it = Utils::Find(tag);
		SK_CORE_ASSERT(it != s_Counters.end());
		if (it != s_Counters.end())
			return it->Time;
		return 0.0f;
	}

	void Counter::SetTime(const std::string& tag, double time)
	{
		const auto& it = Utils::Find(tag);
		SK_CORE_ASSERT(it != s_Counters.end());
		if (it != s_Counters.end() && time > 0.0f)
			it->Time = time;
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
			if (entry.Active)
			{
				entry.TimePassed += ts;
				if (entry.TimePassed >= entry.Time)
				{
					SK_IF_DEBUG(
						if (int n = (int)(entry.TimePassed / entry.Time); n >= 2)
							SK_CORE_INFO("{0} Callbackes of counter {1} are skipped", n - 1, entry.Tag);
					);
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
