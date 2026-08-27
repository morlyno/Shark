#pragma once

#include "Shark/Core/Base.h"

namespace Shark {
	class ProjectConfig;
}

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
