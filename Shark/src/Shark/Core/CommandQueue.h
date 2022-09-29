#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Buffer.h"

namespace Shark {

	class CommandQueue
	{
	public:
		CommandQueue(uint32_t bufferSize = 1024);
		~CommandQueue();

		void Execute();

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

	private:
		using CommandFunc = void(*)(void*);
		void* Allocate(CommandFunc func, uint32_t userFuncSize);
	private:
		Buffer m_Buffer;
		byte* m_BufferPtr = nullptr;
		uint32_t m_CommandCount = 0;
	};

}
