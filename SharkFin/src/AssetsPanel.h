#pragma once

#include <Shark.h>
#include <unordered_map>

#include <imgui.h>
#include <imgui_internal.h>

namespace Shark {

	struct Directory;

	struct Entry
	{
		enum class ContentType
		{
			None = 0,
			Directory, File
		};
		ContentType Type = ContentType::None;
		uint32_t ByteSize = 0;
		Directory* Directory = nullptr;
	};

	struct Directory
	{
		static constexpr Entry::ContentType Type = Entry::ContentType::Directory;
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

		void ReCache() { m_ReloadRequierd = true; }

		void OnImGuiRender(bool& showPanel);

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

		// ...
		void DrawContentPopup(const std::string& path, const Entry& entry);
		void DrawRenameInput();
		void DeletePopup(const std::string& path, const Entry& entry);


		// Helpers
		void CheckOnCell(const Entry& entry, const std::string& path, const ImRect& rec);
		void CheckOnTreeLeaf(const Entry& entry, const std::string& path);

		void SelectCurrentDirectory(const std::filesystem::path& directoryPath);
		void UpdateCurrentPathVec();

		void StartDragDrop(const std::string& path, Entry::ContentType type);
		void StartRename(const std::string& path);
		void StartDelete(const std::string& path);

		Entry& GetEntry(const std::string& path);
		RenderID GetContentTextureID(const Entry& entry);

	private:
		const Project& m_Project;

		Ref<Texture2D> m_DirectoryIcon;
		Ref<Texture2D> m_StandartFileIcon;
		int m_IconSize = 80;


		std::unordered_map<std::string, Directory> m_Directorys;
		bool m_ReloadRequierd = true;


		std::filesystem::path m_CurrentDirectory;
		std::string m_CurrentDirectoryString;
		std::vector<std::string> m_CurrentPathVec;


		std::vector<std::filesystem::path> m_DirectoryHistory;
		uint32_t m_DirHistoryIndex = 0;


		std::string m_SelectedEntry;
		bool m_IgnoreNextSelectionCheck = false;
		bool m_DoNotHilight = false;


		bool m_OnRenameEntry = false;
		std::string m_EntryRenameBuffer;
		std::string m_RenameTarget;


		bool m_ShowDeletePopup = false;

		bool m_AdditionalInit = true;
		const char* m_DeletrEntryName = "Delete Entry";
		ImGuiID m_DeleteEntryID;

	};

}
