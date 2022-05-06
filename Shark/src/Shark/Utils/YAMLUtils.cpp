#include "skpch.h"
#include "YAMLUtils.h"

#include "Shark/Scene/SceneCamera.h"

namespace YAML {

	YAML::Emitter& operator<<(Emitter& out, const glm::vec2& f2)
	{
		out << Flow;
		out << BeginSeq << f2.x << f2.y << EndSeq;
		return out;
	}

	YAML::Emitter& operator<<(Emitter& out, const glm::vec3& f3)
	{
		out << Flow;
		out << BeginSeq << f3.x << f3.y << f3.z << EndSeq;
		return out;
	}

	YAML::Emitter& operator<<(Emitter& out, const glm::vec4& f4)
	{
		out << Flow;
		out << BeginSeq << f4.x << f4.y << f4.z << f4.w << EndSeq;
		return out;
	}

	YAML::Emitter& operator<<(Emitter& out, const std::filesystem::path& filePath)
	{
		return out << Shark::String::FormatDefaultCopy(filePath).string();
	}

	YAML::Emitter& operator<<(Emitter& out, const Shark::UUID& uuid)
	{
		return out << (uint64_t)uuid;
	}

	Node LoadFile(const std::filesystem::path& filename) {
		std::ifstream fin(filename);
		if (!fin) {
			throw BadFile(filename.string());
		}
		return Load(fin);
	}

	Node LoadFile(const char* filename) {
		std::ifstream fin(filename);
		if (!fin) {
			throw BadFile(filename);
		}
		return Load(fin);
	}

}
