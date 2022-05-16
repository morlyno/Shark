#pragma once

#include "Shark/Core/Base.h"

namespace Shark {

	struct ProjectConfig
	{
		// Name without extention
		std::string Name;
		std::filesystem::path ProjectDirectory;

		std::filesystem::path AssetsDirectory;
		std::filesystem::path StartupScenePath;

		std::string ScriptModulePath;

		// Physics
		glm::vec2 Gravity;
		uint32_t VelocityIterations;
		uint32_t PositionIterations;
		float FixedTimeStep;
	};

	class Project : public RefCount
	{
	public:
		static const std::string& Name();
		static const std::filesystem::path& Directory();

		static const std::filesystem::path& AssetsPath();
		static const std::filesystem::path& StartupScenePath();

		static const std::string& ScriptModulePath();

		static const glm::vec2& Gravity();
		static uint32_t VelocityIterations();
		static uint32_t PositionIterations();
		static float FixedTimeStep();

		static std::filesystem::path RelativeCopy(const std::filesystem::path& filePath);
		static std::filesystem::path AbsolueCopy(const std::filesystem::path& filePath);
		static void Relative(std::filesystem::path& filePath);
		static void Absolue(std::filesystem::path& filePath);

		static Ref<Project> GetActive();
		static void SetActive(Ref<Project> project);

	public:
		ProjectConfig& GetConfig() { return m_Config; }
		const ProjectConfig& GetConfig() const { return m_Config; }

	private:
		ProjectConfig m_Config;

		friend class ProjectSerializer;
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
