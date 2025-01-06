#pragma once

#include "Shark/Core/Base.h"

namespace Shark {

	struct RecentProject
	{
		std::string Name;
		std::filesystem::path Filepath;
		std::chrono::system_clock::time_point LastOpened;
	};

	class EditorSettings
	{
	public:
		static void Initialize();
		static void Shutdown();

		static EditorSettings& Get();

	public:
		struct ContentBrowserSettings
		{
			float ThumbnailSize = 120.0f;
			bool GenerateThumbnails = true;
		};

		struct PrefabSettings
		{
			bool AutoGroupRootEntities = true;
		};

	public:
		std::map<std::chrono::system_clock::time_point, RecentProject> RecentProjects;

		ContentBrowserSettings ContentBrowser;
		PrefabSettings Prefab;
	};

	class EditorSettingsSerializer
	{
	public:
		static void LoadSettings();
		static void SaveSettings();
	};

}
