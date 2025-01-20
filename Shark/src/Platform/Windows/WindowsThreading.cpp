#include "skpch.h"
#include "Shark/Core/Threading.h"

namespace Shark::Threading {

	///////////////////////////////////////////////////////////////////////////////////////////////
	//// Mutex ////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////

	Mutex::Mutex()
	{
		InitializeCriticalSection(&m_NativeMutex);
	}

	Mutex::~Mutex()
	{
		DeleteCriticalSection(&m_NativeMutex);
	}

	void Mutex::Lock()
	{
		EnterCriticalSection(&m_NativeMutex);
	}

	bool Mutex::TryLock()
	{
		return (bool)TryEnterCriticalSection(&m_NativeMutex);
	}

	void Mutex::Unlock()
	{
		LeaveCriticalSection(&m_NativeMutex);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	//// Condition Variable ///////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////

	ConditionVariable::ConditionVariable()
	{
		InitializeConditionVariable(&m_ConditionVariable);
	}

	ConditionVariable::~ConditionVariable()
	{

	}

	void ConditionVariable::NotifyOne()
	{
		WakeConditionVariable(&m_ConditionVariable);
	}

	void ConditionVariable::NotifyAll()
	{
		WakeAllConditionVariable(&m_ConditionVariable);
	}

	void ConditionVariable::Wait(std::unique_lock<Mutex>& lock)
	{
		Mutex* mutex = lock.mutex();
		SleepConditionVariableCS(&m_ConditionVariable, mutex->GetNativeMutex(), INFINITE);
	}

	void ConditionVariable::Wait(std::unique_lock<Mutex>& lock, std::chrono::milliseconds time)
	{
		Mutex* mutex = lock.mutex();

		const auto ms = time.count();
		const DWORD milliseconds = (DWORD)std::min<int64_t>(ms, UINT32_MAX);
		SleepConditionVariableCS(&m_ConditionVariable, mutex->GetNativeMutex(), milliseconds);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	//// Process Signal ///////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////

	Threading::ProcessSignal::ProcessSignal(Uninitialized)
	{
	}

	Threading::ProcessSignal::ProcessSignal(bool manualReset, bool initialState)
	{
		m_Handle = CreateEventA(nullptr, manualReset, initialState, nullptr);
	}

	Threading::ProcessSignal::~ProcessSignal()
	{
		CloseHandle(m_Handle);
		m_Handle = NULL;
	}

	Threading::ProcessSignal::ProcessSignal(ProcessSignal&& other)
	{
		m_Handle = other.m_Handle;
		other.m_Handle = NULL;
	}

	Threading::ProcessSignal& Threading::ProcessSignal::operator=(ProcessSignal&& other)
	{
		m_Handle = other.m_Handle;
		other.m_Handle = NULL;
		return *this;
	}

	void Threading::ProcessSignal::Set()
	{
		BOOL success = SetEvent(m_Handle);

		if (!success)
		{
			std::error_code error(GetLastError(), std::system_category());
			SK_CORE_ERROR("Threading::ProcessSignal::Set Failed! ({}) {}", error, error.message());
		}
	}

	void Threading::ProcessSignal::Reset()
	{
		BOOL success = ResetEvent(m_Handle);

		if (!success)
		{
			std::error_code error(GetLastError(), std::system_category());
			SK_CORE_ERROR("Threading::ProcessSignal::Reset Failed! ({}) {}", error, error.message());
		}
	}

	void Threading::ProcessSignal::Wait()
	{
		WaitForSingleObject(m_Handle, INFINITE);
	}

	void Threading::ProcessSignal::Wait(std::chrono::milliseconds time)
	{
		WaitForSingleObject(m_Handle, time.count());
	}

	Threading::ProcessSignal::operator bool() const
	{
		return m_Handle != nullptr;
	}

}
