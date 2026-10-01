#pragma once

#include "Shark/Utils/PlatformUtils.h"

#if 0
#if SK_ENABLE_PROFILER
	#include <tracy/Tracy.hpp>
#endif
#endif

namespace Shark {

	namespace Threading {

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

			std::mutex m_Mutex;
			std::condition_variable m_ConditionVariable;
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

			bool Valid() const { return m_State != nullptr; }
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

namespace Shark::Threading {

	///////////////////////////////////////////////////////////////////////////////////////////////
	//// Future ///////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////

	template<typename T>
	Future<T>::FutureState::FutureState(bool signaled)
		: m_FinishedEvent(true, signaled)
	{
	}

	template<typename T>
	Future<T>::Future(bool createState)
	{
		if (createState)
		{
			m_State = std::make_shared<FutureState>(false);
		}
	}

	template<typename T>
	Future<T>::Future(T result)
	{
		m_State = std::make_shared<FutureState>(true);
		m_State->m_Value = result;
		m_State->m_Finished = true;
	}

	template<typename T>
	void Future<T>::MergeCallbacks(const Future& other)
	{
		if (m_State == other.m_State)
			return;

		std::scoped_lock lock(m_State->m_Mutex, other.m_State->m_Mutex);
		m_State->m_OnReadyCallbacks.insert(m_State->m_OnReadyCallbacks.end(), other.m_State->m_OnReadyCallbacks.begin(), other.m_State->m_OnReadyCallbacks.end());
	}

	template<typename T>
	void Future<T>::Set(const T& value)
	{
		SK_CORE_VERIFY(m_State->m_Finished == false);
		m_State->m_Value = value;
		m_State->m_Finished = true;
	}

	template<typename T>
	void Future<T>::Signal(bool wake, bool callback)
	{
		if (wake)
		{
			m_State->m_FinishedEvent.Notify();
		}

		if (callback)
		{
			std::scoped_lock lock(m_State->m_Mutex);
			for (const auto& callback : m_State->m_OnReadyCallbacks)
			{
				callback(m_State->m_Value);
			}
		}
	}

	template<typename T>
	void Future<T>::SetAndSignal(const T& value)
	{
		Set(value);
		Signal();
	}

	template<typename T>
	void Future<T>::Wait()
	{
		m_State->m_FinishedEvent.Wait();
	}

	template<typename T>
	void Future<T>::Wait(std::chrono::milliseconds milliseconds)
	{
		m_State->m_FinishedEvent.Wait(milliseconds);
	}

	template<typename T>
	const T& Future<T>::WaitAndGet()
	{
		Wait();
		return m_State->m_Value;
	}

	template<typename T>
	const T& Future<T>::Get()
	{
		SK_CORE_VERIFY(m_State->m_Finished);
		return m_State->m_Value;
	}

	template<typename T>
	void Future<T>::OnReady(auto func)
	{
		if (m_State->m_Finished)
			func(m_State->m_Value);

		std::scoped_lock lock(m_State->m_Mutex);
		m_State->m_OnReadyCallbacks.push_back(func);
	}

}
