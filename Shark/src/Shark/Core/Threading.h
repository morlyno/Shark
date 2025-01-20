#pragma once

#include "Shark/Utils/PlatformUtils.h"

#include <tracy/Tracy.hpp>

namespace Shark {

	struct Uninitialized {};
	static constexpr Uninitialized UninitializedTag;

	namespace Threading {

		namespace Internal {

#if SK_PLATFORM_WINDOWS
			using NativeMutex = CRITICAL_SECTION;
			using NativeConditionVariable = CONDITION_VARIABLE;
#endif

		}

		class Thread
		{
		public:
			Thread() = default;
			Thread(std::string_view name);
			Thread(const Thread&) = delete;
			Thread& operator=(const Thread&) = delete;
			Thread(Thread&&) = default;
			Thread& operator=(Thread&&) = default;
			~Thread();

			template<typename TFunc, typename... TArgs>
				requires(!std::is_convertible_v<TFunc, std::string_view>)
			Thread(TFunc&& func, TArgs&&... args);

			template<typename TFunc, typename... TArgs>
			Thread(std::string_view name, TFunc&& func, TArgs&&... args);

			void SetName(std::string_view name);
			bool Running() const;

			template<typename TFunc, typename... TArgs>
			void Dispacht(TFunc&& func, TArgs&&... args);

			void RequestStop();
			void Join();
			void StopAndJoin();

		private:
			std::string m_Name;
			std::jthread m_Thread;
		};

		class Mutex
		{
		public:
			Mutex();
			~Mutex();

			Mutex(const Mutex&) = delete;
			Mutex& operator= (const Mutex&) = delete;

			void Lock();
			bool TryLock();
			void Unlock();

			Internal::NativeMutex* GetNativeMutex() { return &m_NativeMutex; }
		private:
			Internal::NativeMutex m_NativeMutex;

		private: // interface for std::unique_lock
			void lock() { Lock(); }
			void unlock() { Unlock(); }
			template<typename TMutex>
			friend class std::unique_lock;
		};

		namespace Internal {
			class TrackedMutex
			{
			public:
				TrackedMutex(const tracy::SourceLocationData* srcloc);
				~TrackedMutex();

				TrackedMutex(const TrackedMutex&) = delete;
				TrackedMutex& operator= (const TrackedMutex&) = delete;

				void Lock();
				bool TryLock();
				void Unlock();

				Internal::NativeMutex* GetNativeMutex() { return m_Mutex.GetNativeMutex(); }
			public:
				void lock() { Lock(); }
				void unlock() { Unlock(); }
				bool try_lock() { TryLock(); }
			private:
				tracy::LockableCtx m_Context;
				Mutex m_Mutex;
			};
		}

#if defined(TRACY_ENABLE)
		using TrackedMutex = Internal::TrackedMutex;
		#define SKLockableInit( varname ) ::Shark::Threading::TrackedMutex { [] () -> const tracy::SourceLocationData* { static constexpr tracy::SourceLocationData srcloc { nullptr, "Threading::TrackedMutex " #varname, TracyFile, TracyLine, 0 }; return &srcloc; }() }
#else
		using TrackedMutex = Mutex;
		#define SKLockableInit( varname ) ::Shark::Threading::Mutex()
#endif

		class ConditionVariable
		{
		public:
			ConditionVariable();
			~ConditionVariable();

			ConditionVariable(const ConditionVariable&) = delete;
			ConditionVariable& operator= (const ConditionVariable&) = delete;

			void NotifyOne();
			void NotifyAll();

			void Wait(std::unique_lock<Mutex>& lock);
			void Wait(std::unique_lock<Mutex>& lock, std::chrono::milliseconds time);
			
			template<typename TPredicate, typename... TArgs>
			void Wait(std::unique_lock<Mutex>& lock, TPredicate predicate, TArgs&&... valueOrInstance);

			template<typename TPredicate, typename... TArgs>
			void Wait(std::unique_lock<Mutex>& lock, std::chrono::milliseconds time, TPredicate predicate, TArgs&&... valueOrInstance);

		private:
			Internal::NativeConditionVariable m_ConditionVariable;
		};

		class ProcessSignal
		{
		public:
			ProcessSignal(Uninitialized);
			ProcessSignal(bool manualReset, bool initialState);
			~ProcessSignal();

			ProcessSignal(ProcessSignal&& other);
			ProcessSignal& operator=(ProcessSignal&& other);

			ProcessSignal(const ProcessSignal&) = delete;
			ProcessSignal& operator=(const ProcessSignal&) = delete;

			void Set();
			void Reset();

			void Wait();
			void Wait(std::chrono::milliseconds time);

			operator bool() const;

		private:
			NativeHandle m_Handle = 0;
		};

		class ThreadSignal
		{
		public:
			ThreadSignal(bool manualReset = false, bool initialState = false);
			~ThreadSignal();

			ThreadSignal(const ThreadSignal&) = delete;
			ThreadSignal& operator= (const ThreadSignal&) = delete;

			void Notify();
			void Reset();

			void Wait();
			void Wait(std::chrono::milliseconds time);

		private:
			bool m_Signaled = false;
			bool m_ManualReset = false;

			Mutex m_Mutex;
			ConditionVariable m_ConditionVariable;
		};

		template<typename T>
		class Future
		{
		private:
			class FutureState
			{
			public:
				T m_Value;
				ThreadSignal m_FinishedEvent;
				std::vector<std::function<void(T)>> m_OnReadyCallbacks;
				std::atomic<bool> m_Finished = false;
				mutable std::mutex m_Mutex;

				FutureState(bool signaled);
				~FutureState() = default;

				FutureState(const FutureState&) = delete;
				FutureState& operator=(const FutureState&) = delete;
			};

		public:
			Future() = default;
			~Future() = default;

			Future(bool createState);
			explicit Future(T result);

			void MergeCallbacks(const Future& other);

			bool Valid() const { m_State != nullptr; }
			bool Ready() const { return m_State->m_Finished; }


			void Set(const T& value);
			void Signal(bool wake = true, bool callback = true);
			void SetAndSignal(const T& value);


			void Wait();
			void Wait(std::chrono::milliseconds milliseconds);

			const T& WaitAndGet();
			const T& Get();

			void OnReady(auto func);

		private:
			std::shared_ptr<FutureState> m_State;

			template<typename T>
			friend class Promise;
		};

	}

}

#include "Shark/Core/Threading.inl"
