#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Application.h"
#include "Shark/Core/TimeStep.h"
#include "Shark/Utils/PlatformUtils.h"

#include <optick.h>

namespace Shark {

	struct FrameData
	{
		float Duration = 0.0f;
		std::string_view Descriptor;
	};

	class PerformanceProfiler
	{
	public:
		PerformanceProfiler() = default;
		~PerformanceProfiler() = default;

		void Clear();
		void Add(std::string_view descriptor, float duration);

		FrameData* GetStorage(std::string_view descriptor);
		const auto& GetFrameStorage() const { return m_FrameStorage; }

	private:
		std::map<std::string_view, FrameData> m_FrameStorage;
	};

	struct ProfilerEvent
	{
		ProfilerEvent(PerformanceProfiler* profiler, std::string_view descriptor);
		~ProfilerEvent();

	private:
		int64_t m_Start;
		int64_t m_Stop;
		FrameData* Storage = nullptr;
	};

}

#if SK_ENABLE_PERF
#define SK_PERF_SCOPED(name) ::Shark::ProfilerEvent SK_CONNECT(eventAutoGenName, __LINE__) = ::Shark::ProfilerEvent{ Application::Get().GetProfiler(), name }
#define SK_PERF_FUNCTION() SK_PERF_SCOPED(SK_FUNCTION_DECORATED)
#else
#define SK_PERF_SCOPED(...)
#define SK_PERF_FUNCTION(...)
#endif


#define SK_PROFILE_FRAME(name, ...) OPTICK_FRAME(name, __VA_ARGS__)
#define SK_PROFILE_THREAD(name) OPTICK_THREAD(name)
#define SK_PROFILE_FUNCTION() OPTICK_EVENT()
#define SK_PROFILE_SCOPED(name) OPTICK_EVENT(name)
#define SK_PROFILE_SHUTDOWN() OPTICK_SHUTDOWN()
