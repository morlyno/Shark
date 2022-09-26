#pragma once

#include "Shark/Core/Base.h"

namespace Shark {

	//struct ProjectConfig
	//{
	//	// Name without extention
	//	std::string Name;
	//	std::filesystem::path ProjectDirectory;
	//
	//	std::filesystem::path AssetsDirectory;
	//	std::filesystem::path StartupScenePath;
	//
	//	std::string ScriptModulePath;
	//
	//	// Physics
	//	glm::vec2 Gravity;
	//	uint32_t VelocityIterations;
	//	uint32_t PositionIterations;
	//	float FixedTimeStep;
	//};

	class Project : public RefCount
	{
	public:
		static const std::string& GetName();
		static const std::filesystem::path& GetDirectory();
		static const std::filesystem::path& GetAssetsPath();

		static std::filesystem::path RelativeCopy(const std::filesystem::path& filePath);
		static std::filesystem::path AbsolueCopy(const std::filesystem::path& filePath);
		static void Relative(std::filesystem::path& filePath);
		static void Absolue(std::filesystem::path& filePath);

		static Ref<Project> GetActive();
		static void SetActive(Ref<Project> project);

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
	};


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
