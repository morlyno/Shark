#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Buffer.h"

namespace Shark {

	class CommandQueue
	{
	public:
		using CommandFn = void(*)(void*);

	public:
		CommandQueue(uint32_t bufferSize = 1024);
		~CommandQueue();

		void Execute();
		void* Allocate(CommandFn func, uint32_t userFuncSize);

		template<typename TFunc>
		void Submit(const TFunc& func)
		{
			auto command = [](void* funcPtr)
			{
				auto cmdPtr = (TFunc*)funcPtr;
				(*cmdPtr)();
				cmdPtr->~TFunc();
			};

			void* storage = Allocate(command, sizeof(func));
			new(storage) TFunc(func);
		}

		uint32_t GetCommandCount() const { return m_CommandCount; }
		bool IsLocked() const { return m_Locked; };
	private:
		//Buffer m_Buffer;
		byte* m_Buffer;
		uint64_t m_BufferSize = 0;
		byte* m_BufferPtr = nullptr;
		uint32_t m_CommandCount = 0;

		bool m_Locked = false;
	};

}
