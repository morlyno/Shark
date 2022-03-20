#pragma once

#include "Shark/Core/Base.h"

namespace Shark {

	class Buffer
	{
	public:
		Buffer() = default;
		Buffer(std::nullptr_t) {}
		Buffer(byte* data, uint32_t size) : Data(data), Size(size) {}
		~Buffer() = default;

		void Allocate(uint32_t size);
		void Release();
		void Write(const void* data, uint32_t size, uint32_t offset = 0);

		template<typename T>
		T* As() { return (T*)Data; }
		
		template<typename T>
		const T* As() const { return (const T*)Data; }

		byte* Data = nullptr;
		uint32_t Size = 0;
	};

	class ScopedBuffer
	{
	public:
		ScopedBuffer() = default;
		~ScopedBuffer()
		{
			if (m_Buffer.Data)
				m_Buffer.Release();
		}

		ScopedBuffer(const ScopedBuffer&) = delete;
		ScopedBuffer& operator=(const ScopedBuffer&) = delete;

		void Allocate(uint32_t size) { m_Buffer.Allocate(size); }
		void Release() { m_Buffer.Release(); }
		void Write(void* data, uint32_t size, uint32_t offset = 0) { m_Buffer.Write(data, size, offset); }

		byte* const& Data = m_Buffer.Data;
		const uint32_t& Size = m_Buffer.Size;

	private:
		Buffer m_Buffer;
	};

}
