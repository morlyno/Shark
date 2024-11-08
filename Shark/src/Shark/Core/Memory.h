#pragma once

#include <mutex>
#include <map>
#include <unordered_map>

namespace Shark {

	class Allocator;

	namespace Internal {

		template<class T>
		class UntrackedAllocator
		{
		public:
			using value_type = T;

			constexpr UntrackedAllocator()
			{
			};

			template<class U>
			constexpr UntrackedAllocator(const UntrackedAllocator <U>&) noexcept
			{
			}

			[[nodiscard]] T* allocate(std::size_t n)
			{
				if (n > std::numeric_limits<std::size_t>::max() / sizeof(T))
					throw std::bad_array_new_length();

				return (T*)Allocator::AllocateRaw(n * sizeof(T));
			}

			void deallocate(T* p, std::size_t n) noexcept
			{
				Allocator::FreeRaw(p);
			}
		};

		template<typename TKey, typename TValue>
		using UntrackedMapAllocator = UntrackedAllocator<std::pair<const TKey, TValue>>;

	}

	struct MemoryStats
	{
		uint64_t TotalAllocated = 0;
		uint64_t TotalFreed = 0;
		uint64_t CurrentUsage() const { return TotalAllocated - TotalFreed; }
	};

	struct Allocation
	{
		void* Memory;
		size_t Size;
		const char* Descriptor;
		const char* Module;
		int Line = -1;
	};

	struct AllocatorData
	{
		using AllocationMap = std::unordered_map<void*, Allocation, std::hash<void*>, std::equal_to<void*>, Internal::UntrackedAllocator<std::pair<void* const, Allocation>>>;
		using AllocationStatsMap = std::unordered_map<const char*, uint64_t, std::hash<const char*>, std::equal_to<const char*>, Internal::UntrackedAllocator<std::pair<const char* const, uint64_t>>>;

		std::recursive_mutex m_Mutex;
		AllocationMap m_AllocationMap;
		AllocationStatsMap m_AllocationStatsMap;
		MemoryStats m_MemoryStats;
	};

	class Allocator
	{
	public:
		static void Init();
		
		static void* AllocateRaw(size_t size);
		static void FreeRaw(void* memory);

		static void* Allocate(size_t size);
		static void* Allocate(size_t size, const char* desc);
		static void* Allocate(size_t size, const char* file, int line);
		static void Free(void* memory);

		static void* Reallocate(void* memory, size_t newSize);
		static void* Reallocate(void* memory, size_t newSize, const char* desc);
		static void* Reallocate(void* memory, size_t newSize, const char* file, int line);

		static void* ModuleAllocate(const char* moduleName, size_t size, const char* descOrFile = s_DefaultDescriptor, int line = -1);
		static void* ModuleReallocate(const char* moduleName, void* memory, size_t newSize, const char* descOrFile = s_DefaultDescriptor, int line = -1);
		static void ModuleFree(const char* moduleName, void* memory);

		static const MemoryStats& GetMemoryStats() { return s_Data->m_MemoryStats; }
		static const AllocatorData::AllocationStatsMap& GetAllocationStatsMap() { return s_Data->m_AllocationStatsMap; }
		static const AllocatorData::AllocationMap& GetAllocationMap() { return s_Data->m_AllocationMap; }

	private:
		inline static AllocatorData* s_Data = nullptr;
		inline static const char* s_DefaultDescriptor = "Default";
	};

}

#if SK_TRACK_MEMORY

_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(size) _VCRT_ALLOCATOR
void* __CRTDECL operator new(size_t size);

_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(size) _VCRT_ALLOCATOR
void* __CRTDECL operator new[](size_t size);

_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(size) _VCRT_ALLOCATOR
void* __CRTDECL operator new(size_t size, const char* desc);

_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(size) _VCRT_ALLOCATOR
void* __CRTDECL operator new[](size_t size, const char* desc);

_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(size) _VCRT_ALLOCATOR
void* __CRTDECL operator new(size_t size, const char* file, int line);

_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(size) _VCRT_ALLOCATOR
void* __CRTDECL operator new[](size_t size, const char* file, int line);

void __CRTDECL operator delete(void* memory) noexcept;
void __CRTDECL operator delete(void* memory, const char* desc) noexcept;
void __CRTDECL operator delete(void* memory, const char* file, int line) noexcept;
void __CRTDECL operator delete[](void* memory) noexcept;
void __CRTDECL operator delete[](void* memory, const char* desc) noexcept;
void __CRTDECL operator delete[](void* memory, const char* file, int line) noexcept;

#define sknew new(__FILE__, __LINE__)
#define skdelete delete

#else

#define sknew new
#define skdelete delete

#endif
