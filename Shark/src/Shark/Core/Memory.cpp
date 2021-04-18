#include "skpch.h"
#include "Memory.h"

#include "Shark/Core/Allocator.h"

#if 1

void* operator new(size_t size)
{
	return Shark::Allocator::Allocate(size);
}

void* operator new[](size_t size)
{
	return Shark::Allocator::Allocate(size);
}

void operator delete(void* block)
{
	Shark::Allocator::Free(block);
}

void operator delete[](void* block)
{
	Shark::Allocator::Free(block);
}

#else

void* operator new(size_t size)
{
	return malloc(size);
}

void* operator new[](size_t size)
{
	return malloc(size);
}

void operator delete(void* block)
{
	free(block);
}

void operator delete[](void* block)
{
	free(block);
}

#endif
