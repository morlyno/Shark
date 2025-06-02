#include "skpch.h"
#include "StreamReader.h"

namespace Shark {

	bool StreamReader::ReadBuffer(Buffer& buffer)
	{
		if (!ReadData(&buffer.Size, sizeof(uint64_t)))
			return false;

		buffer.Allocate(buffer.Size);
		return ReadData(buffer.Data, buffer.Size);
	}

	bool StreamReader::ReadString(std::string& string)
	{
		uint64_t size;
		if (!ReadData(&size, sizeof(uint64_t)))
			return false;

		string.resize(size);
		return ReadData(string.data(), size * sizeof(char));
	}

}
