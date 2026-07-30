#include "skpch.h"
#include "StringStream.h"

#include "Shark/File/FileSystem.h"

namespace Shark {

	StringStreamWriter::StringStreamWriter(bool asBinary)
		: m_Stream(std::ios::out | (asBinary ? std::ios::binary : 0))
	{
	}

	bool StringStreamWriter::WriteData(const void* data, uint64_t size)
	{
		m_Stream.write(static_cast<const char*>(data), size);
		return true;
	}

	void StringStreamWriter::WriteToDisc(const std::filesystem::path& filepath) const
	{
		FileSystem::WriteString(filepath, View());
	}

}
