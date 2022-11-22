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
	struct convert<std::string_view>
	{
		static Node encode(std::string_view str)
		{
			return Node(std::string(str));
		}

		static bool decode(const Node& node, std::string_view& str)
		{
			if (node.IsScalar())
			{
				str = node.Scalar();
				return true;
			}
			return false;
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
			return Node((uint64_t)uuid);
		}

		static bool decode(const Node& node, Shark::UUID& rhs)
		{
			return convert<uint64_t>::decode(node, (uint64_t&)rhs);
		}
	};

	Emitter& operator<<(Emitter& out, wchar_t);
	Emitter& operator<<(Emitter& out, const glm::vec2& f2);
	Emitter& operator<<(Emitter& out, const glm::vec3& f3);
	Emitter& operator<<(Emitter& out, const glm::vec4& f4);
	Emitter& operator<<(Emitter& out, const std::filesystem::path& filePath);
	Emitter& operator<<(Emitter& out, const Shark::UUID& uuid);

	Node LoadFile(const std::filesystem::path& filename);
	Node LoadFile(const char* filename);

}
