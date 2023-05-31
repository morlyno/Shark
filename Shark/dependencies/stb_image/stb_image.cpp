#include "skpch.h"

#include "Shark/Core/Memory.h"
static const char* s_stb_image_desc = "stb_image";
#define STBI_MALLOC(sz)           Shark::Allocator::Allocate(sz, s_stb_image_desc, __LINE__);
#define STBI_REALLOC(p,newsz)     Shark::Allocator::Reallocate(p, newsz, s_stb_image_desc, __LINE__);
#define STBI_FREE(p)              Shark::Allocator::Free(p);

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"