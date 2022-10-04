#include "skpch.h"
#include "Buffer.h"

namespace Shark {

	void Buffer::Allocate(uint64_t size)
	{
		SK_CORE_ASSERT(!Data);
		if (Data)
		{
			// TODO(moro): let buffer grow;
			return;
		}

		Size = size;
		Data = (byte*)operator new(size);
	}

	void Buffer::Release()
	{
		operator delete(Data);
		Data = nullptr;
		Size = 0;
	}

	void Buffer::Write(const void* data, uint64_t size, uint64_t offset)
	{
		SK_CORE_ASSERT((size + offset) <= Size, fmt::format("({0} + {1}) <= {2}", size, offset, Size));
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

}

