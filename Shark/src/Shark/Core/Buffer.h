#pragma once

#include "Shark/Core/Base.h"

namespace Shark {

	class Buffer
	{
	public:
		Buffer() = default;
		Buffer(std::nullptr_t) {}
		Buffer(byte* data) : Data(data) {}
		Buffer(byte* data, uint32_t size) : Data(data), Size(size) {}
		~Buffer() = default;

		Buffer& operator=(byte* data) { Data = data;        Size = 0; return *this; }
		Buffer& operator=(void* data) { Data = (byte*)data; Size = 0; return *this; }

		void Allocate(uint32_t size);
		void Release();
		void Write(const void* data, uint32_t size, uint32_t offset = 0);

		template<typename T>
		void Write(const T& data, uint32_t offset = 0)
		{
			Write(&data, sizeof(T), offset);
		}

		template<typename T>
		T* As() { return (T*)Data; }
		
		template<typename T>
		const T* As() const { return (const T*)Data; }

		operator byte* () { return Data; }
		operator byte* () const { return Data; }

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
