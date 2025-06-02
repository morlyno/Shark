#pragma once

namespace Shark {

	class StreamReader
	{
	public:
		virtual ~StreamReader() = default;

		virtual bool IsStreamGood() const = 0;
		virtual uint64_t GetStreamPosition() = 0;
		virtual void SetStreamPosition(uint64_t position) = 0;
		virtual bool ReadData(void* destination, uint64_t size) = 0;

		bool ReadBuffer(Buffer& buffer);
		bool ReadString(std::string& string);

		template<typename T>
		bool ReadRaw(T& data)
		{
			return ReadData(&data, sizeof(T));
		}

	};

}
