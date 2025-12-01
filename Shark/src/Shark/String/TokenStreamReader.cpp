#include "skpch.h"
#include "TokenStreamReader.h"

namespace Shark {

	std::string_view String::ITokenStreamReader::Read()
	{
		std::string_view token;
		if (Read(token))
			return token;
		return {};
	}

	bool String::ITokenStreamReader::Seek(std::string_view token)
	{
		while (IsStreamGood() && !Compare(token, 0))
			Advance();
		return IsStreamGood();
	}

	bool String::ITokenStreamReader::Seek(const std::regex& pattern)
	{
		while (IsStreamGood() && !Compare(pattern, 0))
			Advance();
		return IsStreamGood();
	}

	bool String::ITokenStreamReader::SeekPast(std::string_view token)
	{
		if (Seek(token))
			return Advance();
		return false;
	}

	bool String::ITokenStreamReader::SeekPast(const std::regex& pattern)
	{
		if (Seek(pattern))
			return Advance();
		return false;
	}

	bool String::ITokenStreamReader::SeekUntil(std::string_view token, std::string_view breakToken)
	{
		while (IsStreamGood())
		{
			if (Compare(token, 0))
				return true;

			if (Compare(breakToken, 0))
				return false;
			Advance();
		}

		return false;
	}

}
