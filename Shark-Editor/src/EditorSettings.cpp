#include "EditorSettings.h"
#include "Shark/File/FileSystem.h"

#include "Shark/Serialization/YAML.h"

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
		s_SettingsPath = FileSystem::Absolute("Config") / "EditorSettings.yaml";

		SK_CORE_VERIFY(!s_Instance);
		s_Instance = sknew EditorSettings();

		EditorSettingsSerializer::LoadSettings();
	}

	void EditorSettings::Shutdown()
	{
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
		YAML::Node group = rootNode;

		group = rootNode["RecentProjects"];
		for (auto recentProjectNode : group)
		{
			RecentProject recentProject;
			SK_DESERIALIZE_PROPERTY(group, "Name", recentProject.Name);
			SK_DESERIALIZE_PROPERTY(group, "Filepath", recentProject.Filepath);
			SK_DESERIALIZE_PROPERTY(group, "LastOpened", recentProject.LastOpened);

			if (!FileSystem::Exists(recentProject.Filepath))
				continue;

			settings.RecentProjects[recentProject.LastOpened] = std::move(recentProject);
		}

		group = rootNode["ContentBrowser"];
		SK_DESERIALIZE_PROPERTY(group, "ThumbnailSize", settings.ContentBrowser.ThumbnailSize);
		SK_DESERIALIZE_PROPERTY(group, "GenerateThubmnails", settings.ContentBrowser.GenerateThumbnails);

		group = rootNode["Prefab"];
		SK_DESERIALIZE_PROPERTY(group, "AutoGroupRootEntities", settings.Prefab.AutoGroupRootEntities);
	}

	void EditorSettingsSerializer::SaveSettings()
	{
		const auto& settings = EditorSettings::Get();

		YAML::Emitter out;
		out << YAML::BeginMap;
		SK_BEGIN_GROUP(out, "EditorSettings");
		{
			SK_BEGIN_GROUP(out, "RecentProjects");
			for (const auto& [lastOpened, recentProject] : settings.RecentProjects)
			{
				if (!FileSystem::Exists(recentProject.Filepath))
					continue;

				SK_SERIALIZE_PROPERTY(out, "Name", recentProject.Name);
				SK_SERIALIZE_PROPERTY(out, "Filepath", recentProject.Filepath);
				SK_SERIALIZE_PROPERTY(out, "LastOpened", recentProject.LastOpened);
			}
			SK_END_GROUP(out);

			SK_SERIALIZE_PROPERTY(out, "RecentProjects", settings.RecentProjects);

			SK_BEGIN_GROUP(out, "ContentBrowser");
			SK_SERIALIZE_PROPERTY(out, "ThumbnailSize", settings.ContentBrowser.ThumbnailSize);
			SK_SERIALIZE_PROPERTY(out, "GenerateThubmnails", settings.ContentBrowser.GenerateThumbnails);
			SK_END_GROUP(out);

			SK_BEGIN_GROUP(out, "Prefab");
			SK_SERIALIZE_PROPERTY(out, "AutoGroupRootEntities", settings.Prefab.AutoGroupRootEntities);
			SK_END_GROUP(out);
		}
		SK_END_GROUP(out);
		out << YAML::EndMap;

		FileSystem::WriteString(s_SettingsPath, out.c_str());
	}

}

