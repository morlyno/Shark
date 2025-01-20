#include "skpch.h"
#include "Threading.h"

#include "Shark/Utils/PlatformUtils.h"
#include "Shark/Utils/String.h"

namespace Shark {

	///////////////////////////////////////////////////////////////////////////////////////////////
	//// Thread ///////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////

	Threading::Thread::Thread(std::string_view name)
		: m_Name(name)
	{
	}

	Threading::Thread::~Thread()
	{
		StopAndJoin();
	}

	void Threading::Thread::SetName(std::string_view name)
	{
		if (name.empty())
			return;

		m_Name = name;

		if (Running())
		{
			Platform::SetThreadName(m_Thread, name);
		}
	}

	bool Threading::Thread::Running() const
	{
		return m_Thread.joinable();
	}

	void Threading::Thread::RequestStop()
	{
		m_Thread.request_stop();
	}

	void Threading::Thread::Join()
	{
		if (m_Thread.joinable())
			m_Thread.join();
	}

	void Threading::Thread::StopAndJoin()
	{
		if (m_Thread.joinable())
		{
			m_Thread.request_stop();
			m_Thread.join();
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	//// Thread Signal ////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////

	Threading::TrackedMutex::TrackedMutex(const tracy::SourceLocationData* srcloc)
		: m_Context(srcloc)
	{
	}

	Threading::TrackedMutex::~TrackedMutex()
	{

	}

	void Threading::TrackedMutex::Lock()
	{
		const auto runAfter = m_Context.BeforeLock();
		m_Mutex.Lock();
		if (runAfter)
			m_Context.AfterLock();
	}

	bool Threading::TrackedMutex::TryLock()
	{
		const auto acquired = m_Mutex.TryLock();
		m_Context.AfterTryLock(acquired);
		return acquired;
	}

	void Threading::TrackedMutex::Unlock()
	{
		m_Mutex.Unlock();
		m_Context.AfterUnlock();
	}

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
		m_ConditionVariable.NotifyAll();
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
			m_ConditionVariable.Wait(lock);
		}

		if (!m_ManualReset)
			m_Signaled = false;

	}

	void Threading::ThreadSignal::Wait(std::chrono::milliseconds time)
	{
		std::unique_lock lock(m_Mutex);

		if (!m_Signaled)
		{
			m_ConditionVariable.Wait(lock, time);
		}

		if (!m_ManualReset)
			m_Signaled = false;

	}

}
