#pragma once

#include "Shark/Core/Base.h"
#include <span>

namespace Shark {

	//////////////////////////////////////////////////////////////////////////////////////////////////
	//// Buffer //////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////

	class Buffer
	{
	public:
		Buffer() = default;
		Buffer(std::nullptr_t) {}
		Buffer(void* data, uint64_t size) : Data(data), Size(size) {}
		~Buffer() = default;

		void Allocate(uint64_t size);
		void Resize(uint64_t newSize, bool canShrink = true);
		void Grow(uint64_t amount) { Resize(Size + amount); }
		void Release();

		void Write(const void* data, uint64_t size, uint64_t offset = 0) const;
		void Write(const Buffer buffer, uint64_t offset = 0) const { Write(buffer.Data, buffer.Size, offset); }
		void Write(const Buffer buffer, uint64_t size, uint64_t offset) const { Write(buffer.Data, std::min(size, buffer.Size), offset); }

		void Read(void* resultBuffer, uint64_t size, uint64_t offset = 0) const;

		void SetZero() const;
		Buffer SubBuffer(uint32_t offset, uint32_t size) const;

		template<typename T>
			requires (!std::is_pointer_v<T>)
		void Write(const T& data, uint64_t offset = 0) const
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
		T* Offset(uint64_t offset) const
		{
			SK_CORE_ASSERT((offset * sizeof(T)) < Size);
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
			return Buffer((void*)&value, sizeof(TValue));
		}

		template<typename TRange>
		static Buffer FromArray(const TRange& range)
			requires (std::ranges::contiguous_range<TRange>)
		{
			std::span data = range;
			return Buffer((void*)data.data(), data.size_bytes());
		}

		static Buffer Copy(const void* data, uint64_t Size);
		static Buffer Copy(Buffer buffer);

		static Buffer New(uint64_t size, bool setZero = false);

	public:
		void* Data = nullptr;
		uint64_t Size = 0;

		byte* End() { return (byte*)Data + Size; }
		const byte* End() const { return (byte*)Data + Size; }
	};

	//////////////////////////////////////////////////////////////////////////////////////////////////
	//// Stack Buffer ////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////

	template<uint64_t TSize>
	struct StackBuffer
	{
		StackBuffer()
			: Interface(Data, TSize) {}

		static constexpr uint64_t Size = TSize;
		byte Data[TSize];

		const Buffer Interface;

		operator Buffer() const { return Interface; }
		void Write(Buffer buffer, uint64_t offset = 0) const { Interface.Write(buffer, offset); }
		void Write(const void* data, uint64_t size, uint64_t offset = 0) const { Interface.Write(data, size, offset); }

	};

	//////////////////////////////////////////////////////////////////////////////////////////////////
	//// Scoped Buffer ///////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////

	class ScopedBuffer
	{
	public:
		ScopedBuffer() = default;
		ScopedBuffer(std::nullptr_t) {}
		ScopedBuffer(const ScopedBuffer&) = delete;
		ScopedBuffer& operator=(const ScopedBuffer&) = delete;

		ScopedBuffer(ScopedBuffer&&) = default;
		ScopedBuffer& operator=(ScopedBuffer&&) = default;

		ScopedBuffer(Buffer buffer)
			: m_Buffer(buffer) {}

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
		operator Buffer() const { return m_Buffer; }

		void* Data() const { return m_Buffer.Data; }
		uint64_t Size() const { return m_Buffer.Size; }

		byte* End() { return m_Buffer.End(); }

		Buffer GetBuffer() const { return m_Buffer; }

	private:
		Buffer m_Buffer;
	};

}
