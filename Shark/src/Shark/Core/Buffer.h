#pragma once

#include "Shark/Core/Base.h"

namespace Shark {

	class Buffer
	{
	public:
		Buffer() = default;
		Buffer(std::nullptr_t) {}
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

		template<typename TValue>
		static Buffer FromValue(const TValue& value) { return Buffer((byte*)&value, sizeof(TValue)); }
		template<typename TType, uint64_t TSize>
		static Buffer FromArray(const TType(&array)[TSize]) { return Buffer((byte*)array, TSize * sizeof(TType)); }
		static Buffer Copy(const byte* data, uint64_t Size);

	public:
		byte* Data = nullptr;
		uint64_t Size = 0;
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
