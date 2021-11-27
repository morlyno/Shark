#include "skfpch.h"
#include "AssetsPanel.h"

#include "Shark/Debug/Instrumentor.h"

#include <Shark/Utility/PlatformUtils.h>
#include <misc/cpp/imgui_stdlib.h>

namespace Shark {

	static std::string s_TempInputString;

	namespace Utils {

		static Entry::ContentType GetEntryTypeFromDirectoryEntry(const std::filesystem::directory_entry& entry)
		{
			if (entry.is_directory())
				return Entry::ContentType::Directory;
			if (entry.is_regular_file())
				return Entry::ContentType::File;
			SK_CORE_ASSERT(false);
			return Entry::ContentType::None;
		}

		static UI::ContentType GetPaylodType(const std::string& filepath)
		{
			auto&& extension = Utility::GetFileExtention(filepath);
			if (extension == ".shark")
				return UI::ContentType::Scene;
			if (extension == ".png")
				return UI::ContentType::Texture;
			return UI::ContentType::Unkown;
		}

		static std::string GetContentTypeAsString(Entry::ContentType type)
		{
			switch (type)
			{
				case Entry::ContentType::None:         return "None";
				case Entry::ContentType::Directory:    return "Directory";
				case Entry::ContentType::File:         return "File";
			}
			SK_CORE_ASSERT(false);
			return "Unkown";
		}

	}

	AssetsPanel::AssetsPanel()
		: m_Project(Application::Get().GetProject())
	{
		SK_PROFILE_FUNCTION();
		
		m_DirectoryIcon = Texture2D::Create("Resources/AssetsPanel/folder_open.png");
		m_StandartFileIcon = Texture2D::Create("Resources/AssetsPanel/file.png");

		m_DirectoryHistory.emplace_back(m_Project.GetAssetsPath());
		m_CurrentDirectory = m_Project.GetAssetsPath();
		m_CurrentDirectoryString = m_CurrentDirectory.string();

		UpdateCurrentPathVec();
		FileWatcher::AddCallback("AssetsPanel", [this](const auto&) { ReCache(); });

	}

	AssetsPanel::~AssetsPanel()
	{
		SK_PROFILE_FUNCTION();
		
		FileWatcher::RemoveCallback("AssetsPanel");
	}

	void AssetsPanel::OnImGuiRender(bool& showPanel)
	{
		SK_PROFILE_FUNCTION();
		
		if (!showPanel)
			return;

		if (!ImGui::Begin("Assets", &showPanel, ImGuiWindowFlags_NoScrollbar))
		{
			ImGui::End();
			return;
		}

		if (m_AdditionalInit)
		{
			ImGuiWindow* window = ImGui::GetCurrentWindow();
			m_DeleteEntryID = window->GetID(m_DeletrEntryName);
		}

		if (m_ReloadRequierd)
		{
			m_ReloadRequierd = false;
			SaveCurrentAssetDirectory();
		}

		if (!m_SelectedEntry.empty())
		{
			if (ImGui::IsKeyPressed(Key::F2, false))
				StartRename(m_SelectedEntry);
			if (ImGui::IsKeyPressed(Key::Entf, false))
				StartDelete(m_SelectedEntry);
		}

		if (m_ShowDeletePopup)
			DeletePopup(m_SelectedEntry, GetEntry(m_SelectedEntry));

		//ImGui::SetCursorPosY(ImGui::GetCursorPosY() - UI::GetFramePadding().y);
		ImGuiStyle& style = ImGui::GetStyle();
		ImGui::SetCursorPos(ImGui::GetCursorPos() - style.FramePadding);

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

		const float maxHeight = ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing();
		if (ImGui::BeginTable("Assets", 2, ImGuiTableFlags_Resizable))
		{
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);

			// TODO(moro): maybe switch form string to filesystem::path
			auto temp = m_Project.GetAssetsPath();
			DrawAsTree(temp.string());

			ImGui::TableSetColumnIndex(1);

			DrawCurrentDirectory();

			ImGui::EndTable();
		}

		m_IgnoreNextSelectionCheck = false;

		ImGui::PopStyleColor(3);

		UI::MoveCurserPosY(ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeight());
		ImGui::Separator();

		const float widthAvailable = ImGui::GetContentRegionAvail().x;
		const float textSize = ImGui::CalcTextSize("Icon Size").x + style.FramePadding.x * 2.0f;
		const float sliderSize = (widthAvailable - textSize - style.FramePadding.x) * 0.2f;
		const float offset = widthAvailable - textSize - sliderSize;
		
		UI::MoveCurserPosX(offset);
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Icon Size");
		ImGui::SameLine();
		ImGui::PushItemWidth(sliderSize);
		ImGui::SliderInt("##Icon Size", &m_IconSize, 40, 180);

		ImGui::End();
	}

	void AssetsPanel::SaveCurrentAssetDirectory()
	{
		SK_PROFILE_FUNCTION();
		
		auto&& assetsPath = m_Project.GetAssetsPath();
		if (FileSystem::Exists(assetsPath))
		{
			m_Directorys.clear();
			SaveDirectory(assetsPath);
		}
	}

	Directory* AssetsPanel::SaveDirectory(const std::filesystem::path& directoryPath)
	{
		SK_PROFILE_FUNCTION();
		
		Directory& directory = m_Directorys[directoryPath.string()];
		for (auto&& directoryEntry : std::filesystem::directory_iterator(directoryPath))
		{
			auto&& path = directoryEntry.path();
			auto&& stringPath = path.string();
			auto&& entry = directory.Entrys[stringPath];

			entry.Type = Utils::GetEntryTypeFromDirectoryEntry(directoryEntry);
			if (entry.Type == Entry::ContentType::File)
			{
				entry.ByteSize = (uint32_t)directoryEntry.file_size();
				directory.Files++;
			}

			if (entry.Type == Entry::ContentType::Directory)
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
		SK_PROFILE_FUNCTION();
		
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
		SK_PROFILE_FUNCTION();
		
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
				ImGui::TextDisabled("/");
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
		SK_PROFILE_FUNCTION();
		
		auto name = Utility::GetPathName(directory);
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_DefaultOpen;
		if (directory == m_SelectedEntry)
			flags |= ImGuiTreeNodeFlags_Selected;
		bool opened = ImGui::TreeNodeEx(name.data(), flags);
		CheckOnTreeLeaf({ Entry::ContentType::Directory }, directory);
		if (opened)
		{
			DrawTreeNode(directory);
			ImGui::TreePop();
		}
	}

	void AssetsPanel::DrawTreeNode(const std::string& directory)
	{
		SK_PROFILE_FUNCTION();
		
		constexpr ImGuiTreeNodeFlags baseFlags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
		auto&& entrys = m_Directorys[directory].Entrys;
		for (auto&& [subpath, subentry] : entrys)
		{
			ImGuiTreeNodeFlags nodeflags = baseFlags;
			if (subentry.Type == Entry::ContentType::File)
				nodeflags |= ImGuiTreeNodeFlags_Bullet;
			if (subpath == m_SelectedEntry)
				nodeflags |= ImGuiTreeNodeFlags_Selected;

			std::string_view name = Utility::GetPathName(subpath);
			bool opened = ImGui::TreeNodeEx(name.data(), nodeflags);
			CheckOnTreeLeaf(subentry, subpath);
			if (opened)
			{
				if (subentry.Type == Entry::ContentType::Directory)
					DrawTreeNode(subpath);

				ImGui::TreePop();
			}
		}
	}

	void AssetsPanel::DrawCurrentDirectory()
	{
		SK_PROFILE_FUNCTION();
		
		const int maxCollumns = std::clamp((int)(ImGui::GetContentRegionAvailWidth() / m_IconSize), 1, 64);
		const float maxHeight = ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing() - 1;
		ImGui::PushStyleColor(ImGuiCol_ChildBg, { 0.0f, 0.0f, 0.0f, 0.0f });
		const bool tableActive = ImGui::BeginTable("Directory Content", maxCollumns, ImGuiTableFlags_ScrollY, { 0, maxHeight });
		ImGui::PopStyleColor();
		if (tableActive)
		{
			ImGuiWindow* window = ImGui::GetCurrentWindow();
			ImGuiStyle& style = ImGui::GetStyle();

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			const auto& directory = m_Directorys[m_CurrentDirectoryString];
			for (auto&& [path, entry] : directory.Entrys)
			{
				if (!m_DoNotHilight && m_SelectedEntry == path)
					ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, 0xFF202020);

				std::string_view name = Utility::GetPathName(path);
				const RenderID textureID = GetContentTextureID(entry);

				window->DC.CursorPos.x -= style.FramePadding.x;
				UI::ImageButton(path, textureID, { (float)m_IconSize, (float)m_IconSize });
				ImGuiID buttonid = window->DC.LastItemId;
				if (m_OnRenameEntry && m_RenameTarget == path)
					DrawRenameInput();
				else
					ImGui::TextWrapped("%s", name.data());

				
				ImGuiTable* table = ImGui::GetCurrentTable();
				const int collumnIndex = table->CurrentColumn;

				float top = 0.0f;
				if (collumnIndex == maxCollumns - 1)
				{
					ImRect r = ImGui::TableGetCellBgRect(table, 0);
					top = r.Min.y;
				}

				ImGui::TableNextColumn();
				ImRect cellrect = ImGui::TableGetCellBgRect(table, collumnIndex);
				if (top != 0.0f)
				{
					cellrect.Min.y = top;
					cellrect.Max.y -= style.FramePadding.x;
				}
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

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// ... //////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void AssetsPanel::DrawContentPopup(const std::string& path, const Entry& entry)
	{
		SK_PROFILE_FUNCTION();
		
		if (ImGui::BeginPopup(path.c_str()))
		{
			const bool isDirectory = entry.Type == Entry::ContentType::Directory;
			const bool isFile = entry.Type == Entry::ContentType::File;

			std::string_view name = Utility::GetPathName(path);
			ImGui::Text("Name:        %s", name.data());
			ImGui::Text("Type:        %s", Utils::GetContentTypeAsString(entry.Type).c_str());
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

			if (ImGui::MenuItem("Open With", nullptr, false, isFile))
			{
				Utility::OpenWith(path);
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
			const bool startDelete = ImGui::MenuItem("Delete", "del", false, ImGuiSelectableFlags_DontClosePopups);

			ImGui::EndPopup();

			if (startDelete)
				StartDelete(path);
		}

	}

	void AssetsPanel::DrawRenameInput()
	{
		SK_PROFILE_FUNCTION();
		
		ImGuiTable* table = ImGui::GetCurrentTable();
		ImGuiTableColumn* collumn = &table->Columns[table->CurrentColumn];
		ImGui::SetNextItemWidth(collumn->WidthGiven);

		if (ImGui::InputText("##Rename Current Content", &m_EntryRenameBuffer, ImGuiInputTextFlags_EnterReturnsTrue))
		{
			if (Utility::GetPathName(m_RenameTarget) != m_EntryRenameBuffer)
			{
				auto newPath = std::filesystem::path(m_RenameTarget).parent_path() / m_EntryRenameBuffer;

				FileSystem::Rename(m_RenameTarget, newPath);
			}

			m_EntryRenameBuffer.clear();
			m_RenameTarget.clear();
			m_OnRenameEntry = false;
			m_ReloadRequierd = true;
			m_DoNotHilight = false;
		}
	}

	void AssetsPanel::DeletePopup(const std::string& path, const Entry& entry)
	{
		SK_PROFILE_FUNCTION();
		
		m_IgnoreNextSelectionCheck = true;
		const bool isDirectory = entry.Type == Entry::ContentType::Directory;
		const bool isFile = entry.Type == Entry::ContentType::File;

		ImGuiWindow* debug = ImGui::GetCurrentWindow();
		if (ImGui::BeginPopupModal(m_DeletrEntryName))
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
				m_SelectedEntry.clear();
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancle") || ImGui::IsKeyPressed(ImGuiKey_Escape))
				ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
		}
		if (!ImGui::IsPopupOpen(m_DeleteEntryID, ImGuiPopupFlags_None))
			m_ShowDeletePopup = false;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Utilitys /////////////////////////////////////////////////////////////////////////////////////////////////////////

	void AssetsPanel::CheckOnCell(const Entry& entry, const std::string& path, const ImRect& rec)
	{
		SK_PROFILE_FUNCTION();
		
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

		if (entry.Type == Entry::ContentType::Directory && isItemHovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			SelectCurrentDirectory(path);

		StartDragDrop(path, entry.Type);

		if (isItemClicked_Right)
			ImGui::OpenPopup(path.c_str());
		DrawContentPopup(path, entry);

		ImGui::PopID();
	}

	void AssetsPanel::CheckOnTreeLeaf(const Entry& entry, const std::string& path)
	{
		SK_PROFILE_FUNCTION();
		
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

		if (entry.Type == Entry::ContentType::Directory && isItemClicked_Left)
			SelectCurrentDirectory(path);
		
		StartDragDrop(path, entry.Type);

		if (isItemClicked_Right)
			ImGui::OpenPopup(path.c_str());
		DrawContentPopup(path, entry);

		ImGui::PopID();
	}

	void AssetsPanel::SelectCurrentDirectory(const std::filesystem::path& directoryPath)
	{
		SK_PROFILE_FUNCTION();
		
		++m_DirHistoryIndex;
		m_DirectoryHistory.erase(m_DirectoryHistory.begin() + m_DirHistoryIndex, m_DirectoryHistory.end());
		m_DirectoryHistory.emplace(m_DirectoryHistory.begin() + m_DirHistoryIndex, directoryPath);
		m_CurrentDirectory = directoryPath;
		m_CurrentDirectoryString = m_CurrentDirectory.string();

		UpdateCurrentPathVec();
	}

	void AssetsPanel::UpdateCurrentPathVec()
	{
		SK_PROFILE_FUNCTION();
		
		m_CurrentPathVec.clear();
		for (auto&& pathElem = m_CurrentDirectory.begin(); pathElem != m_CurrentDirectory.end(); ++pathElem)
			m_CurrentPathVec.emplace_back(pathElem->string());
	}

	void AssetsPanel::StartDragDrop(const std::string& path, Entry::ContentType t)
	{
		SK_PROFILE_FUNCTION();
		
		UI::ContentType type = UI::ContentType::Directory;
		if (t == Entry::ContentType::File)
			type = Utils::GetPaylodType(path);
		if (type == UI::ContentType::Unkown)
			return;

		if (ImGui::BeginDragDropSource())
		{
			UI::ContentPayload payload;
			strcpy_s(payload.Path, path.c_str());
			payload.Type = type;
			ImGui::SetDragDropPayload(UI::ContentPayload::ID, &payload, sizeof(UI::ContentPayload));
			ImGui::EndDragDropSource();
		}
	}

	void AssetsPanel::StartRename(const std::string& path)
	{
		SK_PROFILE_FUNCTION();
		
		m_EntryRenameBuffer = Utility::GetPathName(path);
		m_OnRenameEntry = true;
		m_RenameTarget = path;
		m_DoNotHilight = true;
	}

	void AssetsPanel::StartDelete(const std::string& path)
	{
		SK_PROFILE_FUNCTION();
		
		m_SelectedEntry = path;
		m_ShowDeletePopup = true;
		m_IgnoreNextSelectionCheck = true;
		
		
		ImGui::OpenPopupEx(m_DeleteEntryID);
	}

	Entry& AssetsPanel::GetEntry(const std::string& path)
	{
		SK_CORE_ASSERT(!path.empty());
		return m_Directorys.at(m_CurrentDirectoryString).Entrys.at(path);
	}

	RenderID AssetsPanel::GetContentTextureID(const Entry& entry)
	{
		if (entry.Type == Entry::ContentType::Directory)
			return m_DirectoryIcon->GetRenderID();
		if (entry.Type == Entry::ContentType::File)
			return m_StandartFileIcon->GetRenderID();
		SK_CORE_ASSERT(false);
		return RenderID{};
	}

}