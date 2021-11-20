#include "skpch.h"
#include "Allocator.h"

#include "MemoryManager.h"

#include <stdlib.h>

#define SK_MEMORY_ALIGNMENT 16
#define SK_ALLOC(size)		malloc(size)
#define SK_FREE(block)		free(block)

namespace Shark {

	using byte = unsigned char;
	
	void* Allocator::Allocate(size_t size)
	{
		size_t actualsize = size + sizeof(size_t);
		byte* data = (byte*)SK_ALLOC(actualsize);
		//memset(data, 0, actualsize);
		memcpy(data, &size, sizeof(size_t));
		data += sizeof(size_t);

		MemoryManager::s_Memory.MemoryAllocated += size;
		MemoryManager::s_Memory.TotalAllocated++;

		return data;
	}

	void Allocator::Free(void* block)
	{
		if (block)
		{
			byte* actualbock = (byte*)block - sizeof(size_t);
			size_t size = *(size_t*)actualbock;
			SK_FREE(actualbock);

			MemoryManager::s_Memory.MemoryFreed += size;
			MemoryManager::s_Memory.TotalFreed++;
		}
	}

}

#if SK_ENABLE_MEMORY_TRACING

void* operator new(size_t size) { return Shark::Allocator::Allocate(size); }
void* operator new[](size_t size) { return Shark::Allocator::Allocate(size); }

void operator delete(void* block) { Shark::Allocator::Free(block); }
void operator delete[](void* block) { Shark::Allocator::Free(block); }

#endif
