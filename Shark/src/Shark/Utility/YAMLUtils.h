#pragma once

#include <yaml-cpp/yaml.h>

namespace YAML {

	template<>
	struct convert<std::filesystem::path>
	{
		static Node encode(const std::filesystem::path& rhs)
		{
			return Node(rhs.string());
		}

		static bool decode(const Node& node, std::filesystem::path& rhs)
		{
			if (!node.IsScalar())
				return false;
			rhs = node.Scalar();
			return true;
		}
	};

	template<typename T>
	Emitter& operator<<(Emitter& emitter, const T& v)
	{
		return emitter.Write(convert<T>::encode(v).as<std::string>());
	}

}
