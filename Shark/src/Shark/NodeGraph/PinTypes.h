#pragma once

#include "Shark/Core/Base.h"

#include <choc/containers/choc_Value.h>
#include <glm/gtc/type_ptr.hpp>

namespace Shark::TypeUtils {

	template<typename... TRest>
	void addObjectMember(choc::value::Type& classType, std::string_view memberName, choc::value::Type memberType, TRest&&... rest)
	{
		classType.addObjectMember(memberName, std::move(memberType));

		if constexpr (sizeof...(TRest))
			addObjectMember(classType, std::forward<TRest>(rest)...);
	}

	template<typename... TRest>
	choc::value::Type createObject(std::string_view className, TRest&&... rest)
	{
		auto type = choc::value::Type::createObject(className);
		addObjectMember(type, std::forward<TRest>(rest)...);
		return type;
	}

}

namespace Shark::NodeGraph {

	namespace Types {

		struct Flow {};

		struct EntityID
		{
			int64_t ID = 0;

			operator int64_t&() { return ID; }
			operator int64_t() const { return ID; }
		};

		struct AssetHandle
		{
			int64_t Handle = 0;

			operator int64_t& () { return Handle; }
			operator int64_t() const { return Handle; }
		};

	}

	inline choc::value::Type CreateTypeEntityID()                     { return TypeUtils::createObject("EntityID", "ID", choc::value::Type::createInt64()); }
	inline choc::value::Type CreateTypeAssetHandle()                  { return TypeUtils::createObject("AssetHandle", "Handle", choc::value::Type::createInt64()); }
	inline choc::value::Type CreateTypeVec3()                         { return choc::value::Type::createVectorInt32(3); }

	inline choc::value::Value CreateEntityID(int64_t value = 0)       { return choc::value::createObject("EntityID", "ID", value); }
	inline choc::value::Value CreateAssetHandle(int64_t value = 0)    { return choc::value::createObject("AssetHandle", "Handle", value); }
	inline choc::value::Value CreateVec3(const glm::vec3& value = {}) { return choc::value::createVector(glm::value_ptr(value), 3); }

	template<typename T>
	inline choc::value::Type CreateType()
	{
		using TRaw = std::remove_cvref_t<std::remove_pointer_t<std::decay_t<T>>>;

		if      constexpr (std::same_as<TRaw, Types::EntityID>)    return CreateTypeEntityID();
		else if constexpr (std::same_as<TRaw, Types::AssetHandle>) return CreateTypeAssetHandle();
		else if constexpr (std::same_as<TRaw, glm::vec3>)          return CreateTypeVec3();

		else if constexpr (requires { choc::value::Type::createPrimitive<TRaw>(); })
		{
			return choc::value::Type::createPrimitive<TRaw>();
		}

		else
		{
			static_assert(false, "Invalid type T");
		}
	}

}
