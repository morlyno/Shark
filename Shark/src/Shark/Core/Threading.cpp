#include "skpch.h"
#include "Threading.h"

#include "Shark/Utils/PlatformUtils.h"
#include "Shark/Utils/String.h"

namespace Shark {

	Thread::Thread(const std::string& name)
		: m_Name(name)
	{
	}

	Thread::~Thread()
	{
		Join();
	}

	bool Thread::Running() const
	{
		return m_Thread.joinable();
	}

	void Thread::Join()
	{
		if (m_Thread.joinable())
			m_Thread.join();
	}

	void Thread::SetName(const std::string& name)
	{
		if (name.empty())
			return;

		m_Name = name;
		Platform::SetThreadName(m_Thread, name);
	}

	Threading::Signal::Signal(Uninitialized)
	{
	}

	Threading::Signal::Signal(bool manualReset, bool initialState)
	{
		m_Handle = CreateEventA(nullptr, manualReset, initialState, nullptr);
	}

	Threading::Signal::~Signal()
	{
		CloseHandle(m_Handle);
		m_Handle = NULL;
	}

	Threading::Signal::Signal(Signal&& other)
	{
		m_Handle = other.m_Handle;
		other.m_Handle = NULL;
	}

	Threading::Signal& Threading::Signal::operator=(Signal&& other)
	{
		m_Handle = other.m_Handle;
		other.m_Handle = NULL;
		return *this;
	}

	void Threading::Signal::Set()
	{
		BOOL success = SetEvent(m_Handle);

		if (!success)
		{
			std::error_code error(GetLastError(), std::system_category());
			SK_CORE_ERROR("Threading::Signal::Set Failed! ({}) {}", error, error.message());
		}
	}

	void Threading::Signal::Reset()
	{
		BOOL success = ResetEvent(m_Handle);

		if (!success)
		{
			std::error_code error(GetLastError(), std::system_category());
			SK_CORE_ERROR("Threading::Signal::Reset Failed! ({}) {}", error, error.message());
		}
	}

	void Threading::Signal::Wait()
	{
		WaitForSingleObject(m_Handle, INFINITE);
	}

	void Threading::Signal::Wait(std::chrono::milliseconds time)
	{
		WaitForSingleObject(m_Handle, time.count());
	}

	Threading::Signal::operator bool() const
	{
		return m_Handle != nullptr;
	}

}
