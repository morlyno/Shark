#pragma once

#include "Shark/Core/UUID.h"
#include "Shark/Core/TimeStep.h"

#include <glm/glm.hpp>
#include <yaml-cpp/yaml.h>

namespace YAML {

	template<>
	struct convert<glm::vec2>
	{
		static Node encode(const glm::vec2& val)
		{
			Node node(NodeType::Sequence);
			node.push_back(val.x);
			node.push_back(val.y);
			return node;
		}

		static bool decode(const Node& node, glm::vec2& f2)
		{
			if (!node.IsSequence() || node.size() != 2)
				return false;

			f2.x = node[0].as<float>();
			f2.y = node[1].as<float>();

			return true;
		}
	};

	template<>
	struct convert<glm::vec3>
	{
		static Node encode(const glm::vec3& val)
		{
			Node node(NodeType::Sequence);
			node.push_back(val.x);
			node.push_back(val.y);
			node.push_back(val.z);
			return node;
		}

		static bool decode(const Node& node, glm::vec3& f3)
		{
			if (!node.IsSequence() || node.size() != 3)
				return false;

			f3.x = node[0].as<float>();
			f3.y = node[1].as<float>();
			f3.z = node[2].as<float>();

			return true;
		}
	};

	template<>
	struct convert<glm::vec4>
	{
		static Node encode(const glm::vec4& val)
		{
			Node node(NodeType::Sequence);
			node.push_back(val.x);
			node.push_back(val.y);
			node.push_back(val.z);
			node.push_back(val.w);
			return node;
		}

		static bool decode(const Node& node, glm::vec4& f4)
		{
			if (!node.IsSequence() || node.size() != 4)
				return false;

			f4.x = node[0].as<float>();
			f4.y = node[1].as<float>();
			f4.z = node[2].as<float>();
			f4.w = node[3].as<float>();

			return true;
		}
	};

	template<typename T>
	struct convert<std::optional<T>>
	{
		static Node encode(const std::optional<T>& val)
		{
			return val ? Node(*val) : Node(YAML::Null);
		}

		static bool decode(const Node& node, std::optional<T>& outVal)
		{
			outVal = node.IsNull() ? std::nullopt : std::optional(node.as<T>());
			return true;
		}
	};

	template<>
	struct convert<std::filesystem::path>
	{
		static Node encode(const std::filesystem::path& path)
		{
			return Node(path.generic_string());
		}

		static bool decode(const Node& node, std::filesystem::path& rhs)
		{
			if (!node.IsScalar())
				return false;
			rhs = node.Scalar();
			return true;
		}
	};

	template<typename TKey, typename TPred, typename TAlloc>
	struct convert<std::set<TKey, TPred, TAlloc>>
	{
		static Node encode(const std::set<TKey, TPred, TAlloc>& set)
		{
			Node node(NodeType::Sequence);
			for (const auto& element : set)
				node.push_back(element);
			return node;
		}

		static bool decode(const Node& node, std::set<TKey, TPred, TAlloc>& outSet)
		{
			if (!node.IsSequence())
				return false;

			outSet.clear();
			for (const auto& element : node)
				outSet.push_back(element.as<TKey>());
			return true;
		}
	};

	template<typename TArg0, typename TArg1, typename TArg2>
	struct convert<std::tuple<TArg0, TArg1, TArg2>>
	{
		static Node encode(const std::tuple<TArg0, TArg1, TArg2>& tuple)
		{
			Node node(NodeType::Sequence);
			node.push_back(std::get<0>(tuple));
			node.push_back(std::get<1>(tuple));
			node.push_back(std::get<2>(tuple));
			return node;
		}

		static bool decode(const Node& node, std::tuple<TArg0, TArg1, TArg2>& tuple)
		{
			if (!node.IsSequence() || node.size() != 3)
				return false;

			std::get<0>(tuple) = node[0].as<TArg0>();
			std::get<1>(tuple) = node[1].as<TArg1>();
			std::get<2>(tuple) = node[2].as<TArg2>();
			return true;
		}
	};

	template<typename TEnum>
		requires std::is_enum_v<TEnum>
	struct convert<TEnum>
	{
		static_assert(magic_enum::detail::supported<std::decay_t<TEnum>>::value);
		static Node encode(const TEnum& value)
		{
			using D = std::decay_t<TEnum>;
			return Node(fmt::to_string(value));
		}

		static bool decode(const Node& node, TEnum& value)
		{
			if (!node.IsScalar())
				return false;

			using D = std::decay_t<TEnum>;
			if constexpr (magic_enum::detail::subtype_v<D> == magic_enum::detail::enum_subtype::flags)
			{
				if (auto optValue = magic_enum::enum_flags_cast<TEnum>(node.Scalar()))
				{
					value = optValue.value();
					return true;
				}

				std::underlying_type_t<TEnum> intValue;
				if (convert<std::underlying_type_t<TEnum>>::decode(node, intValue))
				{
					if (auto optValue = magic_enum::enum_cast<TEnum, magic_enum::detail::enum_subtype::common>(intValue))
					{
						value = optValue.value();
						return true;
					}
				}

				return false;
			}

			if (auto optValue = magic_enum::enum_cast<TEnum>(node.Scalar()))
			{
				value = optValue.value();
				return true;
			}

			return false;
		}
	};

	template<>
	struct convert<std::chrono::system_clock::time_point>
	{
		static Node encode(const std::chrono::system_clock::time_point& timepoint)
		{
			return Node(std::chrono::system_clock::to_time_t(timepoint));
		}

		static bool decode(const Node& node, std::chrono::system_clock::time_point& timepoint)
		{
			timepoint = std::chrono::system_clock::from_time_t(node.as<time_t>());
			return true;
		}
	};

#define DECALRE_YAML_CONVERT_AS(_Type, _BaseType)\
template<>                                                           \
struct convert<_Type>                                                \
{                                                                    \
	using Type = _Type;                                              \
	using BaseType = _BaseType;                                      \
	static Node encode(const Type& value)                            \
	{                                                                \
		return convert<BaseType>::encode(value);                     \
	}																 \
	static bool decode(const Node& node, Type& outValue)			 \
	{																 \
		BaseType temp;												 \
		bool result = convert<BaseType>::decode(node, temp);		 \
		outValue = temp;											 \
		return result;												 \
	}                                                                \
}

	DECALRE_YAML_CONVERT_AS(Shark::UUID, uint64_t);
	DECALRE_YAML_CONVERT_AS(Shark::TimeStep, float);
#undef DECALRE_YAML_CONVERT_AS

	template<typename TValue>
	Emitter& operator<<(Emitter& out, const TValue& value) { return out << YAML::Node(value); }

	Node LoadFile(const std::filesystem::path& filename);
	Node LoadFile(const char* filename);

	template<typename TValue>
	bool DeserializeProperty(const YAML::Node& node, std::string_view name, TValue& outValue)
	{
		try
		{
			outValue = node[name].as<std::decay_t<TValue>>();
		}
		catch (const YAML::BadConversion& exception)
		{
			SK_CORE_ERROR_TAG("Serialization", "Failed to deserialize property '{}'!\n\tError: {}", name, exception.what());
			return false;
		}
		catch (const YAML::InvalidNode& exception)
		{
			SK_CORE_ERROR_TAG("Serialization", "Failed to deserialize property '{}'!\n\tError: {}", name, exception.what());
			return false;
		}
		return true;
	}

	template<typename TValue, typename TDefault>
	bool DeserializeProperty(const YAML::Node& node, std::string_view name, TValue& outValue, TDefault&& defaultArg)
	{
		try
		{
			outValue = node[name].as<std::decay_t<TValue>>();
		}
		catch (const YAML::BadConversion& exception)
		{
			SK_CORE_ERROR_TAG("Serialization", "Failed to deserialize property '{}'!\n\tError: {}", name, exception.what());
			outValue = std::forward<TDefault>(defaultArg);
			return false;
		}
		catch (const YAML::InvalidNode& exception)
		{
			SK_CORE_ERROR_TAG("Serialization", "Failed to deserialize property '{}'!\n\tError: {}", name, exception.what());
			outValue = std::forward<TDefault>(defaultArg);
			return false;
		}
		return true;
	}

	template<typename T>
	void Read(const YAML::Node& node, std::string_view key, T& outResult)
	{
		try
		{
			outResult = node[key].as<T>();
		}
		catch (const YAML::BadConversion& execption)
		{
			SK_CORE_ERROR_TAG("Serialization", "Conversion failed '{}'\n\t{}", key, execption.what());
			throw;
		}
		catch (const YAML::InvalidNode& exception)
		{
			SK_CORE_ERROR_TAG("Serialization", "Invalid node '{}'\n\t{}", key, exception.what());
			throw;
		}
	}

}

#include "Shark/Serialization/SerializationMacros.h"
