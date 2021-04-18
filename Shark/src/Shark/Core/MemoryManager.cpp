#include "skpch.h"
#include "MemoryManager.h"

namespace Shark {

	MemoryMetrics MemoryManager::s_Memory;

	void MemoryManager::Reset()
	{ 
		memset(&s_Memory, 0, sizeof(MemoryMetrics));
	}

}