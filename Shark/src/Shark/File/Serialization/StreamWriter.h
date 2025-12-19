#pragma once

namespace Shark {

	class StreamWriter
	{
	public:
		virtual ~StreamWriter() = default;

		virtual void Flush() = 0;
		virtual bool IsStreamGood() const = 0;
		virtual uint64_t GetStreamPosition() = 0;
		virtual void SetStreamPosition(uint64_t position) = 0;
		virtual bool WriteData(const char* data, uint64_t size) = 0;

		void WriteBuffer(Buffer buffer);
		void WriteZero(uint64_t size);
		void WriteString(const std::string& string);

		template<typename T>
		bool WriteRaw(const T& value)
		{
			return WriteData((const char*)&value, sizeof(T));
		}

	};

}
