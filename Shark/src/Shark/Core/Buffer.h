#pragma once

#include "Shark/Core/Base.h"

namespace Shark {

	class Buffer
	{
	public:
		Buffer() = default;
		Buffer(std::nullptr_t) {}
		Buffer(byte* data, uint64_t size) : Data(data), Size(size) {}
		template<typename T>
		Buffer(T* data, uint64_t size) : Data((byte*)data), Size(size) {}
		~Buffer() = default;

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
		T* As() const
		{
			return (T*)Data;
		}
		
		operator bool() const
		{
			return Data;
		}

	public:
		template<typename TValue>
		static Buffer FromValue(const TValue& value)
		{
			return Buffer((byte*)&value, sizeof(TValue));
		}

		template<typename TType, uint64_t TSize>
		static Buffer FromArray(const TType(&array)[TSize])
		{
			return Buffer((byte*)array, TSize * sizeof(TType));
		}

		template<typename TType>
		static Buffer FromArray(TType* array, uint64_t size)
		{
			return Buffer((byte*)array, size * sizeof(TType));
		}

		static Buffer Copy(const byte* data, uint64_t Size);

	public:
		byte* Data = nullptr;
		uint64_t Size = 0;
	};

	class ScopedBuffer
	{
	public:
		ScopedBuffer() = default;
		ScopedBuffer(std::nullptr_t) {}
		ScopedBuffer(const ScopedBuffer&) = delete;
		const ScopedBuffer& operator=(const ScopedBuffer&) = delete;

		ScopedBuffer(byte* data, uint64_t size) : m_Buffer(data, size) {}
		template<typename T>
		ScopedBuffer(T* data, uint64_t size) : m_Buffer(data, size) {}

		~ScopedBuffer() { m_Buffer.Release(); }

		void Allocate(uint64_t size) { m_Buffer.Allocate(size); }
		void Release() { m_Buffer.Release(); }
		void Write(const void* data, uint64_t size, uint64_t offset = 0) { m_Buffer.Write(data, size, offset); }
		void Write(const byte* data, uint64_t size, uint64_t offset = 0) { m_Buffer.Write(data, size, offset); }

		template<typename T>
		void Write(const T& data, uint64_t offset = 0)
		{
			m_Buffer.Write(data, offset);
		}

		template<typename T>
		T* As() const
		{
			return m_Buffer.As<T>();
		}

		operator bool() const { return m_Buffer; }

		byte* Data() const { return m_Buffer.Data; }
		uint64_t Size() const { return m_Buffer.Size; }

	private:
		Buffer m_Buffer;
	};

}
