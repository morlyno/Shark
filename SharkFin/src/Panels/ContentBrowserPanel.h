#pragma once

#include "Shark.h"

#include "Panels/Panel.h"

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

	class ContentBrowserPanel : public Panel
	{
	public:
		ContentBrowserPanel();
		~ContentBrowserPanel();

		virtual void OnImGuiRender() override;
		virtual void OnEvent(Event& event) override;

		virtual bool IsShown() const override { return m_ShowPanel; }
		

		void Reload() { m_Reload = true; }
		void OnFileChanged(const std::vector<FileChangedData>& fileEvents) { Reload(); }

		void SetOpenAssetCallback(const std::function<void(AssetHandle)>& callback) { m_OpenAssetCallback = callback; }

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
		std::function<void(AssetHandle)> m_OpenAssetCallback = [](AssetHandle) {};

		DirectoryEntry m_RootDirectory;
		DirectoryEntry* m_CurrentDirectory = nullptr;

		DirectoryEntry* m_SelectedEntry = nullptr;
		bool m_IsSelectedHovered = false;

		float m_CellWidth = 80;
		float m_MinCellHeight = 120;

		Ref<Texture2D> m_FolderIcon;
		Ref<Texture2D> m_FileIcon;
		Ref<Texture2D> m_PNGIcon;
		Ref<Texture2D> m_SceneIcon;
		Ref<Texture2D> m_TextureIcon;

		bool m_Reload = false;
		DirectoryEntry* m_DragDropEntry = nullptr;
		bool m_DragDropActive = false;

		struct Settings
		{
			bool ShowOnlyAssets = true;
		};
		Settings m_Settings;

		bool m_IgnoreSelection = false;

		bool m_ShowPanel = true;

	};

}
