#pragma once

#include <Shark.h>
#include <queue>

namespace Shark {

	class AssetsPanel
	{
	private:
		struct DirectoryEntry
		{
			std::filesystem::directory_entry Entry;
			std::filesystem::path Path;
			std::string PathString;
			std::string PathExtenstion;
			std::string FileName;
			std::string FileNameShort;
			DirectoryEntry(const std::filesystem::directory_entry& entry)
			{
				Entry = entry;
				Path = entry.path();
				PathString = Path.string();
				FileName = Path.filename().string();
				FileNameShort = Path.filename().replace_extension().string();
				PathExtenstion = Path.extension().string();
			}
		};
	public:
		AssetsPanel();
		~AssetsPanel();

		void OnImGuiRender();

	private:
		void OnDirectoryClicked(const std::filesystem::path& path);
		void OnFileClicked(const std::filesystem::path& path);

		void DrawDirectory(const std::filesystem::path& directory);
		void DrawNavigationButtons();
		void DrawCurrentPath();
		void DrawDirectoryContent();
		void DrawFilterInput();
		void DrawAssetsFiltered();
		bool CheckFileOnFilter(const std::string& str);
		void ResetFilter();
		void DrawDirectoryEntry(RenderID imageID, const DirectoryEntry& entry);

	private:
		std::filesystem::path m_CurrentPath;
		std::vector<std::filesystem::path> m_DirectoryHistory;
		uint32_t m_DirHistoryIndex = 0;

		std::string m_FilterBuffer;
		std::string m_FilterAsLower;
		bool m_ShowFiltered = false;

		Ref<Texture2D> m_FileImage = nullptr;
		Ref<Texture2D> m_DirectoryImage = nullptr;

		const ImVec2 m_ImageSize = { 64, 64 };
		bool m_WantResetFilter = false;

	};

}
