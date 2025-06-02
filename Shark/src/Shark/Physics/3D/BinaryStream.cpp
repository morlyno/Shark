#include "skpch.h"
#include "BinaryStream.h"

namespace Shark {

	JoltBinaryStreamWriter::JoltBinaryStreamWriter()
		: m_Buffer(Buffer::New(1024)), m_Stream(m_Buffer)
	{
	}

	JoltBinaryStreamWriter::~JoltBinaryStreamWriter()
	{
		m_Buffer.Release();
	}

	void JoltBinaryStreamWriter::WriteBytes(const void* inData, size_t inNumBytes)
	{
		if (m_Stream.GetStreamPosition() + inNumBytes > m_Buffer.Size)
		{
			const uint64_t newSize = std::max(m_Buffer.Size + m_Buffer.Size / 2, m_Stream.GetStreamPosition() + inNumBytes);
			//const uint64_t newSize = m_Stream.GetStreamPosition() + inNumBytes;
			m_Buffer.Resize(newSize);
		}

		m_Stream.WriteData(inData, inNumBytes);
	}

	bool JoltBinaryStreamWriter::IsFailed() const
	{
		return !m_Stream.IsStreamGood();
	}



	JoltBinaryStreamReader::JoltBinaryStreamReader(const Buffer buffer)
		: m_Stream(buffer)
	{
	}

	void JoltBinaryStreamReader::ReadBytes(void* outData, size_t inNumBytes)
	{
		m_Stream.ReadData(outData, inNumBytes);
	}

	bool JoltBinaryStreamReader::IsEOF() const
	{
		return !m_Stream.IsStreamGood();
	}

	bool JoltBinaryStreamReader::IsFailed() const
	{
		return !m_Stream.IsStreamGood();
	}

}
