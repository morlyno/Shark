#pragma once

#include <string_view>
#include <regex>

namespace Shark::String {

	class StreamPosition
	{
	public:
		StreamPosition() = default;
		StreamPosition(size_t offset)
			: Offset(offset) {
		}

		size_t Offset;

		static constexpr size_t Start = 0;

		auto operator<=>(const StreamPosition& other) const = default;
	};

	class ITokenStreamReader
	{
	public:
		virtual void SetSource(std::string_view source, bool keepPosition) = 0;

		virtual bool IsStreamGood() const = 0;
		virtual bool Advance(size_t tokenCount = 1) = 0;
		virtual bool Read(std::string_view& outToken) = 0;
		virtual std::string_view Read();

		virtual StreamPosition GetStreamPosition() const = 0;
		virtual void SetStreamPosition(StreamPosition position) = 0;
		virtual size_t GetSourcePosition() const = 0;

	public:
		virtual bool Seek(std::string_view token);
		virtual bool Seek(const std::regex& pattern);
		virtual bool SeekPast(std::string_view token);
		virtual bool SeekPast(const std::regex& pattern);

		virtual bool SeekUntil(std::string_view token, std::string_view breakToken);

	public:
		template<typename... TTokenTypes>
			requires (sizeof...(TTokenTypes) > 0) && ((std::convertible_to<TTokenTypes, std::string_view> || std::same_as<std::decay_t<TTokenTypes>, std::regex>) && ...)
		bool Seek(TTokenTypes&&... tokens);

		template<typename... TTokenTypes>
			requires (sizeof...(TTokenTypes) > 0) && ((std::convertible_to<TTokenTypes, std::string_view> || std::same_as<std::decay_t<TTokenTypes>, std::regex>) && ...)
		bool SeekPast(TTokenTypes&&... tokens);

	protected:
		virtual bool Compare(std::string_view token, size_t offset) const = 0;
		virtual bool Compare(const std::regex& pattern, size_t offset) const = 0;
	};

}

template<typename... TTokenTypes>
	requires (sizeof...(TTokenTypes) > 0) && ((std::convertible_to<TTokenTypes, std::string_view> || std::same_as<std::decay_t<TTokenTypes>, std::regex>) && ...)
bool Shark::String::ITokenStreamReader::Seek(TTokenTypes&&... tokens)
{
	size_t offset = 0;
	while (IsStreamGood() && !(Compare(std::forward<TTokenTypes>(tokens), offset++) && ...))
		Advance(std::exchange(offset, 0));

	return IsStreamGood();
}

template<typename... TTokenTypes>
	requires (sizeof...(TTokenTypes) > 0) && ((std::convertible_to<TTokenTypes, std::string_view> || std::same_as<std::decay_t<TTokenTypes>, std::regex>) && ...)
bool Shark::String::ITokenStreamReader::SeekPast(TTokenTypes&&... tokens)
{
	if (Seek(std::forward<TTokenTypes>(tokens)...))
		return Advance(sizeof...(TTokenTypes));
	return false;
}
