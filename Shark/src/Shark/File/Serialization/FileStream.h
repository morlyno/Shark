#pragma once

#include "Shark/File/Serialization/StreamReader.h"
#include "Shark/File/Serialization/StreamWriter.h"

namespace Shark {

	class FileStreamReader : public StreamReader
	{
	public:
		FileStreamReader(const std::filesystem::path& filepath);
		FileStreamReader(const FileStreamReader&) = delete;
		~FileStreamReader();

		virtual bool IsStreamGood() const override { return m_Stream.good(); }
		virtual uint64_t GetStreamPosition() override { return m_Stream.tellg(); }
		virtual void SetStreamPosition(uint64_t position) override { m_Stream.seekg(position); }
		virtual bool ReadData(char* destination, uint64_t size) override;

	private:
		std::filesystem::path m_Path;
		std::ifstream m_Stream;
	};

	class FileStreamWriter : public StreamWriter
	{
	public:
		FileStreamWriter(const std::filesystem::path& filepath);
		FileStreamWriter(const FileStreamWriter&) = delete;
		~FileStreamWriter();

		virtual bool IsStreamGood() const override { return m_Stream.good(); }
		virtual uint64_t GetStreamPosition() override { return m_Stream.tellp(); }
		virtual void SetStreamPosition(uint64_t position) override { m_Stream.seekp(position); }
		virtual bool WriteData(const char* data, uint64_t size) override;

	private:
		std::filesystem::path m_Path;
		std::ofstream m_Stream;
	};

}
