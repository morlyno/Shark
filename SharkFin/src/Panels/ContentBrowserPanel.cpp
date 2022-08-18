#include "skfpch.h"
#include "ContentBrowserPanel.h"

#include "Shark/Core/Project.h"
#include "Shark/Editor/Icons.h"

#include "Shark/Utils/PlatformUtils.h"
#include "Shark/Debug/Instrumentor.h"

#include <misc/cpp/imgui_stdlib.h>

namespace Shark {

	ContentBrowserPanel::ContentBrowserPanel()
	{
		SK_PROFILE_FUNCTION();

		if (Project::GetActive())
			CacheDirectory(Project::AssetsPath());
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
			CacheDirectory(Project::AssetsPath());
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
		DrawSettingsBar();
		DrawDragDropTooltip();

		ImGui::End();
	}

	void ContentBrowserPanel::OnEvent(Event& event)
	{
		SK_PROFILE_FUNCTION();

		EventDispacher dispacher(event);
		dispacher.DispachEvent<ProjectChangedEvnet>([this](ProjectChangedEvnet& event) { Reload(); return true; });
	}

	void ContentBrowserPanel::OnFileChanged(const std::vector<FileChangedData>& fileEvents)
	{
		SK_PROFILE_FUNCTION();

		if (m_SkipNextFileEvents)
		{
			m_SkipNextFileEvents = false;
			return;
		}

		Reload();
	}

	void ContentBrowserPanel::DrawTreeView()
	{
		SK_PROFILE_FUNCTION();

		const ImGuiStyle& style = ImGui::GetStyle();
		//ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, style.IndentSpacing * 0.5f);
		DrawTreeView(m_RootDirectory);
		//ImGui::PopStyleVar();
	}

	void ContentBrowserPanel::DrawTreeView(DirectoryEntry& directory)
	{
		SK_PROFILE_FUNCTION();

		for (auto& entry : directory.ChildEntrys)
		{
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
		SK_PROFILE_FUNCTION();

		const int cells = std::clamp((int)(ImGui::GetContentRegionAvail().x / m_CellWidth), 1, 64);
		const ImVec2 tableSize = ImGui::GetContentRegionAvail() - ImVec2{ 0.0f, ImGui::GetFrameHeightWithSpacing() };

		if (ImGui::BeginTable("CellView", cells, ImGuiTableFlags_ScrollY, tableSize))
		{
			ImGui::TableNextRow(0, m_CellHeight);

			if (m_CurrentDirectory)
			{
				for (auto& entry : m_CurrentDirectory->ChildEntrys)
				{
					ImGui::TableNextColumn(0, m_CellHeight);

					UI::PushID(entry.DisplayName);

					if (&entry != m_RenameContext.Entry)
					{
						const bool isSelectedEntry = &entry == m_SelectedEntry;
						ImVec2 cursorPos = ImGui::GetCursorPos();
						ImGui::Selectable("##Dummy", isSelectedEntry, 0, { m_CellWidth, m_CellHeight });
						const bool isHovered = ImGui::IsItemHovered();

						if (isSelectedEntry)
							m_IsSelectedHovered = isHovered;

						if (isHovered)
						{
							if (!m_IgnoreSelection && (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseDown(ImGuiMouseButton_Right)))
								m_SelectedEntry = &entry;
							if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
							{
								if (entry.Type == EntryType::Directory)
									m_CurrentDirectory = m_SelectedEntry;
								else if (entry.Type == EntryType::File)
									m_OpenFileCallback(entry.Path);
							}
						}

						if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceNoPreviewTooltip))
						{
							if (entry.Handle.IsValid())
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
					}

					DrawCell(entry);
					UI::PopID();
				}

				if (m_CreateEntryContext.Show)
				{
					ImGui::TableNextColumn(0, m_CellHeight);
					DrawCreateEntryCell();
				}
				
			}
			ImGui::EndTable();
		}
	}

	void ContentBrowserPanel::DrawCell(DirectoryEntry& entry)
	{
		SK_PROFILE_FUNCTION();

		Ref<Image2D> icon = GetCellIcon(entry);

		const float imageSize = m_CellWidth;
		ImGui::Image(icon->GetViewID(), { imageSize, imageSize });

		if (&entry == m_RenameContext.Entry)
		{
			const ImGuiStyle& style = ImGui::GetStyle();
			ImGui::SetNextItemWidth(m_CellWidth - style.FramePadding.x * 2.0f);
			ImGui::InputText("##Rename", &m_RenameContext.Buffer, ImGuiInputTextFlags_AutoSelectAll);
			if (!ImGui::IsItemActive())
				ImGui::SetKeyboardFocusHere(-1);

			if (ImGui::IsItemDeactivatedAfterEdit())
				OnRenameFinished(false);
			else if (ImGui::IsItemDeactivated())
				ClearRenameContext();
		}
		else
		{
			UI::Text(entry.DisplayName);
		}

		if (entry.Type != EntryType::Directory)
		{
			auto& metaData = ResourceManager::GetMetaData(entry.Handle);
			UI::Text(AssetTypeToString(metaData.Type));
		}
	}

	void ContentBrowserPanel::DrawCreateEntryCell()
	{
		SK_PROFILE_FUNCTION();

		Ref<Image2D> cellIcon = m_CreateEntryContext.Type == EntryType::Directory ? Icons::FolderIcon : Icons::FileIcon;

		const float imageSize = m_CellWidth;
		ImGui::Image(cellIcon->GetViewID(), { imageSize, imageSize });

		const ImGuiStyle& style = ImGui::GetStyle();
		ImGui::SetNextItemWidth(m_CellWidth - style.FramePadding.x * 2.0f);
		ImGui::InputText("##NewEntryName", &m_CreateEntryContext.Buffer, ImGuiInputTextFlags_AutoSelectAll);
		if (!ImGui::IsItemActive())
			ImGui::SetKeyboardFocusHere(-1);

		if (ImGui::IsItemDeactivated())
		{
			OnCreateEntryFinished();
			m_CreateEntryContext.Clear();
		}

		if (m_CreateEntryContext.Type != EntryType::Directory)
			ImGui::Text("New Entry");
	}

	void ContentBrowserPanel::DrawMenuBar()
	{
		SK_PROFILE_FUNCTION();

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
			{
				const auto end = std::next(elem);
				auto i = m_CurrentDirectory->Path.begin();
				for (; i != end; i++)
					newPath /= *i;
			}

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
	}

	void ContentBrowserPanel::DrawSettingsBar()
	{
		SK_PROFILE_FUNCTION();

		const char* text = "Cell Width";
		const ImGuiStyle& style = ImGui::GetStyle();
		const float sliderWidth = ImGui::GetContentRegionAvail().x * 0.2f;
		const float sliderOffset = ImGui::GetContentRegionAvail().x - sliderWidth; // SliderSize: 20% of ContentRegion
		const ImVec2 textSize = ImGui::CalcTextSize(text);
		
		ImGui::Indent(sliderOffset - (textSize.x + style.ItemSpacing.x));
		ImGui::AlignTextToFramePadding();
		ImGui::Text(text);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(sliderWidth);
		if (ImGui::SliderFloat("##CellWidth", &m_CellWidth, 80.0f, 200.0f))
		{
			m_CellHeight = m_CellWidth + 2.0f * ImGui::GetFrameHeight();
		}
	}

	void ContentBrowserPanel::DrawPopups()
	{
		SK_PROFILE_FUNCTION();

		if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows) &&
			ImGui::IsMouseReleased(ImGuiMouseButton_Right))
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

			if (ImGui::MenuItem("Reload", nullptr, false, m_SelectedEntry->Handle.IsValid()))
				ResourceManager::ReloadAsset(m_SelectedEntry->Handle);

			if (ImGui::MenuItem("Import", nullptr, false, m_SelectedEntry->Type == EntryType::File && !m_SelectedEntry->Handle.IsValid()))
				ImportEntry(*m_SelectedEntry);

			ImGui::Separator();
			if (ImGui::MenuItem("Rename"))
			{
				SK_CORE_ASSERT(!m_RenameContext.Entry, "Should never happen");
				m_RenameContext.Entry = m_SelectedEntry;
				m_RenameContext.Buffer = m_SelectedEntry->Path.stem().string();
				m_RenameContext.FirstFrame = true;
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Open"))           PlatformUtils::OpenFile(Project::AbsolueCopy(m_SelectedEntry->Path));
			if (ImGui::MenuItem("Open With"))      PlatformUtils::OpenFileWith(Project::AbsolueCopy(m_SelectedEntry->Path));
			if (ImGui::MenuItem("Open Explorer"))  PlatformUtils::OpenExplorer(Project::AbsolueCopy(m_CurrentDirectory->Path));

			ImGui::Separator();

			if (ImGui::MenuItem("Delete"))
				DeleteSelectedEntry();

			ImGui::EndPopup();
		}

		if (ImGui::BeginPopup("Directory Settings"))
		{
			m_IgnoreSelection = true;

			if (ImGui::BeginMenu("New"))
			{
				if (ImGui::MenuItem("Directory"))
				{
					m_CreateEntryContext.Buffer = "New Directory";
					m_CreateEntryContext.Extentsion.clear();
					m_CreateEntryContext.Type = EntryType::Directory;
					m_CreateEntryContext.Show = true;
				}

				ImGui::Separator();

				if (ImGui::MenuItem("Scene"))
				{
					m_CreateEntryContext.Buffer = "New Scene.skscene";
					m_CreateEntryContext.Extentsion = ".skscene";
					m_CreateEntryContext.Type = EntryType::File;
					m_CreateEntryContext.Show = true;
				}

				if (ImGui::MenuItem("Script"))
				{
					m_CreateEntryContext.Buffer = "New Script.cs";
					m_CreateEntryContext.Extentsion = ".cs";
					m_CreateEntryContext.Type = EntryType::File;
					m_CreateEntryContext.Show = true;
				}
			
				ImGui::EndMenu();
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Open Explorer")) PlatformUtils::OpenExplorer(Project::AbsolueCopy(m_CurrentDirectory->Path));

			ImGui::EndPopup();
		}

		if (m_ShowExtensionChangedWarning)
		{
			ImGui::OpenPopup("Extension Changed");
			m_ShowExtensionChangedWarning = false;
		}
		if (ImGui::BeginPopupModal("Extension Changed"))
		{
			ImGui::Text("Are you shure to change the type of this File");
			if (ImGui::Button("Cancle"))
			{
				ClearRenameContext();
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

	}

	void ContentBrowserPanel::DrawDragDropTooltip()
	{
		SK_PROFILE_FUNCTION();

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
		SK_PROFILE_FUNCTION();

		std::filesystem::path selectedEntry;
		std::filesystem::path currentDirectory;
		std::filesystem::path dragDropEntry;
		if (m_SelectedEntry) selectedEntry = m_SelectedEntry->Path;
		if (m_CurrentDirectory) currentDirectory = m_CurrentDirectory->Path;
		if (m_DragDropEntry) dragDropEntry = m_DragDropEntry->Path;

		m_RootDirectory.ChildEntrys.clear();
		m_RootDirectory.Type = EntryType::Directory;
		m_RootDirectory.Handle = AssetHandle::Invalid;
		m_RootDirectory.Path = Project::RelativeCopy(rootPath);
		CacheDirectory(m_RootDirectory, rootPath);
		m_CurrentDirectory = &m_RootDirectory;

		m_SelectedEntry = GetEntry(selectedEntry);
		m_CurrentDirectory = GetEntry(currentDirectory);
		m_DragDropEntry = GetEntry(dragDropEntry);
		if (!m_CurrentDirectory)
			m_CurrentDirectory = &m_RootDirectory;
	}

	void ContentBrowserPanel::CacheDirectory(DirectoryEntry& directory, const std::filesystem::path& directoryPath)
	{
		SK_PROFILE_FUNCTION();

		uint32_t directoryIndex = 0;

		auto& childs = directory.ChildEntrys;
		for (auto&& entry : std::filesystem::directory_iterator(directoryPath))
		{
			if (ShouldItemBeIgnored(entry.path()))
				continue;

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
		SK_PROFILE_FUNCTION();

		// Textures/Test.png

		if (path.empty())
			return nullptr;

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
		SK_PROFILE_FUNCTION();

		for (auto& entry : directory.ChildEntrys)
		{
			if (entry.Path == path)
				return &entry;
		}

		//SK_CORE_ASSERT(false, "Entry not Found");
		return nullptr;
	}

	Shark::DirectoryEntry* ContentBrowserPanel::GetParentEntry(const DirectoryEntry& entry)
	{
		SK_PROFILE_FUNCTION();

		return GetEntry(entry.Path.parent_path());
	}

	Ref<Image2D> ContentBrowserPanel::GetCellIcon(const DirectoryEntry& entry)
	{
		SK_PROFILE_FUNCTION();

		if (entry.Type == EntryType::Directory)
			return Icons::FolderIcon;

		auto extension = entry.Path.extension();
		if (extension == L".skscene") return Icons::SceneIcon;
		if (extension == L".sktex") return Icons::TextureIcon;
		if (extension == L".png") return Icons::PNGIcon;
		if (extension == L".cs") return Icons::ScriptIcon;

		return Icons::FileIcon;
	}

	void ContentBrowserPanel::ImportEntry(DirectoryEntry& entry)
	{
		SK_PROFILE_FUNCTION();

		entry.Handle = ResourceManager::ImportAsset(Project::AbsolueCopy(entry.Path));
	}

	bool ContentBrowserPanel::ShouldItemBeIgnored(const std::filesystem::path& filePath)
	{
		SK_PROFILE_FUNCTION();

		if (!filePath.has_extension())
			return false;

		const std::wstring fileName = filePath.filename().native();

		if (fileName.find(L".csproj"sv) != std::wstring::npos) return true;

		return false;
	}

	void ContentBrowserPanel::OnRenameFinished(bool canChangeExtension)
	{
		SK_PROFILE_FUNCTION();

		DirectoryEntry& entry = *m_RenameContext.Entry;
		std::filesystem::path oldPath = Project::AbsolueCopy(entry.Path);

		auto newPath = oldPath;
		newPath.replace_filename(m_RenameContext.Buffer);
		newPath.replace_extension(oldPath.extension());

		SK_CORE_ASSERT(newPath.extension() == oldPath.extension());

		// Check for overlapping names
		if (std::filesystem::exists(newPath))
		{
			SK_CORE_WARN("Can't rename file to allready exisiting name");
			ClearRenameContext();
			return;
		}

		std::error_code err;
		std::filesystem::rename(oldPath, newPath, err);
		SK_CORE_ASSERT(!err, err.message());

		ClearRenameContext();
	}

	void ContentBrowserPanel::ClearRenameContext()
	{
		SK_PROFILE_FUNCTION();

		m_RenameContext.Entry = nullptr;
		m_RenameContext.Buffer.clear();
		m_RenameContext.FirstFrame = true;
	}

	void ContentBrowserPanel::DeleteSelectedEntry()
	{
		SK_PROFILE_FUNCTION();

		auto& entry = *m_SelectedEntry;
		auto fsPath = Project::AbsolueCopy(entry.Path);
		
		m_SkipNextFileEvents = true;

		std::error_code err;
		std::filesystem::remove_all(fsPath, err);
		SK_CORE_ASSERT(!err, err.message());

		// Note(moro): if m_SelectedEntry is set throu the tree the Parent Entry isn't m_CurrentDirectory
		DirectoryEntry* parentEntry = GetParentEntry(entry);
		// in this context parentEntry should never be null
		SK_CORE_ASSERT(parentEntry);

		auto& childEntries = parentEntry->ChildEntrys;
		const auto it = std::find_if(childEntries.begin(), childEntries.end(), [&path = entry.Path](auto& entry) { return path == entry.Path; });
		// in this context it should be an end iterator
		SK_CORE_ASSERT(it != childEntries.end());

		childEntries.erase(it);
		m_SelectedEntry = nullptr;
	}

	void ContentBrowserPanel::OnCreateEntryFinished()
	{
		SK_PROFILE_FUNCTION();

		auto fsPath = Project::AbsolueCopy(m_CurrentDirectory->Path / m_CreateEntryContext.Buffer);
		
		if (m_CreateEntryContext.Type == EntryType::Directory)
		{
			if (!std::filesystem::exists(fsPath))
			{
				std::error_code err;
				std::filesystem::create_directory(fsPath, err);
				SK_CORE_ASSERT(!err, err.message());
			}
			return;
		}

		
		std::string extension = fsPath.extension().string();
		if (extension.empty())
		{
			if (m_CreateEntryContext.Extentsion.empty())
				return;

			fsPath.replace_extension(m_CreateEntryContext.Extentsion);
			extension = m_CreateEntryContext.Extentsion;
		}

		if (extension == ".cs")
		{
			std::string scriptName = fsPath.stem().string();
			FileSystem::CreateScriptFile(Project::AbsolueCopy(m_CurrentDirectory->Path), Project::Name(), scriptName);
			PlatformUtils::RunProjectSetupSilent();
			return;
		}

		if (AssetExtensionMap.find(extension) == AssetExtensionMap.end())
		{
			// probaly a utilty file like .txt
			FileSystem::MakeFreeFilePath(fsPath);
			PlatformUtils::CreateFile(fsPath, false);
			return;
		}

		AssetType assetType = AssetExtensionMap.at(extension);
		std::string directoryPath = fsPath.parent_path().string();
		std::string fileName = fsPath.filename().string();
		switch (assetType)
		{
			case AssetType::Scene: ResourceManager::CreateAsset<Scene>(directoryPath, fileName); break;
			case AssetType::Texture: ResourceManager::CreateAsset<Texture2D>(directoryPath, fileName); break;
		}

	}

}