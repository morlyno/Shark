#include "skpch.h"
#include "CommandQueue.h"

#include "Shark/Debug/Profiler.h"

namespace Shark {

	CommandQueue::CommandQueue(uint32_t bufferSize)
	{
		m_Buffer.Allocate(bufferSize);
		m_BufferPtr = m_Buffer.Data;
	}

	CommandQueue::~CommandQueue()
	{
		m_Buffer.Release();
	}

	void CommandQueue::Execute()
	{
		//SK_LOG_IF(m_CommandCount > 0, Log::Logger::Core, Log::Level::Trace, Tag::Renderer, "CommandQueue::Excecute | {0} Commands | {1} bytes", m_CommandCount, (uint64_t)(m_BufferPtr - m_Buffer.Data));

		byte* buffer = m_Buffer.Data;
		while (buffer < m_BufferPtr)
		{
			SK_PROFILE_SCOPED("Execute Command");
			CommandFn cmdFunc = *(CommandFn*)buffer;
			buffer += sizeof(CommandFn);

			uint32_t userFuncSize = *(uint32_t*)buffer;
			buffer += sizeof(uint32_t);

			cmdFunc(buffer);
			buffer += userFuncSize;
		}

		m_BufferPtr = m_Buffer.Data;
		m_CommandCount = 0;
	}

	void* CommandQueue::Allocate(CommandFn func, uint32_t userFuncSize)
	{
		SK_PROFILE_FUNCTION();
		// CommandFunc | UserFuncSize | UserFunc

		if ((m_BufferPtr + sizeof(CommandFn) + sizeof(uint32_t) + userFuncSize) >= (m_Buffer.Data + m_Buffer.Size))
		{
			uint64_t bufferPtrOffset = m_BufferPtr - m_Buffer.Data;
			m_Buffer.Resize(m_Buffer.Size + m_Buffer.Size / 2);
			m_BufferPtr = m_Buffer.Data + bufferPtrOffset;
		}

		*(CommandFn*)m_BufferPtr = func;
		m_BufferPtr += sizeof(CommandFn);

		*(uint32_t*)m_BufferPtr = userFuncSize;
		m_BufferPtr += sizeof(uint32_t);

		void* memory = m_BufferPtr;
		m_BufferPtr += userFuncSize;

		m_CommandCount++;
		return memory;
	}

}
