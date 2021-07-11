#include "AssetsPanel.h"

#include <misc/cpp/imgui_stdlib.h>
#include <fstream>
#include <Shark/Utility/PlatformUtils.h>

namespace Shark {

	const std::string s_AsstesDirectory = "assets";
	static std::string s_TempInputString;

	namespace Utils {

		static ContentType GetEntryTypeFromDirectoryEntry(const std::filesystem::directory_entry& entry)
		{
			if (entry.is_directory())
				return ContentType::Directory;
			if (entry.is_regular_file())
				return ContentType::File;
			SK_CORE_ASSERT(false);
			return ContentType::None;
		}

		static AssetType GetPaylodType(const std::string& filepath)
		{
			auto&& extension = Utility::GetFileExtention(filepath);
			if (extension == ".shark")
				return AssetType::Scene;
			if (extension == ".png")
				return AssetType::Texture;
			return AssetType::Unkown;
		}

		static const char* GetContentTypeAsString(ContentType type)
		{
			switch (type)
			{
				case ContentType::None:         return "None";
				case ContentType::Directory:    return "Directory";
				case ContentType::File:         return "File";
			}
			SK_CORE_ASSERT(false);
			return "Unkown";
		}

	}

	AssetsPanel::AssetsPanel()
	{
		m_CurrentDirectory = s_AsstesDirectory;
		m_CurrentDirectoryString = m_CurrentDirectory.string();
		m_DirectoryHistory.emplace_back(m_CurrentDirectory);
		m_DirHistoryIndex;
		SaveCurrentAssetDirectory();
		UpdateCurrentPathVec();

		m_FolderImage = Texture2D::Create("assets/Textures/folder_open.png");
		m_FileImage = Texture2D::Create("assets/Textures/file.png");
	}

	AssetsPanel::~AssetsPanel()
	{
	}

	void AssetsPanel::OnImGuiRender()
	{
		if (!m_ShowPanel)
			return;

		if (!ImGui::Begin("Assets", &m_ShowPanel))
		{
			ImGui::End();
			return;
		}

		if (m_ReloadRequierd)
		{
			SaveCurrentAssetDirectory();
			m_ReloadRequierd = false;
		}

		auto window = ImGui::GetCurrentWindow();
		auto& style = ImGui::GetStyle();
		window->DC.CursorPos.y -= style.FramePadding.y;

		DrawHistoryNavigationButtons();
		ImGui::SameLine();
		if (ImGui::Button("Reload"))
			SaveCurrentAssetDirectory();
		ImGui::SameLine();
		ImGui::SetNextItemWidth(150);
		ImGui::InputTextWithHint("##EntrySceach", "Sceach...", &s_TempInputString);
		ImGui::SameLine();
		DrawCurrentPath();

		ImGui::Separator();

		ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.5f, 0.5f, 0.5f, 0.3f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.5f, 0.5f, 0.5f, 0.1f });
		ImGui::PushStyleColor(ImGuiCol_Button, { 0.0f, 0.0f, 0.0f, 0.0f });

		if (ImGui::BeginTable("Assets", 2, ImGuiTableFlags_Resizable))
		{
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);

			DrawAsTree(s_AsstesDirectory);

			ImGui::TableSetColumnIndex(1);

			DrawCurrentDirectory();


			ImGui::EndTable();
		}

		m_IgnoreNextSelectionCheck = false;

		ImGui::PopStyleColor(3);

		ImGui::End();
	}

	void AssetsPanel::SaveCurrentAssetDirectory()
	{
		m_Directorys.clear();
		SaveDirectory(std::filesystem::absolute(s_AsstesDirectory));
	}

	Directory* AssetsPanel::SaveDirectory(const std::filesystem::path& directoryPath)
	{
		std::filesystem::path relative = std::filesystem::relative(directoryPath);
		Directory& directory = m_Directorys[relative.string()];
		for (auto&& directoryEntry : std::filesystem::directory_iterator(directoryPath))
		{
			auto&& path = directoryEntry.path();
			auto&& pathRelative = std::filesystem::relative(path);
			auto&& stringPath = pathRelative.string();
			auto&& entry = directory.Entrys[stringPath];

			entry.Type = Utils::GetEntryTypeFromDirectoryEntry(directoryEntry);
			if (entry.Type == ContentType::File)
			{
				entry.ByteSize = directoryEntry.file_size();
				directory.Files++;
			}

			if (entry.Type == ContentType::Directory)
			{
				directory.Directorys++;
				entry.Directory = SaveDirectory(path);
			}
		}

		for (auto&& directoryEntry : std::filesystem::recursive_directory_iterator(directoryPath))
		{
			if (directoryEntry.is_directory())
				directory.TotalDirectorys++;
			if (directoryEntry.is_regular_file())
				directory.TotalFiles++;
		}
		return &directory;
	}

	void AssetsPanel::DrawHistoryNavigationButtons()
	{
		if (ImGui::ArrowButton("DirectoryBack", ImGuiDir_Left))
		{
			if (m_DirHistoryIndex > 0)
			{
				m_CurrentDirectory = m_DirectoryHistory[--m_DirHistoryIndex];
				m_CurrentDirectoryString = m_CurrentDirectory.string();
				UpdateCurrentPathVec();
			}
		}

		ImGui::SameLine();
		if (ImGui::ArrowButton("DirectoryForward", ImGuiDir_Right))
		{
			if (m_DirHistoryIndex < m_DirectoryHistory.size() - 1)
			{
				m_CurrentDirectory = m_DirectoryHistory[++m_DirHistoryIndex];
				m_CurrentDirectoryString = m_CurrentDirectory.string();
				UpdateCurrentPathVec();
			}
		}
	}

	void AssetsPanel::DrawCurrentPath()
	{
		auto&& begin = m_CurrentPathVec.begin();
		auto&& end = m_CurrentPathVec.end();
		for (auto&& pathElem = begin; pathElem != end; std::advance(pathElem, 1))
		{
			ImGuiWindow* window = ImGui::GetCurrentWindow();
			const ImVec2 prevCursor = window->DC.CursorPos;

			const std::string& label = *pathElem;
			const ImVec2 label_size = ImGui::CalcTextSize(label.c_str(), NULL, true);

			auto& style = ImGui::GetStyle();
			ImVec2 size = ImGui::CalcItemSize({ 0, 0 }, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

			const bool isClicked = ImGui::InvisibleButton(label.c_str(), size);
			const bool isHoverd = ImGui::IsItemHovered();

			window->DC.CursorPos = prevCursor;

			ImGui::AlignTextToFramePadding();
			if (isHoverd)
				ImGui::Text(label.c_str());
			else
				ImGui::TextDisabled(label.c_str());

			if (pathElem != end - 1)
			{
				ImGui::SameLine(0.0f, 0.0f);
				ImGui::AlignTextToFramePadding();
				ImGui::TextDisabled("\\\\");
				ImGui::SameLine(0.0f, 0.0f);
			}

			if (isClicked)
			{
				m_CurrentDirectory.clear();
				for (auto i = m_CurrentPathVec.begin(); i != pathElem + 1; ++i)
					m_CurrentDirectory /= *i;
				m_CurrentPathVec.erase(pathElem + 1, end);
				m_CurrentDirectoryString = m_CurrentDirectory.string();
				break;
			}
		}
	}

	void AssetsPanel::DrawAsTree(const std::string& directory)
	{
		auto name = Utility::GetPathName(directory);
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_DefaultOpen;
		if (directory == m_SelectedEntry)
			flags |= ImGuiTreeNodeFlags_Selected;
		bool opened = ImGui::TreeNodeEx(name.data(), flags);
		CheckOnTreeLeaf({ ContentType::Directory }, directory);
		if (opened)
		{
			DrawTreeNode(directory);
			ImGui::TreePop();
		}
	}

	void AssetsPanel::DrawTreeNode(const std::string& directory)
	{
		constexpr ImGuiTreeNodeFlags baseFlags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
		auto&& entrys = m_Directorys[directory].Entrys;
		for (auto&& [subpath, subentry] : entrys)
		{
			ImGuiTreeNodeFlags nodeflags = baseFlags;
			if (subentry.Type == ContentType::File)
				nodeflags |= ImGuiTreeNodeFlags_Bullet;
			if (subpath == m_SelectedEntry)
				nodeflags |= ImGuiTreeNodeFlags_Selected;

			std::string_view name = Utility::GetPathName(subpath);
			bool opened = ImGui::TreeNodeEx(name.data(), nodeflags);
			CheckOnTreeLeaf(subentry, subpath);
			if (opened)
			{
				if (subentry.Type == ContentType::Directory)
					DrawTreeNode(subpath);

				ImGui::TreePop();
			}
		}
	}

	void AssetsPanel::DrawCurrentDirectory()
	{
		const int collumns = ImGui::GetContentRegionAvailWidth() / m_ContentItemSize;

		ImGuiWindow* window = ImGui::GetCurrentWindow();
		ImGuiStyle& style = ImGui::GetStyle();

		if (ImGui::BeginTable("Directory Content", collumns))
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			auto& directory = m_Directorys[m_CurrentDirectoryString];
			for (auto&& [path, entry] : directory.Entrys)
			{
				if (!m_DoNotHilight && m_SelectedEntry == path)
					ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, 0xFF202020);

				std::string_view name = Utility::GetPathName(path);
				const RenderID textureID = GetContentTextureID(entry);

				window->DC.CursorPos.x -= style.FramePadding.x;
				UI::ImageButton(path.c_str(), textureID, { m_ContentItemSize, m_ContentItemSize });
				ImGuiID buttonid = window->DC.LastItemId;
				if (m_OnRenameEntry && m_RenameTarget == path)
					DrawRenameInput();
				else
					ImGui::TextWrapped("%s", name.data());

				int collumnindex = ImGui::TableGetColumnIndex();
				ImGui::TableNextColumn();
				ImGuiTable* table = ImGui::GetCurrentTable();
				ImRect cellrect = ImGui::TableGetCellBgRect(table, collumnindex);
				window->DC.LastItemId = buttonid;
				CheckOnCell(entry, path, cellrect);
				
			}

			if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && ImGui::IsWindowHovered())
				ImGui::OpenPopup("Edit Current Directory", ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_NoOpenOverExistingPopup);
			if (ImGui::BeginPopup("Edit Current Directory"))
			{
				if (ImGui::Selectable("Open in Explorer"))
				{
					Utility::OpenExplorer(m_CurrentDirectoryString);
					ImGui::CloseCurrentPopup();
				}

				ImGui::Separator();
				if (ImGui::BeginMenu("New"))
				{
					if (ImGui::Selectable("Directory"))
					{
						auto path = m_CurrentDirectory / "new";
						FileSystem::CreateDirectory(path);
						StartRename(path.string());
						m_ReloadRequierd = true;
					}

					ImGui::Separator();
					if (ImGui::Selectable("Scene     (.shark)"))
					{
						auto path = m_CurrentDirectory / "new.shark";
						FileSystem::CreateFile(path);
						StartRename(path.string());
						m_ReloadRequierd = true;
					}

					ImGui::Separator();
					if (ImGui::Selectable("Text      (.txt)"))
					{
						auto path = m_CurrentDirectory / "new.txt";
						FileSystem::CreateFile(path);
						StartRename(path.string());
						m_ReloadRequierd = true;
					}
					ImGui::EndMenu();
				}

				ImGui::EndPopup();
			}

			ImGui::EndTable();
		}

	}

	void AssetsPanel::CheckOnCell(const Entry& entry, const std::string& path, const ImRect& rec)
	{
		ImGui::PushID(path.c_str());

		ImVec2 mousepos = ImGui::GetMousePos();
		const bool isItemHovered = rec.Contains(mousepos);
		const bool isMouseClicked_Left = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
		const bool isMouseClicked_Right = ImGui::IsMouseClicked(ImGuiMouseButton_Right);
	
		const bool isItemClicked_Left = isItemHovered && isMouseClicked_Left;
		const bool isItemClicked_Right = isItemHovered && isMouseClicked_Right;

		if (!m_IgnoreNextSelectionCheck)
		{
			if (isItemClicked_Left)
			{
				m_SelectedEntry = path;
				m_IgnoreNextSelectionCheck = true;
			}
			if (isMouseClicked_Left && !isItemHovered && m_SelectedEntry == path)
				m_SelectedEntry.clear();
		}

		switch (entry.Type)
		{
			case ContentType::Directory:
			{
				if (isItemHovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
					SelectCurrentDirectory(path);
				break;
			}
			case ContentType::File:
			{
				StartDragDrop(path);
				break;
			}
		}

		if (isItemClicked_Right)
			ImGui::OpenPopup(path.c_str());
		DrawContentPopup(path, entry);

		ImGui::PopID();
	}

	void AssetsPanel::CheckOnTreeLeaf(const Entry& entry, const std::string& path)
	{
		ImGui::PushID(path.c_str());

		const bool isItemHovered = ImGui::IsItemHovered();
		const bool isMouseClicked_Left = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
		const bool isMouseClicked_Right = ImGui::IsMouseClicked(ImGuiMouseButton_Right);
	
		const bool isItemClicked_Left = isItemHovered && isMouseClicked_Left;
		const bool isItemClicked_Right = isItemHovered && isMouseClicked_Right;

		if (!m_IgnoreNextSelectionCheck)
		{
			if (isItemClicked_Left)
			{
				m_SelectedEntry = path;
				m_IgnoreNextSelectionCheck = true;
			}
			if (isMouseClicked_Left && !isItemHovered && m_SelectedEntry == path)
				m_SelectedEntry.clear();
		}

		switch (entry.Type)
		{
			case ContentType::Directory:
			{
				if (isItemClicked_Left)
					SelectCurrentDirectory(path);
				break;
			}
			case ContentType::File:
			{
				StartDragDrop(path);
				break;
			}
		}

		if (isItemClicked_Right)
			ImGui::OpenPopup(path.c_str());
		DrawContentPopup(path, entry);

		ImGui::PopID();
	}

	void AssetsPanel::SelectCurrentDirectory(const std::filesystem::path& directoryPath)
	{
		++m_DirHistoryIndex;
		m_DirectoryHistory.erase(m_DirectoryHistory.begin() + m_DirHistoryIndex, m_DirectoryHistory.end());
		m_DirectoryHistory.emplace(m_DirectoryHistory.begin() + m_DirHistoryIndex, directoryPath);
		m_CurrentDirectory = directoryPath;
		m_CurrentDirectoryString = m_CurrentDirectory.string();

		UpdateCurrentPathVec();
	}

	void AssetsPanel::UpdateCurrentPathVec()
	{
		m_CurrentPathVec.clear();
		for (auto&& pathElem = m_CurrentDirectory.begin(); pathElem != m_CurrentDirectory.end(); ++pathElem)
			m_CurrentPathVec.emplace_back(pathElem->string());
	}

	RenderID AssetsPanel::GetContentTextureID(const Entry& entry)
	{
		if (entry.Type == ContentType::Directory)
			return m_FolderImage->GetRenderID();
		if (entry.Type == ContentType::File)
			return m_FileImage->GetRenderID();
		SK_CORE_ASSERT(false);
		return NullID;
	}

	void AssetsPanel::StartDragDrop(const std::string& path)
	{
		AssetType type = Utils::GetPaylodType(path);
		if (type == AssetType::Unkown)
			return;

		if (ImGui::BeginDragDropSource())
		{
			AssetPayload payload;
			strcpy(payload.FilePath, path.c_str());
			payload.Type = type;
			ImGui::SetDragDropPayload(AssetPayload::ID, &payload, sizeof(AssetPayload));
			ImGui::EndDragDropSource();
		}
	}

	void AssetsPanel::DrawContentPopup(const std::string& path, const Entry& entry)
	{
		if (ImGui::BeginPopup(path.c_str()))
		{
			const bool isDirectory = entry.Type == ContentType::Directory;
			const bool isFile = entry.Type == ContentType::File;

			std::string_view name = Utility::GetPathName(path);
			ImGui::Text("Name:        %s", name.data());
			ImGui::Text("Type:        %s", Utils::GetContentTypeAsString(entry.Type));
			ImGui::Text("Full Path:   %s", path.c_str());

			if (isDirectory)
			{
				ImGui::Text("Content:     Files: %d, Directorys: %d", entry.Directory->TotalFiles, entry.Directory->TotalDirectorys);
			}
			else if (isFile)
			{
				ImGui::Text("Size:        %d bytes", entry.ByteSize);
			}

			ImGui::Separator();
			if (ImGui::MenuItem("Open Native", nullptr, false, isFile))
			{
				Utility::OpenFile(path);
				ImGui::CloseCurrentPopup();
			}

			if (ImGui::MenuItem("Open in Explorer", nullptr, false, isDirectory))
			{
				Utility::OpenExplorer(path);
				ImGui::CloseCurrentPopup();
			}

			// Rename Content
			ImGui::Separator();
			if (ImGui::MenuItem("Rename", "F2"))
				StartRename(path);

			// Delete Content
			ImGui::Separator();
			if (ImGui::Selectable("Delete", false, ImGuiSelectableFlags_DontClosePopups))
				ImGui::OpenPopup("Delete Entry");

			if (ImGui::BeginPopupModal("Delete Entry"))
			{
				ImGui::Text("Do you want to delete this entry");
				ImGui::Text("Path: %s", path.c_str());
				ImGui::Separator();
				if (ImGui::Button("Delete") || ImGui::IsKeyPressed(ImGuiKey_Enter))
				{
					if (isDirectory)
					{
						FileSystem::DeleteAll(path);
					}
					else if (isFile)
					{
						FileSystem::Delete(path);
					}
					m_ReloadRequierd = true;
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();
				if (ImGui::Button("Cancle"))
					ImGui::CloseCurrentPopup();
				ImGui::EndPopup();
			}

			ImGui::EndPopup();
		}

	}

	void AssetsPanel::StartRename(const std::string& path)
	{
		m_EntryRenameBuffer = Utility::GetPathName(path);
		m_OnRenameEntry = true;
		m_RenameTarget = path;
		m_DoNotHilight = true;
	}

	void AssetsPanel::DrawRenameInput()
	{
		ImGuiTable* table = ImGui::GetCurrentTable();
		ImGuiTableColumn* collumn = &table->Columns[table->CurrentColumn];
		ImGui::SetNextItemWidth(collumn->WidthGiven);

		if (ImGui::InputText("##Rename Current Content", &m_EntryRenameBuffer, ImGuiInputTextFlags_EnterReturnsTrue))
		{
			if (Utility::GetPathName(m_RenameTarget) != m_EntryRenameBuffer)
			{
				auto newPath = std::filesystem::path(m_RenameTarget).parent_path() / m_EntryRenameBuffer;

				FileSystem::Rename(m_RenameTarget, newPath);
				//if (FileSystem::Rename(m_RenameTarget, newPath))
				//	if (std::filesystem::is_directory(newPath) && m_RenameTarget == m_SelectedEntry)
				//		SelectCurrentDirectory(newPath);
			}
			
			m_EntryRenameBuffer.clear();
			m_RenameTarget.clear();
			m_OnRenameEntry = false;
			m_ReloadRequierd = true;
			m_DoNotHilight = false;
		}
	}

}