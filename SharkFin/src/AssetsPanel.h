#pragma once

#include <Shark.h>
#include <unordered_map>

#include <imgui.h>
#include <imgui_internal.h>

namespace Shark {

	enum class ContentType
	{
		None = 0,
		Directory, File
	};

	struct Directory;

	struct Entry
	{
		ContentType Type = ContentType::None;
		uint32_t ByteSize = 0;
		Directory* Directory = nullptr;
	};

	struct Directory
	{
		static constexpr ContentType Type = ContentType::Directory;
		std::unordered_map<std::string, Entry> Entrys;
		uint32_t Files = 0;
		uint32_t Directorys = 0;
		uint32_t TotalFiles = 0;
		uint32_t TotalDirectorys = 0;
	};

	class AssetsPanel
	{
	public:
		AssetsPanel();
		~AssetsPanel();

		void ShowPanel(bool show) { m_ShowPanel = show; }
		bool IsShowen() const { return m_ShowPanel; }

		void OnImGuiRender();

	private:
		void SaveCurrentAssetDirectory();
		Directory* SaveDirectory(const std::filesystem::path& directoryPath);

		// Navigation
		void DrawHistoryNavigationButtons();
		void DrawCurrentPath();

		// TreeView
		void DrawAsTree(const std::string& directory);
		void DrawTreeNode(const std::string& directory);

		// Current Directory Content View
		void DrawCurrentDirectory();

		// Helpers
		void CheckOnCell(const Entry& entry, const std::string& path, const ImRect& rec);
		void CheckOnTreeLeaf(const Entry& entry, const std::string& path);
		void SelectCurrentDirectory(const std::filesystem::path& directoryPath);
		void UpdateCurrentPathVec();
		RenderID GetContentTextureID(const Entry& entry);
		void StartDragDrop(const std::string& path);
		void DrawContentPopup(const std::string& path, const Entry& entry);
		void StartRename(const std::string& path);
		void DrawRenameInput();

	private:
		bool m_ShowPanel = true;

		std::filesystem::path m_CurrentDirectory;
		std::string m_CurrentDirectoryString;
		std::unordered_map<std::string, Directory> m_Directorys;
		bool m_ReloadRequierd = false;

		std::vector<std::filesystem::path> m_DirectoryHistory;
		uint32_t m_DirHistoryIndex = 0;

		std::vector<std::string> m_CurrentPathVec;

		const float m_ContentItemSize = 80.0f;

		Ref<Texture2D> m_FolderImage;
		Ref<Texture2D> m_FileImage;

		std::string m_SelectedEntry;
		bool m_IgnoreNextSelectionCheck = false;
		bool m_DoNotHilight = false;

		bool m_OnRenameEntry = false;
		std::string m_EntryRenameBuffer;
		std::string m_RenameTarget;

	};

}
