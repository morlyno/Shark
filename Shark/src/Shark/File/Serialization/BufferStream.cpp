#include "skpch.h"
#include "BufferStream.h"

namespace Shark {

	BufferStreamReader::BufferStreamReader(const Buffer buffer)
		: m_Buffer(buffer)
	{
	}

	BufferStreamReader::~BufferStreamReader()
	{
	}

	bool BufferStreamReader::ReadData(void* destination, uint64_t size)
	{
		if (m_Position + size > m_Buffer.Size)
		{
			m_Good = false;
			return false;
		}

		m_Buffer.Read(destination, size, m_Position);
		m_Position += size;
		return true;
	}

	BufferStreamWriter::BufferStreamWriter(Buffer& buffer)
		: m_Buffer(buffer)
	{
	}

	BufferStreamWriter::~BufferStreamWriter()
	{
	}

	bool BufferStreamWriter::WriteData(const void* data, uint64_t size)
	{
		if (m_Position + size > m_Buffer.Size)
		{
			m_Good = false;
			return false;
		}

		m_Buffer.Write(data, size, m_Position);
		m_Position += size;
		return true;
	}

}
