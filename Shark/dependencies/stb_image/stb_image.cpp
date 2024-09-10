#include "skpch.h"

#include "Shark/Core/Memory.h"
#define STBI_MALLOC(_size)              ::Shark::Allocator::ModuleAllocate("stb_image", _size, __FILE__, __LINE__)
#define STBI_REALLOC(_memory, _newSize) ::Shark::Allocator::ModuleReallocate("stb_image", _memory, _newSize, __FILE__, __LINE__)
#define STBI_FREE(_memory)              ::Shark::Allocator::ModuleFree("stb_image", _memory)

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"