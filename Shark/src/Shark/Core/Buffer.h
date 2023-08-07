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
		void Resize(uint64_t newSize, bool canShrink = true);
		void Grow(uint64_t amount) { Resize(Size + amount); }
		template<typename TType>
		void Grow(uint64_t count) { Resize(Size + count * sizeof(TType)); }
		void Release();
		void Write(const void* data, uint64_t size, uint64_t offset = 0);
		void Write(const byte* data, uint64_t size, uint64_t offset = 0);
		void Write(const Buffer buffer, uint64_t offset = 0) { Write(buffer.Data, buffer.Size, offset); }
		void SetZero();
		Buffer SubBuffer(uint32_t offset, uint32_t size);

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
		
		template<typename T>
		T& Value() const
		{
			return *(T*)Data;
		}

		template<typename T>
		T* Offset(uint64_t offset)
		{
			SK_CORE_ASSERT((offset * sizeof T) < Size);
			return (T*)Data + offset;
		}

		template<typename T>
		uint64_t Count() const
		{
			return (Size + sizeof(T) - 1) / sizeof(T);
		}

		operator bool() const
		{
			return Data;
		}

		template<typename T>
		void CopyTo(std::vector<T>& dest)
		{
			dest.resize(Count<T>());
			memcpy(dest.data(), Data, Size);
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

		template<typename TType>
		static Buffer FromArray(const std::vector<TType>& array)
		{
			return Buffer((byte*)array.data(), array.size() * sizeof(TType));
		}

		static Buffer Copy(const byte* data, uint64_t Size);
		static Buffer Copy(Buffer buffer);

		static Buffer New(uint64_t size, bool setZero = false);

	public:
		byte* Data = nullptr;
		uint64_t Size = 0;

		byte* End() { return Data + Size; }
		const byte* End() const { return Data + Size; }
	};

	class ScopedBuffer
	{
	public:
		ScopedBuffer() = default;
		ScopedBuffer(std::nullptr_t) {}
		ScopedBuffer(const ScopedBuffer&) = delete;
		ScopedBuffer& operator=(const ScopedBuffer&) = delete;

		ScopedBuffer(ScopedBuffer&&) = default;
		ScopedBuffer& operator=(ScopedBuffer&&) = default;

		ScopedBuffer(byte* data, uint64_t size) : m_Buffer(data, size) {}
		template<typename T>
		ScopedBuffer(T* data, uint64_t size) : m_Buffer(data, size) {}

		~ScopedBuffer() { m_Buffer.Release(); }

		void Allocate(uint64_t size) { m_Buffer.Allocate(size); }
		void Resize(uint64_t newSize, bool canShrink = true) { m_Buffer.Resize(newSize, canShrink); }
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

		template<typename T>
		T* Offset(uint64_t offset)
		{
			return m_Buffer.Offset<T>(offset);
		}

		operator bool() const { return m_Buffer; }

		byte* Data() const { return m_Buffer.Data; }
		uint64_t Size() const { return m_Buffer.Size; }

		byte* End() { return m_Buffer.End(); }

		Buffer GetBuffer() const { return m_Buffer; }

	private:
		Buffer m_Buffer;
	};

}
