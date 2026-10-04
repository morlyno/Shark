#include "skpch.h"
#include "EditorSettings.h"
#include "Shark/File/FileSystem.h"

#include "Shark/Serialization/YAML.h"
#include "Shark/Debug/Profiler.h"

template<>
struct YAML::convert<Shark::RecentProject>
{
	static Node encode(const Shark::RecentProject& recentProject)
	{
		Node node(NodeType::Map);
		node.force_insert("Name", recentProject.Name);
		node.force_insert("Filepath", recentProject.Filepath);
		node.force_insert("LastOpened", std::chrono::system_clock::to_time_t(recentProject.LastOpened));
		return node;
	}

	static bool decode(const Node& node, Shark::RecentProject& recentProject)
	{
		if (!node.IsMap() || node.size() != 3)
			return false;

		recentProject.Name = node["Name"].as<std::string>();
		recentProject.Filepath = node["Filepath"].as<std::filesystem::path>();
		recentProject.LastOpened = std::chrono::system_clock::from_time_t(node["LastOpened"].as<time_t>());
		return true;
	}
};

namespace Shark {

	static std::filesystem::path s_SettingsPath;
	static EditorSettings* s_Instance = nullptr;

	void EditorSettings::Initialize()
	{
		SK_PROFILE_FUNCTION();

		s_SettingsPath = FileSystem::Absolute("Config") / "EditorSettings.yaml";

		SK_CORE_VERIFY(!s_Instance);
		s_Instance = sknew EditorSettings();

		EditorSettingsSerializer::LoadSettings();
	}

	void EditorSettings::Shutdown()
	{
		SK_PROFILE_FUNCTION();

		EditorSettingsSerializer::SaveSettings();

		skdelete s_Instance;
		s_Instance = nullptr;
	}

	EditorSettings& EditorSettings::Get()
	{
		return *s_Instance;
	}

	void EditorSettingsSerializer::LoadSettings()
	{
		SK_PROFILE_FUNCTION();

		// Create default settings if file doesn't exist
		if (!FileSystem::Exists(s_SettingsPath))
		{
			SaveSettings();
			return;
		}

		YAML::Node fileData = YAML::LoadFile(s_SettingsPath);

		if (!fileData["EditorSettings"])
			return;

		auto& settings = EditorSettings::Get();
		YAML::Node rootNode = fileData["EditorSettings"];

		for (auto projectNode : rootNode["RecentProjects"])
		{
			RecentProject recentProject;
			if (!DeserializeProperty(projectNode, recentProject))
				continue;

			settings.RecentProjects[recentProject.LastOpened] = std::move(recentProject);
		}

		if (auto contentBrowserNode = rootNode["ContentBrowser"])
		{
			DeserializeProperty(contentBrowserNode, "GenerateThumbnails", settings.ContentBrowser.GenerateThumbnails, true);
			DeserializeProperty(contentBrowserNode, "ThumbnailSize", settings.ContentBrowser.ThumbnailSize, 120);
		}

		if (auto prefabNode = rootNode["Prefab"])
		{
			DeserializeProperty(prefabNode, "AutoGroupRootEntities", settings.Prefab.AutoGroupRootEntities, true);
		}
	}

	void EditorSettingsSerializer::SaveSettings()
	{
		SK_PROFILE_FUNCTION();

		const auto& settings = EditorSettings::Get();

		YAML::Emitter out;

		out << YAML::BeginMap;
		out << YAML::Key << "EditorSettings";

		out << YAML::BeginMap;
		out << YAML::Key << "RecentProjects" << YAML::Value << std::views::values(settings.RecentProjects);
		out << YAML::Key << "ContentBrowser";
		{
			out << YAML::BeginMap;
			out << YAML::Key << "GenerateThumbnails" << YAML::Value << settings.ContentBrowser.GenerateThumbnails;
			out << YAML::Key << "ThumbnailSize" << YAML::Value << settings.ContentBrowser.ThumbnailSize;
			out << YAML::EndMap;
		}
		out << YAML::Key << "Prefab";
		{
			out << YAML::BeginMap;
			out << YAML::Key << "AutoGroupRootEntities" << settings.Prefab.AutoGroupRootEntities;
			out << YAML::EndMap;
		}
		out << YAML::EndMap;

		out << YAML::EndMap;

		FileSystem::WriteString(s_SettingsPath, out.c_str());
	}

}

