#pragma once

#include "Shark/Core/Base.h"

namespace Shark {

	class ProjectInstance : public RefCount
	{
	public:
		std::string Name;
		std::filesystem::path Directory;

		std::filesystem::path AssetsDirectory;
		std::filesystem::path StartupScenePath;

		std::string ScriptModulePath;

		// Physics
		glm::vec2 Gravity;
		uint32_t VelocityIterations;
		uint32_t PositionIterations;
		float FixedTimeStep;

	public:
		std::filesystem::path GetRelative(const std::filesystem::path& filePath);
		std::filesystem::path GetAbsolue(const std::filesystem::path& filePath);

	};

	class Project
	{
	public:
		static const std::string& GetName();
		static const std::filesystem::path& GetDirectory();
		static const std::filesystem::path& GetAssetsPath();

		static std::filesystem::path RelativeCopy(const std::filesystem::path& filePath);
		static std::filesystem::path AbsolueCopy(const std::filesystem::path& filePath);
		static void Relative(std::filesystem::path& filePath);
		static void Absolue(std::filesystem::path& filePath);

		static Ref<ProjectInstance> GetActive();
		static void SetActive(Ref<ProjectInstance> project);

	};


	class ProjectSerializer
	{
	public:
		ProjectSerializer(Ref<ProjectInstance> project);
		~ProjectSerializer() = default;

		bool Serialize(const std::filesystem::path& filePath);
		bool Deserialize(const std::filesystem::path& filePath);
	private:
		Ref<ProjectInstance> m_Project;
	};

}
