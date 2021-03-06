#pragma once

namespace Shark {

	class Allocator
	{
	public:
		static void* Allocate(size_t size);
		static void Free(void* block);
	};

}
