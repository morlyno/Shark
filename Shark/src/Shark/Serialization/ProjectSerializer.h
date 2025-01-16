#pragma once

#include "Shark/Core/Project.h"

namespace Shark {

	class ProjectSerializer
	{
	public:
		ProjectSerializer(Ref<ProjectConfig> projectConfig);
		~ProjectSerializer() = default;

		bool Serialize(const std::filesystem::path& filePath);
		bool Deserialize(const std::filesystem::path& filePath);
	private:
		Ref<ProjectConfig> m_ProjectConfig;
	};

}
