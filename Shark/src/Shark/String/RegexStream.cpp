#include "skpch.h"
#include "RegexStream.h"
#include "Shark/Utils/String.h"

namespace Shark::String {

	namespace utils {

		static bool TryAdvance(RegexStreamReader::RegexIterator& iterator, size_t count)
		{
			RegexStreamReader::RegexIterator end;

			for (size_t i = 0; i < count; i++)
			{
				if (iterator == end)
					return false;
				iterator++;
			}
			return true;
		}

	}

	RegexStreamReader::RegexStreamReader(const std::regex& tokenPattern)
		: m_Regex(tokenPattern)
	{
	}

	RegexStreamReader::RegexStreamReader(const std::regex& tokenPattern, std::string_view source)
		: m_Regex(tokenPattern)
	{
		SetSource(source, false);
	}

	void RegexStreamReader::SetSource(std::string_view source, bool keepPosition)
	{
		const size_t oldPosition = m_Position;

		m_Source = source;
		m_Begin = RegexIterator(m_Source.begin(), m_Source.end(), m_Regex);
		m_End   = RegexIterator();

		m_Stream = m_Begin;
		m_Position = 0;

		if (keepPosition)
			AdvancePosition(oldPosition);

	}

	bool RegexStreamReader::Advance(size_t tokenCount)
	{
		AdvancePosition(tokenCount);
		return IsStreamGood();
	}

	bool RegexStreamReader::Read(std::string_view& outToken)
	{
		outToken = GetToken(0);
		Advance();

		return true;
	}

	bool RegexStreamReader::ReadCurrent(std::string_view& outToken) const
	{
		outToken = GetToken(0);
		return true;
	}

	StreamPosition RegexStreamReader::GetStreamPosition() const
	{
		return m_Position;
	}

	void RegexStreamReader::SetStreamPosition(StreamPosition position)
	{
		if (m_Position == position.Offset)
			return;

		m_Stream = m_Begin;
		m_Position = 0;
		AdvancePosition(position.Offset);
	}

	size_t RegexStreamReader::GetSourcePosition() const
	{
		if (m_Stream == m_End)
			return m_Source.length();

		const auto& match = (*m_Stream)[0];
		return std::distance(m_Source.begin(), match.first);
	}

	std::string RegexStreamReader::ToString(std::string_view separator) const
	{
		std::string result;
		for (auto i = m_Begin; i != m_End; i++)
		{
			result.append(i->str());
			result.append(separator);
		}

		String::RemoveSuffix(result, separator.length());
		return result;
	}

	std::string_view RegexStreamReader::GetToken(size_t offset) const
	{
		SK_CORE_ASSERT(m_Stream != m_End);

		auto position = m_Stream;
		if (!utils::TryAdvance(position, offset))
			return {};

		const auto& match = (*position)[0];
		if (!match.matched)
			return {};

		return { match.first, match.second };
	}

	bool RegexStreamReader::Compare(std::string_view token, size_t offset) const
	{
		return GetToken(offset) == token;
	}

	bool RegexStreamReader::Compare(const std::regex& pattern, size_t offset) const
	{
		std::string_view token = GetToken(offset);
		return std::regex_match(token.begin(), token.end(), pattern);
	}

	bool RegexStreamReader::AdvancePosition(size_t tokenCount)
	{
		for (size_t i = 0; i < tokenCount; i++)
		{
			if (m_Stream == m_End)
				return false;
			++m_Stream;
			++m_Position;
		}
		return true;
	}

}
