#pragma once

#include "Shark/Core/UUID.h"
#include "Shark/Utils/String.h"

#include <yaml-cpp/yaml.h>
#include <glm/glm.hpp>

namespace YAML {

	template<>
	struct convert<char16_t>
	{
		static bool decode(const Node& node, char16_t& wc)
		{
			wc = node.as<int16_t>();
			return true;
		}
	};

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
			rhs = Shark::String::FormatDefaultCopy(node.Scalar());
			return true;
		}
	};

	template<>
	struct convert<Shark::UUID>
	{
		static Node encode(const Shark::UUID& uuid)
		{
			return Node(uuid.Value());
		}

		static bool decode(const Node& node, Shark::UUID& rhs)
		{
			return convert<uint64_t>::decode(node, (uint64_t&)rhs);
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
		static Node encode(const TEnum& value)
		{
			return Node(magic_enum::enum_name(value));
		}

		static bool decode(const Node& node, TEnum& value)
		{
			if (!node.IsScalar())
				return false;

			std::optional<TEnum> optValue = magic_enum::enum_cast<TEnum>(node.Scalar());
			if (!optValue.has_value())
				return false;

			value = optValue.value();
			return true;
		}
	};

	Emitter& operator<<(Emitter& out, wchar_t);
	Emitter& operator<<(Emitter& out, const glm::vec2& f2);
	Emitter& operator<<(Emitter& out, const glm::vec3& f3);
	Emitter& operator<<(Emitter& out, const glm::vec4& f4);
	Emitter& operator<<(Emitter& out, const std::filesystem::path& filePath);
	Emitter& operator<<(Emitter& out, const Shark::UUID& uuid);

	template<typename TValue>
	Emitter& operator<<(Emitter& out, const TValue& value) { return out << YAML::Node(value); }

	Node LoadFile(const std::filesystem::path& filename);
	Node LoadFile(const char* filename);

}
