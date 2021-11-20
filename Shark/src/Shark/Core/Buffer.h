#pragma once

#include "Shark/Core/Base.h"

namespace Shark {

	class Buffer
	{
	public:
		Buffer();
		~Buffer();

		Buffer(const Buffer& other);
		Buffer(Buffer&& other);
		Buffer& operator=(const Buffer& other);
		Buffer& operator=(Buffer&& other);

		void Allocate(uint32_t size);
		void Release();
		void Write(void* data, uint32_t size, uint32_t offset = 0);

		byte* const& Data = m_Data;
		const uint32_t& Size = m_Size;

	private:
		byte* m_Data = nullptr;
		uint32_t m_Size = 0;
	};

}
