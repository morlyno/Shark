#include "skfpch.h"
#include "ContentBrowserPanel.h"

#include "Shark/Core/Project.h"

#include "Shark/Debug/Instrumentor.h"

#include <Shark/Utility/PlatformUtils.h>
#include <misc/cpp/imgui_stdlib.h>

namespace Shark {

	ContentBrowserPanel::ContentBrowserPanel()
	{
		SK_PROFILE_FUNCTION();

		if (Project::GetActive())
			CacheDirectory(Project::GetAssetsPath());

		m_FolderIcon   = Texture2D::Create("Resources/ContentBrowser/folder_open.png");
		m_FileIcon     = Texture2D::Create("Resources/ContentBrowser/file.png");
		m_PNGIcon      = Texture2D::Create("Resources/ContentBrowser/Icon_PNG.png");
		m_SceneIcon    = Texture2D::Create("Resources/ContentBrowser/Icon_Scene.png");
		m_TextureIcon  = Texture2D::Create("Resources/ContentBrowser/Icon_Texture.png");
	}

	ContentBrowserPanel::~ContentBrowserPanel()
	{
		SK_PROFILE_FUNCTION();
	}

	void ContentBrowserPanel::OnImGuiRender(bool& shown)
	{
		SK_PROFILE_FUNCTION();
		
		if (!shown)
			return;
		
		if (m_Reload)
		{
			CacheDirectory(Project::GetAssetsPath());
			m_Reload = false;
		}

		ImGui::Begin("Content Browser", &shown, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

		DrawMenuBar();
		ImGui::Separator();

		if (ImGui::BeginTable("ContentBrowser", 2, ImGuiTableFlags_Resizable))
		{
			if (!m_IgnoreSelection && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			{
				m_SelectedEntry = nullptr;
				m_IsSelectedHovered = false;
			}

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			DrawTreeView();

			ImGui::TableSetColumnIndex(1);
			DrawCellView();

			ImGui::EndTable();
		}

		DrawPopups();
		DrawDragDropTooltip();

		ImGui::End();
	}

	void ContentBrowserPanel::OnEvent(Event& event)
	{
		EventDispacher dispacher(event);
		dispacher.DispachEvent<ProjectChangedEvnet>([this](ProjectChangedEvnet& event) { Reload(); return true; });
	}

	void ContentBrowserPanel::DrawTreeView()
	{
		const ImGuiStyle& style = ImGui::GetStyle();
		ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, style.IndentSpacing * 0.5f);
		DrawTreeView(m_RootDirectory);
		ImGui::PopStyleVar();
	}

	void ContentBrowserPanel::DrawTreeView(DirectoryEntry& directory)
	{
		for (auto& entry : directory.ChildEntrys)
		{
			if (entry.Type != EntryType::Directory && m_Settings.ShowOnlyAssets && !entry.Handle.IsValid())
				continue;

			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding |
				ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow;
			if (entry.Type == EntryType::File)
				flags |= ImGuiTreeNodeFlags_Bullet;
			if (&entry == m_SelectedEntry)
				flags |= ImGuiTreeNodeFlags_Selected;

			const bool open = ImGui::TreeNodeEx(entry.DisplayName.c_str(), flags);
			
			if (ImGui::IsItemHovered())
			{
				if (!m_IgnoreSelection && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
				{
					m_SelectedEntry = &entry;
					if (entry.Type == EntryType::Directory)
						m_CurrentDirectory = m_SelectedEntry;
				}
			}

			if (open)
			{
				if (entry.Type == EntryType::Directory)
					DrawTreeView(entry);

				ImGui::TreePop();
			}

		}
	}

	void ContentBrowserPanel::DrawCellView()
	{
		const int cells = std::clamp((int)(ImGui::GetContentRegionAvail().x / m_CellWidth), 1, 64);
		const ImVec2 tableSize = ImGui::GetContentRegionAvail() - ImVec2{ 0.0f, ImGui::GetFrameHeightWithSpacing() };

		if (ImGui::BeginTable("CellView", cells, 0, ImGui::GetContentRegionAvail()))
		{
			ImGui::TableNextRow(0, m_MinCellHeight);

			if (m_CurrentDirectory)
			{
				for (auto& entry : m_CurrentDirectory->ChildEntrys)
				{
					if (entry.Type != EntryType::Directory && m_Settings.ShowOnlyAssets && !entry.Handle.IsValid())
						continue;

					ImGui::TableNextColumn(0, m_MinCellHeight);

					UI::PushID(entry.DisplayName);

					const bool isSelectedEntry = &entry == m_SelectedEntry;
					ImVec2 cursorPos = ImGui::GetCursorPos();
					ImGui::Selectable("##Dummy", isSelectedEntry, 0, { m_CellWidth, m_MinCellHeight });
					const bool isHovered = ImGui::IsItemHovered();

					if (isSelectedEntry)
						m_IsSelectedHovered = isHovered;

					if (isHovered)
					{
						if (!m_IgnoreSelection && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
							m_SelectedEntry = &entry;
						if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
						{
							if (entry.Type == EntryType::Directory)
								m_CurrentDirectory = m_SelectedEntry;
							else if (entry.Type == EntryType::File && entry.Handle)
								m_OpenAssetCallback(entry.Handle);
						}
					}

					if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceNoPreviewTooltip))
					{
						if (entry.Handle)
							ImGui::SetDragDropPayload("ASSET", &entry.Handle, sizeof(entry.Handle));
						else if (entry.Type == EntryType::File)
							ImGui::SetDragDropPayload("ASSET_FILEPATH", entry.Path.native().c_str(), entry.Path.native().size() * sizeof(wchar_t));
						else if (entry.Type == EntryType::Directory)
							ImGui::SetDragDropPayload("DIRECTORY_FILEPATH", entry.Path.native().c_str(), entry.Path.native().size() * sizeof(wchar_t));

						ImGui::EndDragDropSource();
						m_DragDropActive = true;
						m_DragDropEntry = &entry;
					}

					ImGui::SetCursorPos(cursorPos);

					DrawCell(entry);
					UI::PopID();

				}
			}
			ImGui::EndTable();
		}
	}

	void ContentBrowserPanel::DrawCell(DirectoryEntry& entry)
	{
		Ref<Texture2D> icon = GetCellIcon(entry);

		float imageSize = m_CellWidth;
		ImGui::Image(icon->GetViewID(), { imageSize, imageSize });
		UI::Text(entry.DisplayName);

		if (entry.Type != EntryType::Directory)
		{
			auto& metaData = ResourceManager::GetMetaData(entry.Handle);
			UI::Text(AssetTypeToString(metaData.Type));
		}
	}

	void ContentBrowserPanel::DrawMenuBar()
	{
		ImGui::Button("<");
		ImGui::SameLine();
		ImGui::Button(">");

		ImGui::SameLine();
		if (ImGui::Button("Reload"))
			m_Reload = true;

		// Current Path w/ selection
		std::filesystem::path newPath;

		ImGui::SameLine();
		auto prevToEnd = --m_CurrentDirectory->Path.end();
		for (auto elem = m_CurrentDirectory->Path.begin(); elem != m_CurrentDirectory->Path.end(); elem++)
		{
			// invisible button
			// disabled text (enabled when hovered)
			// seperator

			// calc text/button size
			ImGuiStyle& style = ImGui::GetStyle();
			auto label = elem->string();
			const ImVec2 label_size = ImGui::CalcTextSize(label.c_str(), NULL, true);
			const ImVec2 size = ImGui::CalcItemSize({ 0.0f, 0.0f }, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

			const ImVec2 cursor = ImGui::GetCursorPos();

			if (ImGui::InvisibleButton(label.c_str(), size))
				newPath = Utility::CreatePathFormIterator(m_CurrentDirectory->Path.begin(), std::next(elem));

			ImGui::SetCursorPos(cursor);
			UI::TextFlags flags = UI::TextFlag::Aligned;
			if (!ImGui::IsItemHovered())
				flags |= UI::TextFlag::Disabled;

			UI::Text(*elem, flags);

			if (elem != prevToEnd)
			{
				ImGui::SameLine(0.0f, 0.0f);
				UI::Text("/", UI::TextFlag::Disabled | UI::TextFlag::Aligned);
				ImGui::SameLine(0.0f, 0.0f);
			}
		}

		if (!newPath.empty())
			m_CurrentDirectory = GetEntry(newPath);

		ImGui::SameLine();
		UI::MoveCursorX(ImGui::GetContentRegionAvail().x - ImGui::GetFrameHeight());

		ImGui::PushStyleColor(ImGuiCol_Button, UI::Theme::GetColors().ButtonNoBg);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, UI::Theme::GetColors().ButtonHoveredNoBg);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, UI::Theme::GetColors().ButtonActiveNoBg);

		if (ImGui::Button("*", { ImGui::GetFrameHeight(), ImGui::GetFrameHeight() }))
			ImGui::OpenPopup("Settings");

		if (ImGui::BeginPopup("Settings"))
		{
			UI::BeginControlsGrid();
			UI::Control("Show only Assets", m_Settings.ShowOnlyAssets);
			UI::EndControls();

			ImGui::EndPopup();
		}

		ImGui::PopStyleColor(3);

	}

	void ContentBrowserPanel::DrawPopups()
	{
		if (ImGui::IsWindowFocused() && ImGui::IsWindowHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
		{
			if (m_SelectedEntry && m_IsSelectedHovered)
				ImGui::OpenPopup("Entry Settings");
			else
				ImGui::OpenPopup("Directory Settings");
		}

		m_IgnoreSelection = false;
		if (ImGui::BeginPopup("Entry Settings"))
		{
			m_IgnoreSelection = true;
			SK_CORE_ASSERT(m_SelectedEntry);

			if (ImGui::MenuItem("Reload", nullptr, false, m_SelectedEntry->Handle))
				ResourceManager::ReloadAsset(m_SelectedEntry->Handle);

			if (ImGui::MenuItem("Import", nullptr, false, m_SelectedEntry->Type == EntryType::File && !m_SelectedEntry->Handle.IsValid()))
				ImportEntry(*m_SelectedEntry);

			ImGui::Separator();

			if (ImGui::Selectable("Open"))           Utility::OpenFile(Project::AbsolueCopy(m_SelectedEntry->Path));
			if (ImGui::Selectable("Open With"))      Utility::OpenFileWith(Project::AbsolueCopy(m_SelectedEntry->Path));
			if (ImGui::Selectable("Open Explorer"))  Utility::OpenExplorer(Project::AbsolueCopy(m_CurrentDirectory->Path));

			ImGui::EndPopup();
		}

		if (ImGui::BeginPopup("Directory Settings"))
		{
			m_IgnoreSelection = true;
			UI::Text("Directory Settings");
			ImGui::EndPopup();
		}
	}

	void ContentBrowserPanel::DrawDragDropTooltip()
	{
		if (m_DragDropActive)
		{
			ImGui::BeginTooltip();
			UI::Text(m_DragDropEntry->DisplayName);
			ImGui::EndTooltip();

			if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
			{
				m_DragDropActive = false;
				m_DragDropEntry = nullptr;
			}
		}
	}

	void ContentBrowserPanel::CacheDirectory(const std::filesystem::path& rootPath)
	{
		std::filesystem::path selectedEntry;
		std::filesystem::path currentDirectory;
		if (m_SelectedEntry) selectedEntry = m_SelectedEntry->Path;
		if (m_CurrentDirectory) currentDirectory = m_CurrentDirectory->Path;

		m_RootDirectory.ChildEntrys.clear();
		m_RootDirectory.Type = EntryType::Directory;
		m_RootDirectory.Handle = AssetHandle::Null();
		m_RootDirectory.Path = Project::RelativeCopy(rootPath);
		CacheDirectory(m_RootDirectory, rootPath);
		m_CurrentDirectory = &m_RootDirectory;

		m_SelectedEntry = GetEntry(selectedEntry);
		m_CurrentDirectory = GetEntry(currentDirectory);
		if (!m_CurrentDirectory)
			m_CurrentDirectory = &m_RootDirectory;
	}

	void ContentBrowserPanel::CacheDirectory(DirectoryEntry& directory, const std::filesystem::path& directoryPath)
	{
		uint32_t directoryIndex = 0;

		auto& childs = directory.ChildEntrys;
		for (auto&& entry : std::filesystem::directory_iterator(directoryPath))
		{
			
			if (entry.is_regular_file())
			{
				auto& childEntry = childs.emplace_back();
				childEntry.Type = EntryType::File;
				childEntry.Handle = ResourceManager::GetAssetHandleFromFilePath(entry.path());
				childEntry.Path = Project::RelativeCopy(entry.path());
				childEntry.DisplayName = entry.path().stem().string();
				continue;
			}

			SK_CORE_ASSERT(entry.is_directory());
			auto& childEntry = *childs.emplace(childs.begin() + directoryIndex++);
			childEntry.Type = EntryType::Directory;
			childEntry.Handle = 0;
			childEntry.Path = Project::RelativeCopy(entry.path());
			childEntry.DisplayName = entry.path().stem().string();

			CacheDirectory(childEntry, entry.path());
		}

	}

	DirectoryEntry* ContentBrowserPanel::GetEntry(const std::filesystem::path& path)
	{
		// Textures/Test.png

		if (path == m_RootDirectory.Path)
			return &m_RootDirectory;

		uint32_t depth = 1;
		uint32_t index = 0;
		DirectoryEntry* directory = &m_RootDirectory;
		while (index < directory->ChildEntrys.size())
		{
			auto& entry = directory->ChildEntrys[index++];

			// Entry is in current Directory
			if (entry.Path == path)
				return &entry;

			// Entry is in sub directory

			// Entry: Scenes
			// Path: Textures/Test.png

			auto i0 = std::next(entry.Path.begin(), depth);
			auto i1 = std::next(path.begin(), depth);

			if (*i0 == *i1)
			{
				directory = &entry;
				depth++;
				index = 0;
			}

		}

		return nullptr;
	}

	DirectoryEntry* ContentBrowserPanel::GetChildEntry(DirectoryEntry& directory, const std::filesystem::path& path)
	{
		for (auto& entry : directory.ChildEntrys)
		{
			if (entry.Path == path)
				return &entry;
		}

		//SK_CORE_ASSERT(false, "Entry not Found");
		return nullptr;
	}

	Ref<Texture2D> ContentBrowserPanel::GetCellIcon(const DirectoryEntry& entry)
	{
		if (entry.Type == EntryType::Directory)
			return m_FolderIcon;

		auto extension = entry.Path.extension();
		if (extension == L".skscene") return m_SceneIcon;
		if (extension == L".sktex") return m_TextureIcon;
		if (extension == L".png") return m_PNGIcon;

		return m_FileIcon;
	}

	void ContentBrowserPanel::ImportEntry(DirectoryEntry& entry)
	{
		entry.Handle = ResourceManager::ImportAsset(Project::AbsolueCopy(entry.Path));
	}

}