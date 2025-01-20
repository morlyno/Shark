#pragma once

namespace Shark::Threading {

	///////////////////////////////////////////////////////////////////////////////////////////////
	//// Thread ///////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////

	template<typename TFunc, typename... TArgs>
		requires(!std::is_convertible_v<TFunc, std::string_view>)
	Thread::Thread(TFunc&& func, TArgs&&... args)
	{
		m_Thread = std::jthread(func, std::forward<TArgs>(args)...);
	}

	template<typename TFunc, typename... TArgs>
	Thread::Thread(std::string_view name, TFunc&& func, TArgs&&... args)
		: m_Name(name)
	{
		m_Thread = std::jthread(func, std::forward<TArgs>(args)...);
		Platform::SetThreadName(m_Thread, m_Name);
	}

	template<typename TFunc, typename... TArgs>
	void Thread::Dispacht(TFunc&& func, TArgs&&... args)
	{
		m_Thread = std::jthread(func, std::forward<TArgs>(args)...);

		if (!m_Name.empty())
			Platform::SetThreadName(m_Thread, m_Name);
	}

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

	///////////////////////////////////////////////////////////////////////////////////////////////
	//// Condition Variable ///////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////

	template<typename TPredicate, typename... TArgs>
	void ConditionVariable::Wait(std::unique_lock<Mutex>& lock, TPredicate predicate, TArgs&&... valueOrInstance)
	{
		while (!std::invoke(predicate, std::move<TArgs>(valueOrInstance)...))
			Wait(lock);
	}

	template<typename TPredicate, typename... TArgs>
	void ConditionVariable::Wait(std::unique_lock<Mutex>& lock, std::chrono::milliseconds time, TPredicate predicate, TArgs&&... valueOrInstance)
	{
		while (!std::invoke(predicate, std::move<TArgs>(valueOrInstance)...))
			Wait(lock, time);
	}

}
