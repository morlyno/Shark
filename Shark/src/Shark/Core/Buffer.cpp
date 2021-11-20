#include "skpch.h"
#include "Buffer.h"

namespace Shark {

	Buffer::Buffer()
	{
	}

	Buffer::~Buffer()
	{
		Release();
	}

	Buffer::Buffer(const Buffer& other)
	{
		Allocate(other.m_Size);
		m_Size = other.m_Size;
		memcpy(m_Data, other.m_Data, m_Size);
	}

	Buffer::Buffer(Buffer&& other)
	{
		m_Size = other.m_Size;
		m_Data = other.m_Data;
		other.m_Size = 0;
		other.m_Data = nullptr;
	}

	Buffer& Buffer::operator=(const Buffer& other)
	{
		if (m_Size != other.m_Size)
		{
			Release();
			Allocate(other.m_Size);
		}

		m_Size = other.m_Size;
		memcpy(m_Data, other.m_Data, m_Size);
		return *this;
	}

	Buffer& Buffer::operator=(Buffer&& other)
	{
		if (m_Data)
			Release();

		m_Size = other.m_Size;
		m_Data = other.m_Data;
		other.m_Size = 0;
		other.m_Data = nullptr;
		return *this;
	}

	void Buffer::Allocate(uint32_t size)
	{
		SK_CORE_ASSERT(!m_Data);
		if (m_Data)
		{
			// TODO(moro): let buffer grow;
			return;
		}

		m_Size = size;
		m_Data = (byte*)operator new(size);
	}

	void Buffer::Release()
	{
		operator delete(m_Data);
		m_Data = nullptr;
		m_Size = 0;
	}

	void Buffer::Write(void* data, uint32_t size, uint32_t offset)
	{
		SK_CORE_ASSERT((size + offset) <= m_Size);
		if ((size + offset) > m_Size)
			return;

		memcpy(m_Data + offset, data, size);
	}

}

