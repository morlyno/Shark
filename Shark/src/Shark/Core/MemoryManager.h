#pragma once

#define SK_CURR_MEMROY ::Shark::MemoryManager::GetMetrics()
#define SK_LOG_MEMORY(memory) \
{ \
	const auto&& m = (memory); \
	SK_CORE_INFO("========== Memory =========="); \
	SK_CORE_INFO("Memory Uage:       {0}", m.MemoryUsage()); \
	SK_CORE_INFO("Memory Allocated:  {0}", m.MemoryAllocated); \
	SK_CORE_INFO("Memory Freed:      {0}", m.MemoryFreed); \
	SK_CORE_INFO("Total Diff:        {0}", m.TotalAllocated - m.TotalFreed); \
	SK_CORE_INFO("Total Allocated:   {0}", m.TotalAllocated); \
	SK_CORE_INFO("Total Freed:       {0}", m.TotalFreed); \
}

namespace Shark {

	struct MemoryMetrics
	{
		uint64_t TotalAllocated = 0;
		uint64_t TotalFreed = 0;

		uint64_t MemoryAllocated = 0;
		uint64_t MemoryFreed = 0;

		uint64_t MemoryUsage() const { return MemoryAllocated - MemoryFreed; }
	};

	class MemoryManager
	{
	public:
		static void Reset();
		
		static const MemoryMetrics& GetMetrics() { return s_Memory; }
	private:
		static MemoryMetrics s_Memory;

		friend class Allocator;
	};

}