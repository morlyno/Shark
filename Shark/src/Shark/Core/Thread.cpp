#include "skpch.h"
#include "Thread.h"

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

	void Thread::Join()
	{
		if (m_Thread.joinable())
			m_Thread.join();
	}

	void Thread::SetName(const std::string& name)
	{
		Platform::SetThreadName(m_Thread, name);
	}

}
