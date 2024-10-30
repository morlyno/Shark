#include "skpch.h"
#include "Buffer.h"

namespace Shark {

	void Buffer::Allocate(uint64_t size)
	{
		Release();

		if (size)
		{
			Size = size;
			Data = sknew byte[size];
		}
	}

	void Buffer::Resize(uint64_t newSize, bool canShrink)
	{
		if (newSize == Size || !canShrink && newSize < Size)
			return;

		byte* newData = sknew byte[newSize];
		memcpy(newData, Data, std::min(Size, newSize));
		skdelete[] Data;
		Data = newData;
		Size = newSize;
	}

	void Buffer::Release()
	{
		if (!Data)
			return;

		skdelete[] Data;
		Data = nullptr;
		Size = 0;
	}

	void Buffer::Write(const void* data, uint64_t size, uint64_t offset)
	{
		SK_CORE_VERIFY((size + offset) <= Size, "Out of range! ({0} + {1}) <= {2}", size, offset, Size);
		if ((size + offset) > Size)
			return;

		memcpy(Data + offset, data, size);
	}

	void Buffer::Write(const byte* data, uint64_t size, uint64_t offset)
	{
		Write((const void*)data, size, offset);
	}

	void Buffer::Read(void* resultBuffer, uint64_t size, uint64_t offset)
	{
		SK_CORE_VERIFY(offset + size <= Size, "Out of range! ({0} + {1}) <= {2}", size, offset, Size);
		memcpy(resultBuffer, Data + offset, size);
	}

	Buffer Buffer::SubBuffer(uint32_t offset, uint32_t size)
	{
		SK_CORE_ASSERT((offset + size) <= Size);
		return Buffer(Data + offset, size);
	}

	void Buffer::SetZero()
	{
		memset(Data, 0, Size);
	}

	Buffer Buffer::Copy(const byte* data, uint64_t Size)
	{
		Buffer buffer;
		buffer.Allocate(Size);
		buffer.Write(data, Size);
		return buffer;
	}

	Buffer Buffer::Copy(Buffer buffer)
	{
		return Copy(buffer.Data, buffer.Size);
	}

	Buffer Buffer::New(uint64_t size, bool setZero)
	{
		Buffer buffer;
		buffer.Allocate(size);

		if (setZero)
			buffer.SetZero();

		return buffer;
	}

}

