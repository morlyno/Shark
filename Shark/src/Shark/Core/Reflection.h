#pragma once

#include "Shark/Utils/String.h"

namespace Shark {
	template<typename T, typename Tag>
	struct Reflection_Type {};
}

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

		template<typename TFunc>
		static constexpr auto ForEachTypeIndexed(TFunc&& func)
		{
			size_t index = 0;
			(func.template operator()<decltype(TMembers)>(index++), ...);
		}

	};

	template<typename T, typename Tag>
	using Type = Reflection_Type<T, Tag>;

	struct DummyTag {};

}

// 
// REFLECT_TYPE_TAGGER
// 
// _class -> Type with full namespace
// _tag   -> Tag type, this allowes _class to be defined mutiple times with different tags (for example: one with Full type information and one with Serialization members)
// ...    -> Members of the class
// 
// Members:
//  Namespace            -> std::string_view
//  Class                -> std::string_view
//  Members              -> std::array<std::string_view, MemberCount>
//  
// Inherited (MemberList):
//  MemberCount          -> size_t
//  Tuple                -> std::tuple<TMembers...>
//  Type<Index>          => Member type at Index
//  
//  Apply()              => calls a function with TMembers...
//  ForEachTypeIndexed() => calls a function for eacht member with the member type as a template paramter and the index as a function parameter
// 
// 
// For reference see node graph (e.g. ProcessNode.h, MathNodes.h)
// 

#define REFLECTION_PROXY(_proxy_class) \
	template<typename T, typename Tag> \
	struct _proxy_class

#define REFLECTION_PROXY_CONNECT(_proxy_class)						\
	template<typename T, typename Tag>								\
		requires requires { _proxy_class<T, Tag>(); }				\
	struct ::Shark::Reflection_Type<T, Tag> : _proxy_class<T, Tag>	\
	{};


#define REFLECT_TYPE_TAGGED_PROXY(_proxy_class, _class, _tag, ...)																									   \
	template<>																																						   \
	struct _proxy_class<_class, _tag> : ::Shark::Reflection::MemberList<__VA_ARGS__>																				   \
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
			auto members = ::Shark::String::SplitString<MemberCount>(MemberString, ",");																			   \
			for (auto& member : members)																															   \
			{																																						   \
				::Shark::String::Trim(member, " ");																													   \
				member = member.substr(member.find("::") == std::string_view::npos ? 0 : member.find_last_of(':') + 1);												   \
			}																																						   \
			return members;																																			   \
		}();																																						   \
	}

#define REFLECT_TYPE_TAGGED(_class, _tag, ...)																														   \
	template<>																																						   \
	struct ::Shark::Reflection_Type<::_class, _tag> : ::Shark::Reflection::MemberList<__VA_ARGS__>																	   \
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
			auto members = ::Shark::String::SplitString<MemberCount>(MemberString, ",");																			   \
			for (auto& member : members)																															   \
			{																																						   \
				::Shark::String::Trim(member, " ");																													   \
				member = member.substr(member.find("::") == std::string_view::npos ? 0 : member.find_last_of(':') + 1);												   \
			}																																						   \
			return members;																																			   \
		}();																																						   \
	}

#define REFLECT_TYPE(_class, ...) REFLECT_TYPE_TAGGED(_class, Shark::Reflection::DummyTag, __VA_ARGS__)
