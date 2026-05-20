#include "skpch.h"
#include "FileStream.h"

namespace Shark {

	FileStreamReader::FileStreamReader(const std::filesystem::path& filepath)
		: m_Path(filepath)
	{
		m_Stream = std::ifstream(filepath, std::ios::in | std::ios::binary);
	}

	FileStreamReader::~FileStreamReader()
	{
		m_Stream.close();
	}

	void FileStreamReader::SetStreamPosition(uint64_t position, SeekOrigin origin)
	{
		switch (origin)
		{
			case SeekOrigin::Start:   m_Stream.seekg(position, std::ios::beg); break;
			case SeekOrigin::End:     m_Stream.seekg(position, std::ios::end); break;
			case SeekOrigin::Current: m_Stream.seekg(position, std::ios::cur); break;
		}
	}

	bool FileStreamReader::ReadData(void* destination, uint64_t size)
	{
		m_Stream.read(static_cast<char*>(destination), size);
		return true;
	}

	bool FileStreamReader::ReadData(void* destination, uint64_t size, uint64_t& bytesRead)
	{
		if (ReadData(destination, size))
		{
			bytesRead = m_Stream.gcount();
			return true;
		}

		bytesRead = 0;
		return false;
	}

	FileStreamWriter::FileStreamWriter(const std::filesystem::path& filepath)
		: m_Path(filepath)
	{
		m_Stream = std::ofstream(filepath, std::ios::out | std::ios::binary);
	}

	FileStreamWriter::~FileStreamWriter()
	{
		m_Stream.close();
	}

	bool FileStreamWriter::WriteData(const void* data, uint64_t size)
	{
		m_Stream.write(static_cast<const char*>(data), size);
		return true;
	}

}
