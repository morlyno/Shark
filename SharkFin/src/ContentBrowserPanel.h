#pragma once

#include <Shark.h>

#include <imgui.h>

namespace Shark {

	enum class EntryType
	{
		None,
		File,
		Directory,
	};

	struct DirectoryEntry
	{
		EntryType Type;
		AssetHandle Handle;
		std::filesystem::path Path;
		std::string DisplayName;

		// TODO(moro): Add Parent
		std::vector<DirectoryEntry> ChildEntrys;
	};

	class ContentBrowserPanel
	{
	public:
		ContentBrowserPanel();
		~ContentBrowserPanel();

		void OnImGuiRender(bool& showPanel);

		void Reload() { m_Reload = true; }
		void OnProjectChanged() { Reload(); }
		void OnFileChanged(const std::vector<FileChangedData>& fileEvents) { Reload(); }

	private:
		void DrawTreeView();
		void DrawTreeView(DirectoryEntry& entry);
		void DrawCellView();
		void DrawCell(DirectoryEntry& entry);
		void DrawMenuBar();

		void CacheDirectory(const std::filesystem::path& rootPath);
		void CacheDirectory(DirectoryEntry& directory, const std::filesystem::path& directoryPath);

		DirectoryEntry* GetEntry(const std::filesystem::path& path);
		DirectoryEntry* GetChildEntry(DirectoryEntry& directory, const std::filesystem::path& path);

		Ref<Texture2D> GetCellIcon(const DirectoryEntry& entry);
		void ImportEntry(DirectoryEntry& entry);

	private:
		DirectoryEntry m_RootDirectory;
		DirectoryEntry* m_CurrentDirectory = nullptr;

		DirectoryEntry* m_SelectedEntry = nullptr;
		bool m_IsSelectedHovered = false;

		float m_CellWidth = 80;
		float m_MinCellHeight = 120;

		Ref<Texture2D> m_FolderIcon;
		Ref<Texture2D> m_FileIcon;

		bool m_Reload = false;
		DirectoryEntry* m_DragDropEntry = nullptr;
		bool m_DragDropActive = false;

		struct Settings
		{
			bool ShowOnlyAssets = true;
		};
		Settings m_Settings;

		bool m_IgnoreSelection = false;

	};

}
