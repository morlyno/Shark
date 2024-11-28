#include "skpch.h"
#include "Memory.h"

#if SK_ENABLE_PROFILER
	#include <tracy/Tracy.hpp>
#endif

namespace Shark {

#if SK_ENABLE_PROFILER
#define PROFILE_ALLOCATE(_memory, _size, _module) TracyAllocN(_memory, _size, _module)
#define PROFILE_FREE(_memory, _module) TracyFreeN(_memory, _module)
#else
#define PROFILE_ALLOCATE(_memory, _size, _module) (void)0
#define PROFILE_FREE(_memory, _module) (void)0
#endif

	void Allocator::Init()
	{
		s_Data = (AllocatorData*)AllocateRaw(sizeof(AllocatorData));
		new(s_Data) AllocatorData{};
	}

	void* Allocator::AllocateRaw(size_t size)
	{
		void* memory = malloc(size);
		PROFILE_ALLOCATE(memory, size, "Raw");
		return memory;
	}

	void Allocator::FreeRaw(void* memory)
	{
		free(memory);
		PROFILE_FREE(memory, "Raw");
	}

	void* Allocator::Allocate(size_t size)
	{
		return ModuleAllocate("Shark", size);
	}

	void* Allocator::Allocate(size_t size, const char* desc)
	{
		return ModuleAllocate("Shark", size, desc);
	}

	void* Allocator::Allocate(size_t size, const char* file, int line)
	{
		return ModuleAllocate("Shark", size, file, line);
	}

	void Allocator::Free(void* memory)
	{
		ModuleFree("Shark", memory);
	}

	void* Allocator::Reallocate(void* memory, size_t newSize)
	{
		return ModuleReallocate("Shark", memory, newSize);
	}

	void* Allocator::Reallocate(void* memory, size_t newSize, const char* desc)
	{
		return ModuleReallocate("Shark", memory, newSize, desc);
	}

	void* Allocator::Reallocate(void* memory, size_t newSize, const char* file, int line)
	{
		return ModuleReallocate("Shark", memory, newSize, file, line);
	}

	void* Allocator::ModuleAllocate(const char* moduleName, size_t size, const char* descOrFile, int line)
	{
		if (!s_Data)
			Init();

		void* memory = malloc(size);
		PROFILE_ALLOCATE(memory, size, moduleName);

		{
			std::scoped_lock lock(s_Data->m_Mutex);
			SK_CORE_VERIFY(!s_Data->m_AllocationMap.contains(memory));
			Allocation& allocation = s_Data->m_AllocationMap[memory];
			allocation.Memory = memory;
			allocation.Size = size;
			allocation.Descriptor = descOrFile;
			allocation.Line = line;
			allocation.Module = moduleName;

			s_Data->m_AllocationStatsMap[descOrFile] += size;
			s_Data->m_MemoryStats.TotalAllocated += size;
		}

		return memory;
	}

	void* Allocator::ModuleReallocate(const char* moduleName, void* memory, size_t newSize, const char* descOrFile, int line)
	{
		if (!s_Data)
			Init();

		void* newMemory = realloc(memory, newSize);

		const char* allocModule = nullptr;
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

					allocModule = alloc.Module;
					s_Data->m_AllocationMap.erase(allocIter);
				}
			}

			Allocation& alloc = s_Data->m_AllocationMap[newMemory];
			alloc.Memory = newMemory;
			alloc.Size = newSize;
			alloc.Descriptor = descOrFile;
			alloc.Line = line;
			alloc.Module = moduleName;

			s_Data->m_AllocationStatsMap[alloc.Descriptor] += newSize;
			s_Data->m_MemoryStats.TotalAllocated += newSize;
		}

		if (!found && memory)
		{
			SK_CORE_WARN_TAG("Memory", "Memory block {} not found in Allocation Map", memory);
			SK_CORE_VERIFY(false);
		}

		PROFILE_FREE(memory, moduleName);
		PROFILE_ALLOCATE(newMemory, newSize, moduleName);
		return newMemory;
	}

	void Allocator::ModuleFree(const char* moduleName, void* memory)
	{
		if (!memory)
			return;

		if (!s_Data)
			Init();

		const char* allocModule = nullptr;
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

				allocModule = alloc.Module;
				s_Data->m_AllocationMap.erase(allocIter);
			}
		}

		if (!found)
		{
			SK_CORE_WARN_TAG("Memory", "Memory block {} not found in Allocation Map", memory);
			SK_CORE_VERIFY(false);
		}

		free(memory);
		PROFILE_FREE(memory, allocModule);
	}

}

#if SK_TRACK_MEMORY

_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(size) _VCRT_ALLOCATOR
void* __CRTDECL operator new(size_t size)
{
	return Shark::Allocator::ModuleAllocate("Default", size);
}

_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(size) _VCRT_ALLOCATOR
void* __CRTDECL operator new[](size_t size)
{
	return Shark::Allocator::ModuleAllocate("Default", size);
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
