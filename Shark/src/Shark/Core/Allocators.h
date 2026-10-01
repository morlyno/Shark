#pragma once

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
