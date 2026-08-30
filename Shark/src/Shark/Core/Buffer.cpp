#include "skpch.h"
#include "Buffer.h"

#include "Shark/Core/Allocator.h"

#define ENABLE_VERIFY 1
#include "Shark/Core/Assert.h"

#define SK_BUFFER_ASSERT SK_CORE_ASSERT
#define SK_BUFFER_VERIFY SK_CORE_VERIFY

#define BUFFER_CHECK_RANGE SK_CORE_ASSERT

#if defined(ENABLE_SAFE_BUFFER_EXCEPTIONS) && !defined(ENABLE_SAFE_BUFFER_ACCESS)
	#define ENABLE_SAFE_BUFFER_ACCESS
#endif

#ifdef ENABLE_SAFE_BUFFER_ACCESS
	#ifdef ENABLE_SAFE_BUFFER_EXCEPTIONS
		#include <stdexcept>
		#define BUFFER_GUARD(_condition, _message, ...) if (!_condition) { throw std::out_of_range(_message); }
	#else
		#define BUFFER_GUARD(_condition, _message, ...) if (!_condition) { __VA_OPT__(__VA_ARGS__;)return; }
	#endif
#else
	#define BUFFER_GUARD(...) (void)0
#endif


namespace Shark {

	//////////////////////////////////////////////////////////////////////////
	//// Const Buffer
	//////////////////////////////////////////////////////////////////////////

	#pragma region ConstBuffer

	Buffer::Buffer(const MutableBuffer buffer)
		:
		Data(buffer.Data),
		Size(buffer.Size)
	{
	}

	Buffer::Buffer(Buffer&& other) noexcept
		:
		Data(std::exchange(other.Data, nullptr)),
		Size(std::exchange(other.Size, 0))
	{
	}

	Buffer& Buffer::operator=(Buffer&& other) noexcept
	{
		if (this == std::addressof(other))
			return *this;

		Data = std::exchange(other.Data, nullptr);
		Size = std::exchange(other.Size, 0);
		return *this;
	}

	void Buffer::Read(void* targetData, uint64_t size, uint64_t offset) const
	{
		BUFFER_CHECK_RANGE(size + offset <= Size, "[Buffer.Read] Out of range! Size:{} + Offset:{} is greater than buffer size {}", size, offset, Size);

		BUFFER_GUARD(size + offset <= Size, "[Buffer.Write] Out of range!");
		memcpy(targetData, static_cast<const uint8_t*>(Data) + offset, size);
	}

	const void* Buffer::At(uint64_t byteOffset) const
	{
		return static_cast<const uint8_t*>(Data) + byteOffset;
	}

	Buffer Buffer::AsBuffer() const
	{
		return { Data, Size };
	}

	#pragma endregion

	//////////////////////////////////////////////////////////////////////////
	//// Buffer
	//////////////////////////////////////////////////////////////////////////

	#pragma region Buffer

	MutableBuffer::MutableBuffer(MutableBuffer&& other) noexcept
		:
		Data(std::exchange(other.Data, nullptr)),
		Size(std::exchange(other.Size, 0))
	{
	}

	MutableBuffer& MutableBuffer::operator=(MutableBuffer&& other) noexcept
	{
		if (this == std::addressof(other))
			return *this;

		Data = std::exchange(other.Data, nullptr);
		Size = std::exchange(other.Size, 0);
		return *this;
	}

	void MutableBuffer::Allocate(uint64_t size)
	{
		Release();

		if (size)
		{
			Size = size;
			Data = Allocator::Allocate(size);
		}
	}

	void MutableBuffer::Resize(uint64_t newSize, bool canShrink)
	{
		if (newSize == Size || !canShrink && newSize < Size)
			return;

		Data = Allocator::Reallocate(Data, newSize);
		Size = newSize;
	}

	void MutableBuffer::Release()
	{
		if (!Data)
			return;

		Allocator::Free(Data);
		Data = nullptr;
		Size = 0;
	}

	void MutableBuffer::WriteZero() const
	{
		memset(Data, 0, Size);
	}

	void MutableBuffer::Write(const void* data, uint64_t size, uint64_t offset) const
	{
		BUFFER_CHECK_RANGE(size + offset <= Size, "[Buffer.Write] Out of range! Size:{} + Offset:{} is greater than Buffer Size {}", size, offset, Size);
	
		BUFFER_GUARD(size + offset <= Size, "[Buffer.Write] Out of range!");
		memcpy(static_cast<uint8_t*>(Data) + offset, data, size);
	}

	void MutableBuffer::Read(void* targetData, uint64_t size, uint64_t offset) const
	{
		BUFFER_CHECK_RANGE(size + offset <= Size, "[Buffer.Read] Out of range! Size:{} + Offset:{} is greater than buffer size {}", size, offset, Size);

		BUFFER_GUARD(size + offset <= Size, "[Buffer.Write] Out of range!");
		memcpy(targetData, static_cast<uint8_t*>(Data) + offset, size);
	}

	MutableBuffer MutableBuffer::Range(uint64_t size, uint64_t offset) const
	{
		BUFFER_CHECK_RANGE(size + offset <= Size, "[Buffer.Range] Out of range! Size:{} + Offset:{} is greater than buffer size {}", size, offset, Size);

		BUFFER_GUARD(size + offset <= Size, "[Buffer.Range] Out of range!");
		return { At(offset), size };
	}

	void* MutableBuffer::At(uint64_t byteOffset) const
	{
		return static_cast<uint8_t*>(Data) + byteOffset;
	}

	Buffer MutableBuffer::AsBuffer() const
	{
		return { Data, Size };
	}

	MutableBuffer MutableBuffer::Copy(const Buffer data)
	{
		MutableBuffer result;
		result.Allocate(data.Size);
		result.Write(data.Data, data.Size);
		return result;
	}

	MutableBuffer MutableBuffer::Copy(const void* data, uint64_t size)
	{
		MutableBuffer result;
		result.Allocate(size);
		result.Write(data, size);
		return result;
	}

	#pragma endregion

	//////////////////////////////////////////////////////////////////////////
	//// Unique Buffer
	//////////////////////////////////////////////////////////////////////////

	#pragma region Unique Buffer

	UniqueBuffer::UniqueBuffer(MutableBuffer&& buffer)
		: MutableBuffer(std::exchange(buffer.Data, nullptr),
						std::exchange(buffer.Size, 0))
	{
	}

	UniqueBuffer::~UniqueBuffer()
	{
		Release();
	}

	UniqueBuffer::UniqueBuffer(UniqueBuffer&& other) noexcept
		: MutableBuffer(std::exchange(other.Data, nullptr),
						std::exchange(other.Size, 0))
	{
	}

	UniqueBuffer& UniqueBuffer::operator=(UniqueBuffer&& other) noexcept
	{
		if (this == std::addressof(other))
			return *this;

		Release();
		Data = std::exchange(other.Data, nullptr);
		Size = std::exchange(other.Size, 0);
		return *this;
	}

	MutableBuffer UniqueBuffer::ExtractBuffer()
	{
		MutableBuffer result{ Data, Size };
		Data = nullptr;
		Size = 0;
		return result;
	}

	Buffer UniqueBuffer::AsBuffer() const
	{
		return { Data, Size };
	}

	UniqueBuffer UniqueBuffer::Copy(const Buffer buffer)
	{
		UniqueBuffer temp;
		temp.Allocate(buffer.Size);
		temp.Write(buffer.Data, buffer.Size);
		return temp;
	}

	UniqueBuffer UniqueBuffer::Copy(const void* data, uint64_t size)
	{
		UniqueBuffer temp;
		temp.Allocate(size);
		temp.Write(data, size);
		return temp;
	}

#pragma endregion

	//////////////////////////////////////////////////////////////////////////
	//// Buffer Handle
	//////////////////////////////////////////////////////////////////////////

	#pragma region Buffer Handle

	BufferHandle::BufferHandle(Buffer buffer)
		:
		m_ConstData(buffer.Data),
		m_Size(buffer.Size)
	{
	}

	BufferHandle::BufferHandle(MutableBuffer buffer)
		:
		m_ConstData(buffer.Data),
		m_MutableData(buffer.Data),
		m_Size(buffer.Size)
	{
	}

	BufferHandle::BufferHandle(UniqueBuffer&& buffer)
		: BufferHandle(buffer.ExtractBuffer())
	{
		m_Stored = true;
		m_Free = true;
	}

	BufferHandle::BufferHandle(BufferHandle&& other)
		:
		m_ConstData(std::exchange(other.m_ConstData, nullptr)),
		m_MutableData(std::exchange(other.m_MutableData, nullptr)),
		m_Size(std::exchange(other.m_Size, 0)),
		m_Stored(std::exchange(other.m_Stored, false)),
		m_Free(std::exchange(other.m_Free, false))
	{
	}

	BufferHandle& BufferHandle::operator=(BufferHandle&& other)
	{
		if (this == std::addressof(other))
			return *this;

		m_ConstData = std::exchange(other.m_ConstData, nullptr);
		m_MutableData = std::exchange(other.m_MutableData, nullptr);
		m_Size = std::exchange(other.m_Size, 0);
		m_Stored = std::exchange(other.m_Stored, false);
		m_Free = std::exchange(other.m_Free, false);
		return *this;
	}

	BufferHandle::~BufferHandle()
	{
		if (!m_Free)
			return;

		// this code is only executed when the memory is taken from a UniqueBuffer
		Allocator::Free(const_cast<void*>(m_ConstData));
	}

	BufferHandle BufferHandle::Alive(const Buffer buffer)
	{
		BufferHandle temp;
		temp.m_ConstData = buffer.Data;
		temp.m_Size = buffer.Size;
		temp.m_Stored = true;
		temp.m_Free = false;
		return temp;
	}

	BufferHandle BufferHandle::Alive(const MutableBuffer buffer)
	{
		BufferHandle temp;
		temp.m_ConstData = buffer.Data;
		temp.m_MutableData = buffer.Data;
		temp.m_Size = buffer.Size;
		temp.m_Stored = true;
		temp.m_Free = false;
		return temp;
	}

	Buffer BufferHandle::AsBuffer() const
	{
		return { m_ConstData, m_Size };
	}

	MutableBuffer BufferHandle::AsMutableBuffer() const
	{
		if (m_MutableData)
			return { m_MutableData, m_Size };
		return {};
	}

	bool BufferHandle::IsEmpty() const
	{
		return m_Size == 0;
	}

	bool BufferHandle::IsMutable() const
	{
		return m_MutableData != nullptr;
	}

	BufferHandle BufferHandle::Store()
	{
		return Store(*this);
	}

	BufferHandle BufferHandle::Store(BufferHandle& handle)
	{
		if (handle.m_Stored)
			return std::move(handle);

		auto local = std::move(handle);
		return UniqueBuffer::Copy(local.AsBuffer());
	}

	#pragma endregion

}
