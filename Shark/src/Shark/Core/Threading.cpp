#include "skpch.h"
#include "Threading.h"

#include "Shark/Utils/PlatformUtils.h"
#include "Shark/Utils/String.h"

namespace Shark {

	///////////////////////////////////////////////////////////////////////////////////////////////
	//// Thread Signal ////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////

	Threading::ThreadSignal::ThreadSignal(bool manualReset, bool initialState)
		: m_ManualReset(manualReset), m_Signaled(initialState)
	{
	}

	Threading::ThreadSignal::~ThreadSignal()
	{
	}

	void Threading::ThreadSignal::Notify()
	{
		std::unique_lock lock(m_Mutex);
		m_Signaled = true;
		m_ConditionVariable.notify_all();
	}

	void Threading::ThreadSignal::Reset()
	{
		std::unique_lock lock(m_Mutex);
		m_Signaled = false;
	}

	void Threading::ThreadSignal::Wait()
	{
		std::unique_lock lock(m_Mutex);
		while (!m_Signaled)
		{
			m_ConditionVariable.wait(lock);
		}

		if (!m_ManualReset)
			m_Signaled = false;

	}

	void Threading::ThreadSignal::Wait(std::chrono::milliseconds time)
	{
		std::unique_lock lock(m_Mutex);

		if (!m_Signaled)
		{
			m_ConditionVariable.wait_for(lock, time);
		}

		if (!m_ManualReset)
			m_Signaled = false;

	}

}
