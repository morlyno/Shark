#pragma once

#include "Shark/Core/Base.h"

#include <choc/containers/choc_Value.h>
#include <glm/gtc/type_ptr.hpp>

namespace Shark::NodeGraph {

	namespace Types {

		struct EntityID
		{
			int64_t ID = 0;

			operator int64_t&() { return ID; }
			operator int64_t() const { return ID; }
		};

		struct Flow {};

	}

	template<typename T>
	inline choc::value::Type AsType();

	template<typename T>
		requires std::is_pointer_v<T> || std::is_const_v<T>
	inline choc::value::Type AsType()
	{
		return AsType<std::remove_const_t<std::remove_pointer_t<T>>>();
	}

	template<typename T>
	inline choc::value::Type AsType()
	{
		return choc::value::Type::createPrimitive<T>();
	}

	template<>
	inline choc::value::Type AsType<Types::EntityID>()
	{
		auto type = choc::value::Type::createObject("EntityID");
		type.addObjectMember("ID", choc::value::Type::createInt64());
		return type;
	}

	template<>
	inline choc::value::Type AsType<glm::vec3>()
	{
		return choc::value::Type::createArray(choc::value::Type::createFloat32(), 3);
	}

	template<typename T>
	inline choc::value::Type AsTypeFromValue(const T& value)
	{
		return AsType<T>();
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	//// As Value //////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////

	template<typename T>
	inline choc::value::Value AsValue(const T& val);

	template<typename T>
		requires (requires(T val) { choc::value::createPrimitive(val); })
	inline choc::value::Value AsValue(const T& val)
	{
		return choc::value::createPrimitive(val);
	}

	template<>
	inline choc::value::Value AsValue(const Types::EntityID& id)
	{
		return choc::value::createObject("EntityID", "ID", id.ID);
	}

	template<>
	inline choc::value::Value AsValue(const glm::vec3& v)
	{
		return choc::value::createVector(glm::value_ptr(v), 3);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	//// From Value ////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////

	template<typename T>
	inline T FromValue(choc::value::ValueView value);

	template<typename T>
		requires (requires(T val) { choc::value::createPrimitive(val); })
	inline T FromValue(choc::value::ValueView value)
	{
		return value.get<T>();
	}

	template<>
	inline glm::vec3 FromValue<glm::vec3>(choc::value::ValueView value)
	{
		return { value[0].getFloat32(), value[1].getFloat32(), value[2].getFloat32() };
	}

}
