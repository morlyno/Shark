#pragma once

//#include "Shark/File/Serialization/StreamReader.h"
#include "Shark/File/Serialization/StreamWriter.h"

namespace Shark {

	class StringStreamWriter : public StreamWriter
	{
	public:
		StringStreamWriter(bool asBinary = false);
		StringStreamWriter(const StringStreamWriter&) = delete;
		~StringStreamWriter() = default;

		virtual void Flush() override {}
		virtual bool IsStreamGood() const override                 { return m_Stream.good(); }
		virtual uint64_t GetStreamPosition() override              { return m_Stream.tellp(); }
		virtual void SetStreamPosition(uint64_t position) override { m_Stream.seekp(position); }
		virtual bool WriteData(const void* data, uint64_t size) override;
		virtual std::ostream& GetStream() override { return m_Stream; }

		auto String() const -> std::string      { return m_Stream.str(); }
		auto View()   const -> std::string_view { return m_Stream.view(); }

		void WriteToDisc(const std::filesystem::path& filepath) const;

	private:
		std::ostringstream m_Stream;
	};

}
