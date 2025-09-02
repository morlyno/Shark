#pragma once

#include "Shark/Core/Base.h"

namespace Shark {

	class Memory
	{
	public:
		static void Write(void* destination, void* source, uint64_t byteSize);
		static void Write(void* destination, const Buffer source);

		// This will resize the vector if the size is not enough
		template<typename T>
		static void Write(std::vector<T>& destination, void* source, uint64_t byteSize);
	};

}

template<typename T>
void Shark::Memory::Write(std::vector<T>& destination, void* source, uint64_t byteSize)
{
	const uint64_t count = (byteSize + sizeof(T) - 1) / sizeof(T);
	if (destination.size() < count)
		destination.resize(count);

	Write(destination.data(), source, byteSize);
}
