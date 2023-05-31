#include "skpch.h"
#include "Memory.h"

namespace Shark {

	void Allocator::Init()
	{
		s_Data = (AllocatorData*)AllocateRaw(sizeof(AllocatorData));
		new(s_Data) AllocatorData{};
	}

	void* Allocator::AllocateRaw(size_t size)
	{
		return malloc(size);
	}

	void Allocator::FreeRaw(void* memory)
	{
		free(memory);
	}

	void* Allocator::Allocate(size_t size)
	{
		if (!s_Data)
			Init();

		void* memory = malloc(size);

		{
			std::scoped_lock lock(s_Data->m_Mutex);
			Allocation& allocation = s_Data->m_AllocationMap[memory];
			allocation.Memory = memory;
			allocation.Size = size;
			allocation.Descriptor = s_NullDesc;

			s_Data->m_AllocationStatsMap[s_NullDesc] += size;
			s_Data->m_MemoryStats.TotalAllocated += size;
		}

		return memory;
	}

	void* Allocator::Allocate(size_t size, const char* desc)
	{
		if (!s_Data)
			Init();

		void* memory = malloc(size);

		{
			std::scoped_lock lock(s_Data->m_Mutex);
			Allocation& allocation = s_Data->m_AllocationMap[memory];
			allocation.Memory = memory;
			allocation.Size = size;
			allocation.Descriptor = desc ? desc : s_NullDesc;

			s_Data->m_AllocationStatsMap[allocation.Descriptor] += size;
			s_Data->m_MemoryStats.TotalAllocated += size;
		}

		return memory;
	}

	void* Allocator::Allocate(size_t size, const char* file, int line)
	{
		if (!s_Data)
			Init();

		void* memory = malloc(size);

		{
			std::scoped_lock lock(s_Data->m_Mutex);
			Allocation& allocation = s_Data->m_AllocationMap[memory];
			allocation.Memory = memory;
			allocation.Size = size;
			allocation.Descriptor = file;
			allocation.Line = line;

			s_Data->m_AllocationStatsMap[file] += size;
			s_Data->m_MemoryStats.TotalAllocated += size;
		}

		return memory;
	}

	void Allocator::Free(void* memory)
	{
		if (!memory)
			return;

		if (!s_Data)
			Init();

		bool found = false;
		{
			std::scoped_lock lock(s_Data->m_Mutex);
			auto allocIter = s_Data->m_AllocationMap.find(memory);
			found = allocIter != s_Data->m_AllocationMap.end();
			if (found)
			{
				Allocation& alloc = allocIter->second;
				s_Data->m_MemoryStats.TotalFreed += alloc.Size;
				if (alloc.Descriptor)
					s_Data->m_AllocationStatsMap.at(alloc.Descriptor) -= alloc.Size;

				s_Data->m_AllocationMap.erase(allocIter);
			}
		}

		if (!found)
		{
			SK_CORE_WARN_TAG("Memory", "Memory block {} not found in Allocation Map", memory);
		}

		free(memory);
	}

	void* Allocator::Reallocate(void* memory, size_t newSize)
	{
		return InternalReallocate(memory, newSize);
	}

	void* Allocator::Reallocate(void* memory, size_t newSize, const char* desc)
	{
		return InternalReallocate(memory, newSize, desc);
	}

	void* Allocator::Reallocate(void* memory, size_t newSize, const char* file, int line)
	{
		return InternalReallocate(memory, newSize, file, line);
	}

	void* Allocator::InternalReallocate(void* memory, size_t newSize, const char* descOrFile, int line)
	{
		if (!s_Data)
			Init();

		void* newMemory = realloc(memory, newSize);

		bool found = memory;
		{
			std::scoped_lock lock(s_Data->m_Mutex);
			if (memory)
			{
				auto allocIter = s_Data->m_AllocationMap.find(memory);
				found = allocIter != s_Data->m_AllocationMap.end();
				if (found)
				{
					Allocation& alloc = allocIter->second;
					s_Data->m_MemoryStats.TotalFreed += alloc.Size;
					if (alloc.Descriptor)
						s_Data->m_AllocationStatsMap.at(alloc.Descriptor) -= alloc.Size;

					s_Data->m_AllocationMap.erase(allocIter);
				}
			}

			Allocation& alloc = s_Data->m_AllocationMap[newMemory];
			alloc.Memory = newMemory;
			alloc.Size = newSize;
			alloc.Descriptor = descOrFile;
			alloc.Line = line;

			s_Data->m_AllocationStatsMap[alloc.Descriptor] += newSize;
			s_Data->m_MemoryStats.TotalAllocated += newSize;
		}

		if (!found && memory)
		{
			SK_CORE_WARN_TAG("Memory", "Memory block {} not found in Allocation Map", memory);
		}

		return newMemory;
	}

}

#if SK_TRACK_MEMORY

_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(size) _VCRT_ALLOCATOR
void* __CRTDECL operator new(size_t size)
{
	return Shark::Allocator::Allocate(size);
}

_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(size) _VCRT_ALLOCATOR
void* __CRTDECL operator new[](size_t size)
{
	return Shark::Allocator::Allocate(size);
}

_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(size) _VCRT_ALLOCATOR
void* __CRTDECL operator new(size_t size, const char* desc)
{
	return Shark::Allocator::Allocate(size, desc);
}

_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(size) _VCRT_ALLOCATOR
void* __CRTDECL operator new[](size_t size, const char* desc)
{
	return Shark::Allocator::Allocate(size, desc);
}

_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(size) _VCRT_ALLOCATOR
void* __CRTDECL operator new(size_t size, const char* file, int line)
{
	return Shark::Allocator::Allocate(size, file, line);
}

_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(size) _VCRT_ALLOCATOR
void* __CRTDECL operator new[](size_t size, const char* file, int line)
{
	return Shark::Allocator::Allocate(size, file, line);
}

void __CRTDECL operator delete(void* memory) noexcept
{
	Shark::Allocator::Free(memory);
}

void __CRTDECL operator delete(void* memory, const char* desc) noexcept
{
	Shark::Allocator::Free(memory);
}

void __CRTDECL operator delete(void* memory, const char* file, int line) noexcept
{
	Shark::Allocator::Free(memory);
}

void __CRTDECL operator delete[](void* memory) noexcept
{
	Shark::Allocator::Free(memory);
}

void __CRTDECL operator delete[](void* memory, const char* desc) noexcept
{
	Shark::Allocator::Free(memory);
}

void __CRTDECL operator delete[](void* memory, const char* file, int line) noexcept
{
	Shark::Allocator::Free(memory);
}

#endif
