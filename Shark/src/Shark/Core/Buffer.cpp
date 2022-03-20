#include "skpch.h"
#include "Buffer.h"

namespace Shark {

	void Buffer::Allocate(uint32_t size)
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

	void Buffer::Write(const void* data, uint32_t size, uint32_t offset)
	{
		SK_CORE_ASSERT((size + offset) <= Size);
		if ((size + offset) > Size)
			return;

		memcpy(Data + offset, data, size);
	}

}

