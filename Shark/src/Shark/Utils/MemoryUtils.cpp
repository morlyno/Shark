#include "skpch.h"
#include "MemoryUtils.h"

namespace Shark {

	void MemoryUtils::ZeroMemory(void* data, uint32_t size)
	{
		memset(data, 0, size);
	}

}
