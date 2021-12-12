#pragma once

#include "Shark/Core/UUID.h"
#include "Shark/File/FileSystem.h"

#include <yaml-cpp/yaml.h>
#include <DirectXMath.h>

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
		static bool decode(const Node& node, std::filesystem::path& rhs)
		{
			if (!node.IsScalar())
				return false;
			rhs = Shark::FileSystem::MakeDefaultFormat(node.Scalar());
			return true;
		}
	};

	template<>
	struct convert<Shark::UUID>
	{
		static bool decode(const Node& node, Shark::UUID& rhs)
		{
			return convert<uint64_t>::decode(node, (uint64_t&)rhs);
		}
	};

	Emitter& operator<<(Emitter& out, const DirectX::XMFLOAT2& f2);
	Emitter& operator<<(Emitter& out, const DirectX::XMFLOAT3& f3);
	Emitter& operator<<(Emitter& out, const DirectX::XMFLOAT4& f4);
	Emitter& operator<<(Emitter& out, const std::filesystem::path& filePath);
	Emitter& operator<<(Emitter& out, const Shark::UUID& uuid);

	Node LoadFile(const std::filesystem::path& filename);
	Node LoadFile(const char* filename);

}
