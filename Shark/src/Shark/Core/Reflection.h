#pragma once

namespace Shark::Reflection {

	template<auto... TMembers>
	struct MemberList
	{
		static constexpr size_t MemberCount = sizeof...(TMembers);

		template<typename TFunc>
		static constexpr auto Apply(TFunc&& func)
		{
			func(TMembers...);
		}
	};

	template<typename T, typename Tag>
	struct Type {};

	struct DummyTag {};

#define REFLECT_TYPE_TAGGED(_class, _tag, ...)																														   \
	template<>																																						   \
	struct ::Shark::Reflection::Type<_class, _tag> : ::Shark::Reflection::MemberList<__VA_ARGS__>																					   \
	{																																								   \
	private:																																						   \
		static constexpr std::string_view ClassString = #_class;																									   \
		static constexpr std::string_view MemberString = #__VA_ARGS__;																								   \
																																									   \
	public:																																							   \
		static constexpr std::string_view Namespace = ClassString.substr(0, ClassString.find("::") == std::string_view::npos ? 0 : ClassString.find_last_of(':') - 1); \
		static constexpr std::string_view Class = ClassString.substr(Namespace.empty() ? 0 : Namespace.size() + 2);													   \
																																									   \
		static constexpr std::array<std::string_view, MemberCount> Members = []() -> std::array<std::string_view, MemberCount>										   \
		{																																							   \
			auto members = String::SplitString<MemberCount>(MemberString, ",");																						   \
			for (auto& member : members)																															   \
			{																																						   \
				String::Trim(member, " ");																															   \
				member = member.substr(member.find("::") == std::string_view::npos ? 0 : member.find_last_of(':') + 1);												   \
			}																																						   \
			return members;																																			   \
		}();																																						   \
	}

}

#define REFLECT_TYPE(_class, ...) REFLECT_TYPE_TAGGED(_class, Shark::Reflection::DummyTag, __VA_ARGS__)
