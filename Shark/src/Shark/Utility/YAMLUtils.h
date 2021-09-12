#pragma once

#include <yaml-cpp/yaml.h>
#include "Shark/Utility/Utility.h"

namespace Shark {
	class SceneCamera;
	enum class Geometry;
	enum class BodyType;
	enum class ShapeType;
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
			static_assert(sizeof(Shark::Geometry) <= sizeof(int));
			return Node((int)val);
		}

		static bool decode(const Node& node, Shark::Geometry& val)
		{
			static_assert(sizeof(Shark::Geometry) <= sizeof(int));
			val = (Shark::Geometry)node.as<int>();
			return true;
		}
	};

	template<>
	struct convert<Shark::BodyType>
	{
		static Node encode(const Shark::BodyType& val)
		{
			static_assert(sizeof(Shark::BodyType) <= sizeof(int));
			return Node((int)val);
		}

		static bool decode(const Node& node, Shark::BodyType& val)
		{
			static_assert(sizeof(Shark::BodyType) <= sizeof(int));
			val = (Shark::BodyType)node.as<int>();
			return true;
		}
	};

	template<>
	struct convert<Shark::ShapeType>
	{
		static Node encode(const Shark::ShapeType& val)
		{
			static_assert(sizeof(Shark::ShapeType) <= sizeof(int));
			return Node((int)val);
		}

		static bool decode(const Node& node, Shark::ShapeType& val)
		{
			static_assert(sizeof(Shark::ShapeType) <= sizeof(int));
			val = (Shark::ShapeType)node.as<int>();
			return true;
		}
	};

	template<typename T>
	Emitter& operator<<(Emitter& emitter, const T& v)
	{
		return emitter.Write(convert<T>::encode(v).as<std::string>());
	}

	Emitter& operator<<(Emitter& out, const DirectX::XMFLOAT2& f2);

	Emitter& operator<<(Emitter& out, const DirectX::XMFLOAT3& f3);

	Emitter& operator<<(Emitter& out, const DirectX::XMFLOAT4& f4);


	Node LoadFile(const std::filesystem::path& filename);
	Node LoadFile(const char* filename);

}

#if SK_YAMLUTILS_ALL

namespace Shark {
	enum class SceneCamera::Projection;
}

namespace YAML {

	template<>
	struct convert<Shark::SceneCamera::Projection>
	{
		static Node encode(const Shark::SceneCamera::Projection& val)
		{
			static_assert(sizeof(Shark::SceneCamera::Projection) <= sizeof(int));
			return Node((int)val);
		}

		static bool decode(const Node& node, Shark::SceneCamera::Projection& val)
		{
			static_assert(sizeof(Shark::SceneCamera::Projection) <= sizeof(int));
			val = (Shark::SceneCamera::Projection)node.as<int>();
			return true;
		}
	};

}

#endif
