#pragma once

#include "Shark/Core/Base.h"

namespace Shark {

	class RenderCommandQueue
	{
	public:
		using CommandFn = void(*)(void*);

	public:
		RenderCommandQueue();
		~RenderCommandQueue();

		void Execute();
		void* Allocate(CommandFn func, uint32_t userFuncSize);

	public:
		uint32_t GetCommandCount() const { return m_CommandCount; }
		bool IsExecuting() const { return m_Executing; };

	private:
		byte* m_Buffer;
		uint64_t m_BufferSize = 0;
		byte* m_BufferPtr = nullptr;
		uint32_t m_CommandCount = 0;

		bool m_Executing = false;
	};

}
