#pragma once

#include <yaml-cpp/yaml.h>
#include "Shark/Utility/Utility.h"

namespace Shark {
	enum class Geometry;
}

namespace YAML {

	template<>
	struct convert<DirectX::XMFLOAT2>
	{
		static bool decode(const Node& node, DirectX::XMFLOAT2& f2)
		{
			if (!node.IsSequence() || node.size() != 2)
				return false;

			f2.x = node[0].as<float>();
			f2.y = node[1].as<float>();

			return true;
		}
	};

	template<>
	struct convert<DirectX::XMFLOAT3>
	{
		static bool decode(const Node& node, DirectX::XMFLOAT3& f3)
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
	struct convert<DirectX::XMFLOAT4>
	{
		static bool decode(const Node& node, DirectX::XMFLOAT4& f4)
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

	template<>
	struct convert<Shark::Geometry>
	{
		static Node encode(const Shark::Geometry& val)
		{
			return Node((Shark::Utility::IntTypeFromSize<sizeof(Shark::Geometry)>::Signed)val);
		}

		static bool decode(const Node& node, Shark::Geometry& val)
		{
			val = (Shark::Geometry)node.as<Shark::Utility::IntTypeFromSize<sizeof(Shark::Geometry)>::Signed>();
			return true;
		}
	};

	template<typename T>
	Emitter& operator<<(Emitter& emitter, const T& v)
	{
		return emitter.Write(convert<T>::encode(v).as<std::string>());
	}

	inline Emitter& operator<<(Emitter& out, const DirectX::XMFLOAT2& f2)
	{
		out << Flow;
		out << BeginSeq << f2.x << f2.y << EndSeq;
		return out;
	}

	inline Emitter& operator<<(Emitter& out, const DirectX::XMFLOAT3& f3)
	{
		out << Flow;
		out << BeginSeq << f3.x << f3.y << f3.z << EndSeq;
		return out;
	}

	inline Emitter& operator<<(Emitter& out, const DirectX::XMFLOAT4& f4)
	{
		out << Flow;
		out << BeginSeq << f4.x << f4.y << f4.z << f4.w << EndSeq;
		return out;
	}

}
