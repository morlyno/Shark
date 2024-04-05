#include "skpch.h"
#include "StreamReader.h"

namespace Shark {

	bool StreamReader::ReadBuffer(Buffer& buffer)
	{
		if (!ReadData((char*)&buffer.Size, sizeof(uint64_t)))
			return false;

		buffer.Allocate(buffer.Size);
		return ReadData((char*)buffer.Data, buffer.Size);
	}

	bool StreamReader::ReadString(std::string& string)
	{
		uint64_t size;
		if (!ReadData((char*)&size, sizeof(uint64_t)))
			return false;

		string.resize(size);
		return ReadData((char*)string.data(), size * sizeof(char));
	}

}
