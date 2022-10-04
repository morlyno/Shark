#pragma once

#include "Shark/Core/Base.h"

namespace Shark {

	class Buffer
	{
	public:
		Buffer() = default;
		Buffer(byte* data, uint64_t size) : Data(data), Size(size) {}
		virtual ~Buffer() = default;

		void Allocate(uint64_t size);
		void Release();
		void Write(const void* data, uint64_t size, uint64_t offset = 0);
		void Write(const byte* data, uint64_t size, uint64_t offset = 0);

		template<typename T>
		void Write(const T& data, uint64_t offset = 0)
		{
			Write(&data, sizeof(T), offset);
		}

		template<typename T>
		T* As() { return (T*)Data; }
		
		template<typename T>
		const T* As() const { return (const T*)Data; }

		byte* Data = nullptr;
		uint64_t Size = 0;

		static Buffer Copy(const byte* data, uint64_t Size);
	};

	class ScopedBuffer : public Buffer
	{
	public:
		virtual ~ScopedBuffer()
		{
			Release();
		}

	};

}
