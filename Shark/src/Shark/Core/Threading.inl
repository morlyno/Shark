#pragma once

namespace Shark {

	template<typename T>
	Threading::Future<T>::FutureState::FutureState(bool signaled)
		: m_FinishedEvent(true, signaled)
	{
	}

	template<typename T>
	Threading::Future<T>::Future(bool createState)
	{
		if (createState)
		{
			m_State = std::make_shared<FutureState>(false);
		}
	}

	template<typename T>
	Threading::Future<T>::Future(T result)
	{
		m_State = std::make_shared<FutureState>(true);
		m_State->m_Value = result;
		m_State->m_Finished = true;
	}

	template<typename T>
	void Threading::Future<T>::MergeCallbacks(const Future& other)
	{
		if (m_State == other.m_State)
			return;

		std::scoped_lock lock(m_State->m_Mutex, other.m_State->m_Mutex);
		m_State->m_OnReadyCallbacks.insert(m_State->m_OnReadyCallbacks.end(), other.m_State->m_OnReadyCallbacks.begin(), other.m_State->m_OnReadyCallbacks.end());
	}

	template<typename T>
	void Threading::Future<T>::Set(const T& value)
	{
		SK_CORE_VERIFY(m_State->m_Finished == false);
		m_State->m_Value = value;
		m_State->m_Finished = true;
	}

	template<typename T>
	void Threading::Future<T>::Signal(bool wake, bool callback)
	{
		if (wake)
		{
			m_State->m_FinishedEvent.Set();
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
	void Threading::Future<T>::SetAndSignal(const T& value)
	{
		Set(value);
		Signal();
	}

	template<typename T>
	void Threading::Future<T>::Wait()
	{
		m_State->m_FinishedEvent.Wait();
	}

	template<typename T>
	void Threading::Future<T>::Wait(std::chrono::milliseconds milliseconds)
	{
		m_State->m_FinishedEvent.Wait(milliseconds);
	}

	template<typename T>
	const T& Threading::Future<T>::WaitAndGet()
	{
		Wait();
		return m_State->m_Value;
	}

	template<typename T>
	const T& Threading::Future<T>::Get()
	{
		SK_CORE_VERIFY(m_State->m_Finished);
		return m_State->m_Value;
	}

	template<typename T>
	void Threading::Future<T>::OnReady(auto func)
	{
		if (m_State->m_Finished)
			func(m_State->m_Value);

		std::scoped_lock lock(m_State->m_Mutex);
		m_State->m_OnReadyCallbacks.push_back(func);
	}

}
