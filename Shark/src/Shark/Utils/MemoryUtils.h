#pragma once

#undef ZeroMemory

namespace Shark {

	class MemoryUtils
	{
	public:
		static void ZeroMemory(void* data, uint32_t size);

		template<typename T>
		static void ZeroMemory(T& data)
		{
			ZeroMemory(&data, sizeof(T));
		}

	};

}
