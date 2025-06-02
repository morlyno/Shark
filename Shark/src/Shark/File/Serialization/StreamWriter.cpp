#include "skpch.h"
#include "StreamWriter.h"

namespace Shark {

	void StreamWriter::WriteBuffer(const Buffer buffer)
	{
		WriteData(&buffer.Size, sizeof(uint64_t));
		WriteData(buffer.Data, buffer.Size);
	}

	void StreamWriter::WriteZero(uint64_t size)
	{
		char zero = 0;
		for (uint32_t i = 0; i < size; i++)
			WriteData(&zero, 1);
	}

	void StreamWriter::WriteString(const std::string& string)
	{
		uint64_t size = string.size();
		WriteData(&size, sizeof(uint64_t));
		WriteData(string.data(), size * sizeof(char));
	}

}
