#include "skpch.h"
#include "Memory.h"

namespace Shark {

	static const char* s_NullDesc = "(null)";

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

			s_Data->m_AllocationStatsMap[file] += size;
			s_Data->m_MemoryStats.TotalAllocated += size;
		}

		return memory;
	}

	void* Allocator::Allocate(size_t size, const char* func, const char* file, int line)
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

}

_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(size) _VCRT_ALLOCATOR
void* __CRTDECL operator new(size_t size)
{
	//return Shark::Allocator::AllocateRaw(size);
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

_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(size) _VCRT_ALLOCATOR
void* __CRTDECL operator new(size_t size, const char* func, const char* file, int line)
{
	return Shark::Allocator::Allocate(size, func, file, line);
}

_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(size) _VCRT_ALLOCATOR
void* __CRTDECL operator new[](size_t size, const char* func, const char* file, int line)
{
	return Shark::Allocator::Allocate(size, func, file, line);
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

void __CRTDECL operator delete(void* memory, const char* func, const char* file, int line) noexcept
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

void __CRTDECL operator delete[](void* memory, const char* func, const char* file, int line) noexcept
{
	Shark::Allocator::Free(memory);
}
