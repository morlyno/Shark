#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Buffer.h"

namespace Shark {

	class Memory
	{
	public:
		static void Write(void* destination, void* source, uint64_t byteSize);
		static void Write(void* destination, const Buffer source);
		static void Write(Buffer& destination, void* source, uint64_t byteSize);
		static void WriteZero(void* destination, uint64_t byteSize);

		// This will resize the vector if the size is not enough
		template<typename T>
		static void Write(std::vector<T>& destination, void* source, uint64_t byteSize);
		template<typename T>
		static void Write(std::vector<T>& destination, const Buffer source);

		template<typename T>
		static void WriteZero(T& destination);

		static void Read(void* memory, uint64_t byteOffset, void* destination, uint64_t byteCount);

		template<typename T>
		static void Read(void* memory, uint64_t byteOffset, T& destination);

		template<typename T>
		static T Read(void* memory, uint64_t byteOffset);
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

template<typename T>
void Shark::Memory::Write(std::vector<T>& destination, const Buffer source)
{
	const uint64_t count = (source.Size + sizeof(T) - 1) / sizeof(T);
	if (destination.size() < count)
		destination.resize(count);

	Write(destination.data(), source);
}

template<typename T>
void Shark::Memory::WriteZero(T& destination)
{
	WriteZero(&destination, sizeof(T));
}

template<typename T>
void Shark::Memory::Read(void* memory, uint64_t byteOffset, T& destination)
{
	Read(memory, byteOffset, &destination, sizeof(T));
}

template<typename T>
T Shark::Memory::Read(void* memory, uint64_t byteOffset)
{
	T value;
	Read(memory, byteOffset, &value, sizeof(T));
	return value;
}
