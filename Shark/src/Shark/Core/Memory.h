#pragma once

void* operator new(size_t size);

void* operator new[](size_t size);

void operator delete(void* block);

void operator delete[](void* block);
