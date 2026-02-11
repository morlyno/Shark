#pragma once

#include "Shark/String/TokenStreamReader.h"

#include <regex>

namespace Shark::String {

	class RegexStreamReader : public ITokenStreamReader
	{
	public:
		using RegexIterator = std::regex_iterator<std::string_view::iterator>;

		RegexStreamReader(const std::regex& tokenPattern);
		RegexStreamReader(const std::regex& tokenPattern, std::string_view source);

	public:
		virtual void SetSource(std::string_view source, bool keepPosition) override;
		virtual bool IsStreamGood() const override { return m_Stream != m_End; }
		virtual bool Advance(size_t tokenCount = 1) override;
		virtual bool Read(std::string_view& outToken) override;
		bool ReadCurrent(std::string_view& outToken) const;

		virtual StreamPosition GetStreamPosition() const override;
		virtual void SetStreamPosition(StreamPosition position) override;
		virtual size_t GetSourcePosition() const override;

		std::string ToString(std::string_view separator) const;

	private:
		std::string_view GetToken(size_t offset) const;
		virtual bool Compare(std::string_view token, size_t offset) const override;
		virtual bool Compare(const std::regex& pattern, size_t offset) const override;

		bool AdvancePosition(size_t tokenCount);

	private:

		const std::regex& m_Regex;

		std::string_view m_Source;
		RegexIterator m_Begin;
		RegexIterator m_End;

		RegexIterator m_Stream;
		size_t m_Position;

	};

}
