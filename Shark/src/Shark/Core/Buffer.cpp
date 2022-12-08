#include "skpch.h"
#include "Buffer.h"

namespace Shark {

	void Buffer::Allocate(uint64_t size)
	{
		Release();

		if (size)
		{
			Size = size;
			Data = (byte*)operator new(size);
		}
	}

	void Buffer::Release()
	{
		operator delete(Data);
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

}

