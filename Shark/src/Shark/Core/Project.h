#pragma once

#include "Shark/Core/Base.h"

namespace Shark {

	class ProjectInstance : public RefCount
	{
	public:
		std::string Name;                       // Untitled
		std::filesystem::path Directory;        // Empty

		std::filesystem::path AssetsDirectory;  // Assets
		std::filesystem::path StartupScenePath; // Empty

		std::string ScriptModulePath;           // Binaries/{Name}.dll

		// Physics
		glm::vec2 Gravity;                      // 0.0f, -9.81f
		uint32_t VelocityIterations;            // 8
		uint32_t PositionIterations;            // 3
		float FixedTimeStep;                    // 0.001f

	public:
		std::filesystem::path GetRelative(const std::filesystem::path& filePath);
		std::filesystem::path GetAbsolue(const std::filesystem::path& filePath);

		std::filesystem::path GetProjectFilePath() const;

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

		static Ref<ProjectInstance> Create(const std::filesystem::path& directory, const std::string& name);

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
