#include "skpch.h"
#include "CommandQueue.h"

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
		//if (m_CommandCount > 0)
		//{
		//	SK_CORE_INFO("CommandQueue::Excecute | {0} Commands | {1} bytes", m_CommandCount, m_BufferPtr - m_Buffer);
		//}

		byte* buffer = m_Buffer.Data;

		while (buffer < m_BufferPtr)
		{
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
		// CommandFunc | UserFuncSize | UserFunc

		if ((m_BufferPtr + sizeof(CommandFn) + sizeof(uint32_t) + userFuncSize) >= (m_Buffer.Data + m_Buffer.Size))
		{
			// TODO(moro): Grow Buffer
			SK_CORE_ASSERT(false, "Command Buffer to small");
			return nullptr;
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
