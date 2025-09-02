#include "skpch.h"
#include "Memory.h"

namespace Shark {

	void Memory::Write(void* destination, void* source, uint64_t byteSize)
	{
		memcpy(destination, source, byteSize);
	}

	void Memory::Write(void* destination, const Buffer source)
	{
		memcpy(destination, source.Data, source.Size);
	}

}
