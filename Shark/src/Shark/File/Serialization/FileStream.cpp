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

	bool FileStreamReader::ReadData(void* destination, uint64_t size)
	{
		m_Stream.read((char*)destination, size);
		return true;
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
		m_Stream.write((const char*)data, size);
		return true;
	}

}
