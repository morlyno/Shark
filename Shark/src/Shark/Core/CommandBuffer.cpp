#include "skpch.h"
#include "CommandBuffer.h"

namespace Shark {

	CommandBuffer::CommandBuffer(uint32_t bufferSize)
	{
		m_Buffer.Allocate(bufferSize);
		m_BufferPtr = m_Buffer;
	}

	CommandBuffer::~CommandBuffer()
	{
		m_Buffer.Release();
	}

	void CommandBuffer::Execute()
	{
		if (m_CommandCount > 0)
		{
			SK_CORE_INFO("CommandBuffer::Excecute | {0} Commands | {1} bytes", m_CommandCount, m_BufferPtr - m_Buffer);
		}

		byte* buffer = m_Buffer;

		while (buffer < m_BufferPtr)
		{
			CommandFunc cmdFunc = *(CommandFunc*)buffer;
			buffer += sizeof(CommandFunc);

			uint32_t userFuncSize = *(uint32_t*)buffer;
			buffer += sizeof(uint32_t);

			cmdFunc(buffer);
			buffer += userFuncSize;
		}

		m_BufferPtr = m_Buffer;
		m_CommandCount = 0;
	}

	void* CommandBuffer::Allocate(CommandFunc func, uint32_t userFuncSize)
	{
		// CommandFunc | UserFuncSize | UserFunc

		if ((m_BufferPtr + sizeof(CommandFunc) + sizeof(uint32_t) + userFuncSize) >= (m_Buffer + m_Buffer.Size))
		{
			// TODO(moro): Grow Buffer
			SK_CORE_ASSERT(false, "Command Buffer to small");
			return nullptr;
		}

		*(CommandFunc*)m_BufferPtr = func;
		m_BufferPtr += sizeof(CommandFunc);

		*(uint32_t*)m_BufferPtr = userFuncSize;
		m_BufferPtr += sizeof(uint32_t);

		void* memory = m_BufferPtr;
		m_BufferPtr += userFuncSize;

		m_CommandCount++;
		return memory;
	}

}
