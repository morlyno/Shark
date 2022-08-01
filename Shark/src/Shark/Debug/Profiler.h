#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/TimeStep.h"

#include <optick.h>

#include <queue>

namespace Shark {

	class PerformanceTimer;

	class PerformanceProfiler
	{
	public:
		using TimerStorage = std::unordered_map<std::string_view, float>;

	public:
		static void Initialize();
		static void Shutdown();

		static void SetSampleRate(uint32_t frameCount);
		static uint32_t GetSampleRate();

		static void NewFrame();
		static void Push(std::string_view key, float time);

		static const TimerStorage& GetStatistics();

	private:
		static TimerStorage& GetStorage();

	private:
		uint64_t Frame = 0;
		uint32_t StorageIndex = 0;
		TimerStorage TimerStorageBuffers[2];
		TimerStorage* ActiveStorage = nullptr;
		TimerStorage* FinishedSotrage = nullptr;

		uint32_t SampleRate = 10;

		float FrameStartTime = 0.0f;

		static PerformanceProfiler* s_Instance;
	};

	class PerformanceTimer
	{
	public:
		PerformanceTimer(std::string_view key);
		~PerformanceTimer();

		void Start();
		void Stop();

	private:
		std::string_view m_Key;
		float m_Start;
	};

}

#if SK_ENABLE_PERF
#define SK_PERF_NEW_FRAME()  ::Shark::PerformanceProfiler::NewFrame()

#define SK_PERF_SCOPED(name) ::Shark::PerformanceTimer SK_CONNECT(performanceTimerAutoGenName, __LINE__) { name };
#define SK_PERF_FUNCTION()   SK_PERF_SCOPED(SK_FUNCTION_DECORATED)
#else
#define SK_PERF_NEW_FRAME(...)

#define SK_PERF_SCOPED(...)
#define SK_PERF_FUNCTION(...)
#endif


#define SK_PROFILE_FRAME(name, ...) OPTICK_FRAME(name, __VA_ARGS__)
#define SK_PROFILE_THREAD(name) OPTICK_THREAD(name)
#define SK_PROFILE_FUNCTION() OPTICK_EVENT()
#define SK_PROFILE_SCOPED(name) OPTICK_EVENT(name)
#define SK_PROFILE_SHUTDOWN() OPTICK_SHUTDOWN()
