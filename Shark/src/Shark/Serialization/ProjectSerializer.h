#pragma once

#include "Shark/Core/Project.h"

namespace Shark {

	class ProjectSerializer
	{
	public:
		ProjectSerializer(Ref<Project> project);
		~ProjectSerializer() = default;

		bool Serialize(const std::filesystem::path& filePath);
		bool Deserialize(const std::filesystem::path& filePath);
	private:
		Ref<Project> m_Project;
	};

}
