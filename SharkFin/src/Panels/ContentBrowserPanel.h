#pragma once

#include "Shark.h"

#include "Shark/Editor/Panel.h"

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

		virtual void OnImGuiRender(bool& shown) override;
		virtual void OnEvent(Event& event) override;

		void Reload() { m_Reload = true; }
		void OnFileChanged(const std::vector<FileChangedData>& fileEvents);

		template<typename Func>
		void SetOpenFileCallback(const Func& callback) { m_OpenFileCallback = callback; }

	private:
		void DrawTreeView();
		void DrawTreeView(DirectoryEntry& entry);
		void DrawCellView();
		void DrawCell(DirectoryEntry& entry);
		void DrawCreateEntryCell();
		void DrawMenuBar();
		void DrawSettingsBar();

		void DrawPopups();
		void DrawDragDropTooltip();

		void CacheDirectory(const std::filesystem::path& rootPath);
		void CacheDirectory(DirectoryEntry& directory, const std::filesystem::path& directoryPath);

		DirectoryEntry* GetEntry(const std::filesystem::path& path);
		DirectoryEntry* GetChildEntry(DirectoryEntry& directory, const std::filesystem::path& path);
		DirectoryEntry* GetParentEntry(const DirectoryEntry& entry);

		Ref<Texture2D> GetCellIcon(const DirectoryEntry& entry);
		void ImportEntry(DirectoryEntry& entry);

		bool ShouldItemBeIgnored(const std::filesystem::path& filePath);

		void OnRenameFinished(bool canChangeExtension);
		void ClearRenameContext();

		void DeleteSelectedEntry();

		void OnCreateEntryFinished();

	private:
		std::function<void(const std::filesystem::path&)> m_OpenFileCallback = [](...) {};

		DirectoryEntry m_RootDirectory;
		DirectoryEntry* m_CurrentDirectory = nullptr;

		DirectoryEntry* m_SelectedEntry = nullptr;
		bool m_IsSelectedHovered = false;

		float m_CellWidth = 80;
		float m_CellHeight = 118;

		Ref<Texture2D> m_FolderIcon;
		Ref<Texture2D> m_FileIcon;
		Ref<Texture2D> m_PNGIcon;
		Ref<Texture2D> m_SceneIcon;
		Ref<Texture2D> m_TextureIcon;
		Ref<Texture2D> m_ScriptIcon;

		bool m_Reload = false;
		DirectoryEntry* m_DragDropEntry = nullptr;
		bool m_DragDropActive = false;

		bool m_IgnoreSelection = false;

		struct RenameContext
		{
			DirectoryEntry* Entry = nullptr;
			std::string Buffer;
			bool FirstFrame = false;
		};
		RenameContext m_RenameContext;
		bool m_ShowExtensionChangedWarning = false;

		bool m_SkipNextFileEvents = false;

		struct CreateEntryContext
		{
			std::string Buffer;
			std::string Extentsion;
			EntryType Type = EntryType::None;
			bool Show = false;

			void Clear()
			{
				Buffer.clear();
				Extentsion.clear();
				Type = EntryType::None;
				Show = false;
			}
		};
		CreateEntryContext m_CreateEntryContext;
	};

}
