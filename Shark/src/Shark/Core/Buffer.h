#pragma once

#include "Shark/Core/Base.h"

#include <ranges>

namespace Shark {

	class Buffer;
	class MutableBuffer;
	class UniqueBuffer;
	class BufferHandle;

	//////////////////////////////////////////////////////////////////////////
	//// Const Buffer
	//////////////////////////////////////////////////////////////////////////

	class Buffer
	{
	public:
		Buffer() = default;
		Buffer(std::nullptr_t) {}
		Buffer(const void* data, uint64_t size) : Data(data), Size(size) {}
		Buffer(const MutableBuffer buffer);
		~Buffer() = default;

		template<typename TRange>
			requires std::ranges::contiguous_range<TRange>
		Buffer(const TRange& data);

		template<typename TValue>
			requires (std::is_trivially_copyable_v<TValue> && !std::ranges::contiguous_range<TValue>)
		Buffer(const TValue& data);

		Buffer(Buffer&& other) noexcept;
		Buffer& operator=(Buffer&& other) noexcept;
		Buffer(const Buffer& other) = default;
		Buffer& operator=(const Buffer& other) = default;

		void Read(void* targetData, uint64_t size, uint64_t offset = 0) const;
		const void* At(uint64_t byteOffset) const;

		template<typename T>
		const T* At(uint64_t offset) const;

		template<typename T>
		const T* As() const;

		Buffer AsBuffer() const;
		
		bool operator!() const { return Data == nullptr; }
		operator bool() const  { return Data != nullptr; }

	public:
		const void* Data = nullptr;
		uint64_t Size = 0;
	};

	//////////////////////////////////////////////////////////////////////////
	//// Buffer
	//////////////////////////////////////////////////////////////////////////

	class MutableBuffer
	{
	public:
		MutableBuffer() = default;
		MutableBuffer(std::nullptr_t) {}
		MutableBuffer(void* data, uint64_t size) : Data(data), Size(size) {}
		~MutableBuffer() = default;

		MutableBuffer(MutableBuffer&& other) noexcept;
		MutableBuffer& operator=(MutableBuffer&& other) noexcept;
		MutableBuffer(const MutableBuffer& other) = default;
		MutableBuffer& operator=(const MutableBuffer& other) = default;

	public:
		void Allocate(uint64_t size);
		void Resize(uint64_t newSize, bool canShrink = true);
		void Release();

		void WriteZero() const;
		void Write(const void* data, uint64_t size, uint64_t offset = 0) const;
		void Read(void* targetData, uint64_t size, uint64_t offset = 0) const;

		template<typename T>
		T& ReadAs(uint64_t byteOffset = 0) const;

		MutableBuffer Range(uint64_t size, uint64_t offset = 0) const;
		void* At(uint64_t byteOffset) const;

		template<typename T>
		T* At(uint64_t offset) const;

		template<typename T>
		T* As() const;

		Buffer AsBuffer() const;

	public:
		static MutableBuffer Copy(const Buffer data);
		static MutableBuffer Copy(const void* data, uint64_t size);

	public:
		void* Data = nullptr;
		uint64_t Size = 0;
	};

	//////////////////////////////////////////////////////////////////////////
	//// Unique Buffer
	//////////////////////////////////////////////////////////////////////////

	class UniqueBuffer : private MutableBuffer
	{
	public:
		UniqueBuffer() = default;
		UniqueBuffer(std::nullptr_t) {}
		UniqueBuffer(MutableBuffer&& buffer);
		~UniqueBuffer();

		UniqueBuffer(UniqueBuffer&& other) noexcept;
		UniqueBuffer& operator=(UniqueBuffer&& other) noexcept;
		UniqueBuffer(const UniqueBuffer&)            = delete;
		UniqueBuffer& operator=(const UniqueBuffer&) = delete;
		
		operator Buffer() &       { return AsBuffer(); }
		operator Buffer() const&  { return AsBuffer(); }
		operator Buffer() &&      = delete;
		operator Buffer() const&& = delete;

		using MutableBuffer::Allocate;
		using MutableBuffer::Resize;
		using MutableBuffer::Release;

		using MutableBuffer::WriteZero;
		using MutableBuffer::Write;
		using MutableBuffer::Read;
		using MutableBuffer::ReadAs;

		using MutableBuffer::Range;
		using MutableBuffer::At;
		using MutableBuffer::As;

		MutableBuffer ExtractBuffer();
		Buffer AsBuffer() const;

		bool operator!() const { return Data == nullptr; }
		operator bool() const  { return Data != nullptr; }

		static UniqueBuffer Copy(const Buffer buffer);
		static UniqueBuffer Copy(const void* data, uint64_t size);

	public:
		using MutableBuffer::Data;
		using MutableBuffer::Size;
	};

	//////////////////////////////////////////////////////////////////////////
	//// Buffer Handle
	//////////////////////////////////////////////////////////////////////////

	class BufferHandle
	{
	public:
		BufferHandle() = default;
		BufferHandle(Buffer buffer);
		BufferHandle(MutableBuffer buffer);
		BufferHandle(UniqueBuffer&& buffer);
		~BufferHandle();

		BufferHandle(BufferHandle&& other);
		BufferHandle& operator=(BufferHandle&& other);

		template<typename T>
			requires requires(const T& t) { { Buffer(t) } -> std::same_as<Buffer>; }
		BufferHandle(const T& t)
			: BufferHandle(Buffer(t)) {}

		operator Buffer() &       { return AsBuffer(); }
		operator Buffer() const&  { return AsBuffer(); }
		operator Buffer() &&      = delete;
		operator Buffer() const&& = delete;

		static BufferHandle Alive(const Buffer buffer);
		static BufferHandle Alive(const MutableBuffer buffer);

		Buffer        AsBuffer() const;
		MutableBuffer AsMutableBuffer() const;

		bool IsEmpty() const;
		bool IsMutable() const;

		bool operator!() const { return m_ConstData == nullptr; }
		operator bool() const  { return m_ConstData != nullptr; }

		[[nodiscard]] BufferHandle Store();
		[[nodiscard]] static BufferHandle Store(BufferHandle& handle);

	private:
		void*       m_MutableData = nullptr;
		const void* m_ConstData = nullptr;
		uint64_t    m_Size = 0;

		bool m_Stored = false;
		bool m_Free = false;

	};

	//////////////////////////////////////////////////////////////////////////
	//// As Buffer
	//////////////////////////////////////////////////////////////////////////

	struct AsBuffer_t
	{
		template<typename T>
			requires requires(const T& t) { { t.AsBuffer() } -> std::same_as<Buffer>; }
		Buffer operator()(const T& t) const { return t.AsBuffer(); }

		template<typename T>
			requires std::constructible_from<Buffer, const T&>
		Buffer operator()(const T& t) const { return Buffer(t); }
	};

	static constexpr AsBuffer_t AsBuffer = {};

	template<typename T>
		requires std::invocable<AsBuffer_t, const T&>
	Buffer operator|(const T& t, AsBuffer_t)
	{
		return AsBuffer(t);
	}

}

namespace Shark {

	template<typename TRange>
		requires std::ranges::contiguous_range<TRange>
	Buffer::Buffer(const TRange& data)
		:
		Data(std::ranges::data(data)),
		Size(std::ranges::size(data) * sizeof(std::ranges::range_value_t<TRange>))
	{
	}

	template<typename TValue>
		requires (std::is_trivially_copyable_v<TValue> && !std::ranges::contiguous_range<TValue>)
	Buffer::Buffer(const TValue& data)
		:
		Data(std::addressof(data)),
		Size(sizeof data)
	{
	}

	template<typename T>
	const T* Buffer::At(uint64_t offset) const
	{
		return static_cast<T*>(At(offset * sizeof(T)));
	}

	template<typename T>
	const T* Buffer::As() const
	{
		return static_cast<const T*>(Data);
	}

	template<typename T>
	T& MutableBuffer::ReadAs(uint64_t byteOffset) const
	{
		return *static_cast<T*>(At(byteOffset));
	}

	template<typename T>
	T* MutableBuffer::At(uint64_t offset) const
	{
		return static_cast<T*>(At(offset * sizeof(T)));
	}

	template<typename T>
	T* MutableBuffer::As() const
	{
		return static_cast<T*>(Data);
	}

}
