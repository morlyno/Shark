#include "skpch.h"
#include "RenderCommandQueue.h"

#include "Shark/Debug/Profiler.h"

namespace Shark {

	RenderCommandQueue::RenderCommandQueue(uint64_t bufferSize)
	{
		m_BufferSize = bufferSize;
		m_Buffer = sknew byte[m_BufferSize];
		m_BufferPtr = m_Buffer;
	}

	RenderCommandQueue::~RenderCommandQueue()
	{
		SK_CORE_VERIFY(m_CommandCount == 0);
		skdelete m_Buffer;
		m_Buffer = nullptr;
		m_BufferPtr = nullptr;
		//m_BufferSize = 0;
	}

	void RenderCommandQueue::Execute()
	{
		//SK_LOG_IF(m_CommandCount > 0, Log::Logger::Core, LogLevel::Trace, Tag::Renderer, "CommandQueue::Excecute | {0} Commands | {1} bytes", m_CommandCount, (uint64_t)(m_BufferPtr - m_Buffer.Data));
		SK_CORE_TRACE_TAG("Renderer", "Executing {} Commands ({} bytes)", m_CommandCount, m_BufferPtr - m_Buffer);

		m_Executing = true;
		byte* buffer = m_Buffer;
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

		m_BufferPtr = m_Buffer;
		m_CommandCount = 0;
		m_Executing = false;
	}

	void* RenderCommandQueue::Allocate(CommandFn func, uint32_t userFuncSize)
	{
		SK_PROFILE_FUNCTION();
		// CommandFunc | UserFuncSize | UserFunc

		if ((m_BufferPtr + sizeof(CommandFn) + sizeof(uint32_t) + userFuncSize) >= (m_Buffer + m_BufferSize))
		{
			uint64_t bufferPtrOffset = m_BufferPtr - m_Buffer;

			uint64_t newSize = m_BufferSize + m_BufferSize / 2;
			SK_CORE_WARN_TAG("Renderer", "Resizing RenderCommandQueue {} -> {}", m_BufferSize, newSize);
			byte* newBuffer = sknew byte[newSize];
			memcpy(newBuffer, m_Buffer, m_BufferSize);

			skdelete m_Buffer;
			m_Buffer = newBuffer;
			m_BufferSize = newSize;

			m_BufferPtr = m_Buffer + bufferPtrOffset;
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
