#include "skfpch.h"
#include "ContentBrowserPanel.h"

#include "Shark/Event/ApplicationEvent.h"
#include "Shark/Editor/EditorSettings.h"
#include "Shark/Editor/Icons.h"

namespace Shark {

	static void DrawDebugTreeView(Ref<DirectoryInfo> directory)
	{
		std::string path = directory->FilePath.string();
		ImGui::Text("%s", path.c_str());
		for (auto subdir : directory->SubDirectories)
			DrawDebugTreeView(subdir);
	}

	ContentBrowserPanel::ContentBrowserPanel()
	{
		SK_CORE_ASSERT(!s_Instance);
		s_Instance = this;

		m_IconExtensionMap[".skscene"] = Icons::SceneIcon;
		m_IconExtensionMap[".sktex"] = Icons::TextureIcon;
		m_IconExtensionMap[".png"] = Icons::PNGIcon;
		m_IconExtensionMap[".cs"] = Icons::ScriptIcon;

		memset(m_SearchBuffer, 0, sizeof(m_SearchBuffer));
	}

	ContentBrowserPanel::~ContentBrowserPanel()
	{
		s_Instance = nullptr;
	}

	void ContentBrowserPanel::OnImGuiRender(bool& shown)
	{
		if (!shown)
			return;

		std::lock_guard guard(m_Mutex);

		for (auto& func : m_PostRenderQueue)
			func();
		m_PostRenderQueue.clear();

		CheckForProject();
		if (!m_Project)
			return;

		CheckForReload();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
		const bool open = ImGui::Begin("Contant Browser", &shown, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		ImGui::PopStyleVar();
		if (open)
		{
			m_PanelFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);

			m_ChangesBlocked = true;
			const ImGuiStyle& style = ImGui::GetStyle();

			UI::ScopedStyle cellPadding(ImGuiStyleVar_CellPadding, { 0.0f, style.CellPadding.y });
			if (ImGui::BeginTable("##cbTable", 2, ImGuiTableFlags_Resizable))
			{
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				if (ImGui::BeginChild("##treeView"))
				{
					UI::MoveCursorY(style.FramePadding.y);
					UI::ScopedIndent indent(style.WindowPadding.x * 0.5f);

					const bool open = ImGui::TreeNodeEx("Assets", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth);

					if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
						ChangeDirectory(m_BaseDirectory);

					if (open)
					{
						UI::ScopedIndent indent(-style.IndentSpacing * 0.75f);
						for (auto subdir : m_BaseDirectory->SubDirectories)
							DrawDirectoryHirachy(subdir);
						ImGui::TreePop();
					}

					ImGuiWindow* window = ImGui::GetCurrentWindow();
					ImDrawList* drawList = window->DrawList;
					const float shadowSize = 20.0f;
					const float rightX = window->Pos.x + window->Size.x;
					const ImVec2 topLeft = { rightX - shadowSize, window->WorkRect.Min.y };
					const ImVec2 bottemRight = { rightX, window->WorkRect.Max.y };
					drawList->AddRectFilledMultiColor(
						topLeft,
						bottemRight,
						IM_COL32(15, 15, 15, 0),
						IM_COL32(15, 15, 15, 190),
						IM_COL32(15, 15, 15, 190),
						IM_COL32(15, 15, 15, 0)
					);
				}
				ImGui::EndChild();

				ImGui::TableSetColumnIndex(1);
				UI::MoveCursor(style.WindowPadding);
				DrawHeader();
				UI::MoveCursorY(style.ItemSpacing.y);
				ImGui::Separator();
				UI::MoveCursorY(-style.ItemSpacing.y);
				if (ImGui::BeginChild("##contentView", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysUseWindowPadding))
				{
					const float padding = 8.0f;
					const float panelWidth = ImGui::GetContentRegionAvail().x;
					const float cellSize = EditorSettings::Get().ContentBrowserThumbnailSize + padding;
					int columnsCount = (int)(panelWidth / cellSize);
					columnsCount = std::max(columnsCount, 1);

					{
						UI::ScopedStyle rowSpacing(ImGuiStyleVar_ItemSpacing, { padding, 12.0f });
						ImGui::Columns(columnsCount, nullptr, false);
						DrawItems();
					}

					if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered())
						SelectItem(nullptr);

					if (ImGui::BeginPopupContextWindow("##DirectoryPopup", ImGuiMouseButton_Right, false))
					{
						if (ImGui::MenuItem("Open In Explorer"))
							PlatformUtils::OpenExplorer(m_Project->Directory / m_CurrentDirectory->FilePath);

						ImGui::Separator();

						if (ImGui::BeginMenu("New"))
						{
							if (ImGui::MenuItem("Directory"))
							{
								Ref<ContentBrowserItem> newItem = CreateDirectory("New Directory");
								SK_CORE_ASSERT(m_SelectedItem ? !m_SelectedItem->IsRenaming() : true);
								if (newItem)
									newItem->StartRenameing();
							}

							ImGui::Separator();

							if (ImGui::MenuItem("Scene"))
								CreateAsset<Scene>(m_CurrentDirectory, "New Scene.skscene", true);

							ImGui::EndMenu();
						}

						ImGui::EndPopup();
					}

				}
				ImGui::EndChild();

				ImGui::EndTable();
			}
			m_ChangesBlocked = false;
		}
		ImGui::End();

	}

	void ContentBrowserPanel::OnEvent(Event& event)
	{
		EventDispacher dispacher(event);
		dispacher.DispachEvent<ProjectChangedEvnet>([this](auto& e) { m_NextProject = e.GetProject(); return false; });
		dispacher.DispachEvent<KeyPressedEvent>(SK_BIND_EVENT_FN(ContentBrowserPanel::OnKeyPressedEvent));
	}

	void ContentBrowserPanel::OnFileEvents(const std::vector<FileChangedData>& fileEvents)
	{
		// Note(moro): when the new FileWatcher from the Scripting branch gets merged in
		//             locking the main thread is no loger necessary
		std::lock_guard guard(m_Mutex);
		SK_CORE_ASSERT(m_ChangesBlocked == false);

		if (m_SkipNextFileEvents)
		{
			SK_CORE_DEBUG("File Events Skiped");
			SK_CORE_INFO("{0}", fmt::join(fileEvents, "\n"));
			m_SkipNextFileEvents = false;
			return;
		}

		SK_CORE_DEBUG(SK_FUNCTION);
		SK_CORE_INFO("{0}", fmt::join(fileEvents, "\n"));

		bool changed = false;

		for (uint32_t i = 0; i < fileEvents.size(); i++)
		{
			const FileChangedData& event = fileEvents[i];
			if (event.Type == FileEvent::NewName || event.Type == FileEvent::Modified)
				continue;

			changed = true;
			const bool isDirectory = event.IsDirectory;

			switch (event.Type)
			{
				case FileEvent::Created:
				{
					if (isDirectory)
					{
						Ref<DirectoryInfo> parent = GetDirectory(event.FilePath.parent_path());
						SK_CORE_ASSERT(parent);
						auto path = std::filesystem::relative(event.FilePath, m_Project->Directory);
						Ref<DirectoryInfo> directory = Ref<DirectoryInfo>::Create(parent, path, AssetHandle::Generate());
						parent->AddDirectory(directory);
						m_DirectoryHandleMap[directory->Handle] = directory;
					}
					break;
				}
				case FileEvent::Deleted:
				{
					if (isDirectory)
					{
						Ref<DirectoryInfo> directory = GetDirectory(event.FilePath);
						if (directory)
							Internal_OnDirectoryDeleted(directory);
						break;
					}

					const auto& metadata = ResourceManager::GetMetaData(event.FilePath);
					if (metadata.IsValid())
					{
						Ref<DirectoryInfo> directory = GetDirectory(event.FilePath.parent_path());
						directory->Assets.erase(std::find(directory->Assets.begin(), directory->Assets.end(), metadata.Handle));

						auto item = m_CurrentItems.Get(metadata.Handle);
						Internal_OnItemDeleted(item);
					}
					break;
				}
				case FileEvent::OldName:
				{
					const auto& event2 = fileEvents[++i];
					SK_CORE_ASSERT(event2.Type == FileEvent::NewName);
					if (isDirectory)
					{
						Ref<DirectoryInfo> directory = GetDirectory(event.FilePath);
						if (directory)
						{
							if (auto item = m_CurrentItems.Get(directory->Handle))
								item->m_Name = event2.FilePath.stem().string();

							directory->FilePath = std::filesystem::relative(event2.FilePath, m_Project->Directory);
						}
						break;
					}

					const auto& metadata = ResourceManager::GetMetaData(event.FilePath);
					if (metadata.IsValid())
					{
						auto item = m_CurrentItems.Get(metadata.Handle);
						if (item)
							item->m_Name = event2.FilePath.stem().string();
					}

					break;
				}
			}

			Internal_ChangeDirectory(m_CurrentDirectory, true);
		}
	}

	bool ContentBrowserPanel::OnKeyPressedEvent(KeyPressedEvent& event)
	{
		if (!m_PanelFocused)
			return false;

		switch (event.GetKeyCode())
		{

			case Key::Delete:
			{
				if (m_SelectedItem)
				{
					m_SelectedItem->Delete();
					return true;
				}
			}

			case Key::F2:
			{
				if (m_SelectedItem)
				{
					m_SelectedItem->StartRenameing();
					return true;
				}
			}

		}

		return false;
	}

	void ContentBrowserPanel::Internal_ChangeDirectory(Ref<DirectoryInfo> directory, bool addToHistroy)
	{
		SK_CORE_ASSERT(!m_ChangesBlocked);

		if (IsSearchActive())
		{
			m_CurrentItems = Search(m_SearchBuffer);
		}
		else
		{
			m_CurrentItems.Clear();

			for (auto& subdir : directory->SubDirectories)
				m_CurrentItems.Items.emplace_back(Ref<ContentBrowserItem>::Create(CBItemType::Directory, subdir->Handle, subdir->FilePath.stem().string(), Icons::FolderIcon));

			for (auto assetHandle : directory->Assets)
			{
				const auto& metadata = ResourceManager::GetMetaData(assetHandle);
				if (metadata.IsValid())
					m_CurrentItems.Items.emplace_back(Ref<ContentBrowserItem>::Create(CBItemType::Asset, metadata.Handle, FileSystem::ParseFileName(metadata.FilePath), GetThumbnail(metadata)));
			}

			if (addToHistroy && directory != m_CurrentDirectory)
			{
				uint32_t insertIndex = m_HistoryIndex + 1;
				if (insertIndex != (uint32_t)m_History.size())
					m_History.erase(m_History.begin() + insertIndex, m_History.end());

				SK_CORE_ASSERT(insertIndex == m_History.size());
				m_History.emplace_back(directory);
				m_HistoryIndex = insertIndex;
			}

			m_CurrentDirectory = directory;
		}

		std::sort(m_CurrentItems.begin(), m_CurrentItems.end(), [](const auto& lhs, const auto& rhs)
		{
			if (lhs->GetType() == rhs->GetType())
				return lhs->GetName() < rhs->GetName();
			return lhs->GetType() == CBItemType::Directory;
		});

		if (m_SelectedItem)
		{
			m_SelectedItem->SetSelected(false);
			m_SelectedItem = nullptr;
		}

	}

	void ContentBrowserPanel::Internal_MoveForward()
	{
		SK_CORE_ASSERT(!m_ChangesBlocked);

		uint32_t nextIndex = m_HistoryIndex + 1;
		if (nextIndex >= m_History.size())
			return;

		Internal_ChangeDirectory(m_History[nextIndex], false);
		m_HistoryIndex = nextIndex;
	}

	void ContentBrowserPanel::Internal_MoveBackward()
	{
		SK_CORE_ASSERT(!m_ChangesBlocked);

		if (m_HistoryIndex == 0)
			return;

		Internal_ChangeDirectory(m_History[--m_HistoryIndex], false);
	}

	void ContentBrowserPanel::Internal_OnItemDeleted(Ref<ContentBrowserItem> item)
	{
		SK_CORE_ASSERT(!m_ChangesBlocked);

		if (m_CurrentItems.Contains(item->GetHandle()))
			m_CurrentItems.Erase(item->GetHandle());

		if (m_SelectedItem == item)
			SelectItem(nullptr);

		if (auto directory = GetDirectory(item))
			Internal_OnDirectoryDeleted(directory);
	}

	void ContentBrowserPanel::Internal_OnDirectoryDeleted(Ref<DirectoryInfo> directory)
	{
		SK_CORE_ASSERT(!m_ChangesBlocked);

		Ref<DirectoryInfo> parent = directory->Parent;
		SK_CORE_ASSERT(parent);
		parent->SubDirectories.erase(std::find(parent->SubDirectories.begin(), parent->SubDirectories.end(), directory));
		m_DirectoryHandleMap.erase(directory->Handle);
	}

	void ContentBrowserPanel::ChangeDirectory(Ref<DirectoryInfo> directory, bool addToHistroy)
	{
		Ref<ContentBrowserPanel> instance = this;
		m_PostRenderQueue.emplace_back([instance, directory, addToHistroy]
		{
			instance->Internal_ChangeDirectory(directory, addToHistroy);
		});
	}

	void ContentBrowserPanel::MoveForward()
	{
		Ref<ContentBrowserPanel> instance = this;
		m_PostRenderQueue.emplace_back([instance]
		{
			instance->Internal_MoveForward();
		});
	}

	void ContentBrowserPanel::MoveBackward()
	{
		Ref<ContentBrowserPanel> instance = this;
		m_PostRenderQueue.emplace_back([instance]
		{
			instance->Internal_MoveBackward();
		});
	}

	void ContentBrowserPanel::OnItemSelcted(Ref<ContentBrowserItem> item)
	{
		if (item == m_SelectedItem)
			return;

		if (m_SelectedItem)
			m_SelectedItem->SetSelected(false);
		m_SelectedItem = item;
		SK_CORE_ASSERT(item ? item->IsSelected() : true);
	}

	void ContentBrowserPanel::SelectItem(Ref<ContentBrowserItem> item)
	{
		if (item)
			item->SetSelected(true);
		OnItemSelcted(item);
	}

	void ContentBrowserPanel::DrawItems()
	{
		for (Ref<ContentBrowserItem> item : m_CurrentItems)
		{
			CBItemAction::Flags action = item->Draw();

			if (action & CBItemAction::Open && item->GetType() == CBItemType::Directory)
				ChangeDirectory(GetDirectory(item));

			if (action & CBItemAction::OpenInExplorer)
			{
				Ref<DirectoryInfo> directory = GetDirectory(item);
				if (!directory)
					directory = m_CurrentDirectory;

				PlatformUtils::OpenExplorer(m_Project->Directory / directory->FilePath);
			}

			if (action & CBItemAction::Selected)
				OnItemSelcted(item);

			if (action & CBItemAction::ReloadRequired)
				Reload();

			if (action & CBItemAction::ReloadAsset)
				ResourceManager::ReloadAsset(item->GetHandle());

			ImGui::NextColumn();
		}
	}

	void ContentBrowserPanel::DrawHeader()
	{
		const ImGuiStyle& style = ImGui::GetStyle();
		ImGuiWindow* window = ImGui::GetCurrentWindow();

		//UI::ScopedClipRect clipRect(window->Rect());
		UI::ScopedStyle itemSpacing(ImGuiStyleVar_ItemSpacing, style.ItemSpacing * 0.5f);
		UI::MoveCursor({ -style.WindowPadding.x * 0.5f, -style.WindowPadding.y * 0.5f });

		const ImVec2 buttonSize = { ImGui::GetFrameHeight(), ImGui::GetFrameHeight() };


		ImGui::BeginDisabled(IsSearchActive());

		ImGui::BeginDisabled(m_HistoryIndex == 0);
		if (ImGui::ButtonEx("<", buttonSize))
			MoveBackward();
		ImGui::EndDisabled();

		ImGui::SameLine();

		ImGui::BeginDisabled(m_HistoryIndex == (uint32_t)m_History.size() - 1);
		if (ImGui::Button(">", buttonSize))
			MoveForward();
		ImGui::EndDisabled();

		ImGui::EndDisabled();

		ImGui::SameLine();
		if (ImGui::Button("Reload"))
			m_ReloadScheduled = true;

		// Search
		ImGui::SameLine();
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.2f);
		//if (ImGui::InputTextWithHint("##search", "Search ...", m_SearchBuffer, SearchBufferSize))
		if (UI::Search(UI::GenerateID(), m_SearchBuffer, SearchBufferSize))
		{
			ChangeDirectory(m_CurrentDirectory, false);

			m_SearchCaseSensitive = false;
			for (auto& c : m_SearchBuffer)
			{
				if (isupper(c))
				{
					m_SearchCaseSensitive = true;
					break;
				}
			}
		}

		{
			ImGui::SameLine();
			UI::ScopedStyle itemSpacing(ImGuiStyleVar_ItemSpacing, { 0.0f, style.ItemSpacing.y });
			for (auto iter = m_CurrentDirectory->FilePath.begin(); iter != m_CurrentDirectory->FilePath.end(); iter++)
			{
				std::string text = iter->string();
				ImVec2 size = ImGui::CalcTextSize(text.c_str());
				ImGuiID id = ImGui::GetID(text.c_str());

				const ImVec2 topLeft = ImGui::GetCursorScreenPos();
				const ImRect rect{ topLeft, topLeft + size + ImVec2{ 0.0f, style.FramePadding.y * 2.0f } };

				ImGui::ItemSize(rect);
				if (!ImGui::ItemAdd(rect, id))
					continue;

				bool hovered, held;
				bool pressed = ImGui::ButtonBehavior(rect, id, &hovered, &held);

				ImDrawList* drawList = ImGui::GetWindowDrawList();
				const ImU32 textColor = hovered ? ImGui::GetColorU32(ImGuiCol_Text) : ImGui::GetColorU32(ImGuiCol_TextDisabled);
				drawList->AddText(topLeft + ImVec2{ 0.0f, style.FramePadding.y }, textColor, text.c_str());

				auto prevEnd = m_CurrentDirectory->FilePath.end();
				--prevEnd;
				if (iter != prevEnd)
				{
					ImGui::SameLine();
					ImGui::TextDisabled("/");
					ImGui::SameLine();
				}

				if (pressed)
				{
					std::filesystem::path newPath;
					for (auto i = m_CurrentDirectory->FilePath.begin(); i != iter; i++)
						newPath /= *i;
					newPath /= *iter;

					ChangeDirectory(GetDirectory(newPath));
				}
			}
		}

		{
			UI::ScopedStyle frameBorder(ImGuiStyleVar_FrameBorderSize, 0.0f);
			UI::ScopedColorStack colors(
				ImGuiCol_Button, ImVec4{ 0, 0, 0, 0 },
				ImGuiCol_ButtonHovered, ImVec4{ 0, 0, 0, 0 },
				ImGuiCol_ButtonActive, ImVec4{ 0, 0, 0, 0 }
			);

			const float iconSize = ImGui::GetFontSize();
			const float settingsIconOffset = ImGui::GetContentRegionAvail().x - iconSize - style.WindowPadding.x - style.ItemSpacing.x;
			ImGui::SameLine(settingsIconOffset);
			ImGui::ImageButton(Icons::SettingsIcon->GetViewID(), { iconSize, iconSize });
			if (ImGui::IsItemClicked())
				ImGui::OpenPopup("cbSettings");
			if (ImGui::BeginPopup("cbSettings"))
			{
				ImGui::DragFloat("Thubnail Size", &EditorSettings::Get().ContentBrowserThumbnailSize, 1.0f, 16.0f, FLT_MAX);
				ImGui::EndPopup();
			}
		}
	}

	void ContentBrowserPanel::DrawDirectoryHirachy(Ref<DirectoryInfo> directory)
	{
		std::string name = directory->FilePath.filename().string();
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow;
		if (directory->SubDirectories.empty())
			flags |= ImGuiTreeNodeFlags_Leaf;

		const bool open = ImGui::TreeNode(Icons::FolderIcon->GetViewID(), name.c_str(), flags);

		if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
			ChangeDirectory(directory);

		if (open)
		{
			for (auto subdir : directory->SubDirectories)
				DrawDirectoryHirachy(subdir);

			ImGui::TreePop();
		}
	}

	CBItemList ContentBrowserPanel::Search(const std::string& filter)
	{
		CBItemList foundItems;
		Search({ filter, m_SearchCaseSensitive }, m_BaseDirectory, foundItems.Items);
		return foundItems;
	}

	void ContentBrowserPanel::Search(const CBFilter& filter, Ref<DirectoryInfo> directory, std::vector<Ref<ContentBrowserItem>>& foundItmes)
	{
		for (auto subdir : directory->SubDirectories)
		{
			std::string str = subdir->FilePath.string();
			if (filter.Filter(str))
				foundItmes.emplace_back(Ref<ContentBrowserItem>::Create(subdir));

			Search(filter, subdir, foundItmes);
		}

		for (auto handle : directory->Assets)
		{
			const auto& metadata = ResourceManager::GetMetaData(handle);
			std::string str = metadata.FilePath.string();
			if (filter.Filter(str))
				foundItmes.emplace_back(Ref<ContentBrowserItem>::Create(CBItemType::Asset, handle, metadata.FilePath.stem().string(), GetThumbnail(metadata)));
		}

	}

	void ContentBrowserPanel::CheckForProject()
	{
		SK_CORE_ASSERT(!m_ChangesBlocked);

		if (m_NextProject)
		{
			m_Project = m_NextProject;
			m_NextProject = nullptr;
			m_ReloadScheduled = false;

			m_BaseDirectory = Ref<DirectoryInfo>::Create(std::filesystem::relative(m_Project->AssetsDirectory, m_Project->Directory), AssetHandle::Generate());
			
			Internal_ChangeDirectory(m_BaseDirectory, true);
			CacheDirectoryHandles();
		}
	}

	void ContentBrowserPanel::CheckForReload()
	{
		SK_CORE_ASSERT(!m_ChangesBlocked);

		if (m_ReloadScheduled)
		{
			m_BaseDirectory->Reload();
			Internal_ChangeDirectory(m_BaseDirectory, true);
			CacheDirectoryHandles();
			m_History.clear();
			m_History.emplace_back(m_BaseDirectory);
			m_HistoryIndex = 0;

			m_ReloadScheduled = false;
		}
	}

	void ContentBrowserPanel::CacheDirectoryHandles()
	{
		SK_CORE_ASSERT(!m_ChangesBlocked);

		m_DirectoryHandleMap.clear();
		CacheDirectoryHandles(m_BaseDirectory);
	}

	void ContentBrowserPanel::CacheDirectoryHandles(Ref<DirectoryInfo> directory)
	{
		SK_CORE_ASSERT(!m_ChangesBlocked);

		m_DirectoryHandleMap[directory->Handle] = directory;
		for (auto subdir: directory->SubDirectories)
			CacheDirectoryHandles(subdir);
	}

	void ContentBrowserPanel::ClearHistroy()
	{
		m_History.clear();
		m_HistoryIndex = -1;
	}

	Ref<Image2D> ContentBrowserPanel::GetIcon(const AssetMetaData& metadata)
	{
		std::string extension = metadata.FilePath.extension().string();
		if (m_IconExtensionMap.find(extension) != m_IconExtensionMap.end())
			return m_IconExtensionMap.at(extension);
		return Icons::FileIcon;
	}

	Ref<Image2D> ContentBrowserPanel::GetThumbnail(const AssetMetaData& metadata)
	{
		if (!metadata.IsValid())
			Icons::FileIcon;

		if (metadata.Type == AssetType::Texture || metadata.Type == AssetType::TextureSource)
			return Image2D::Create(ResourceManager::GetFileSystemPath(metadata));

		return GetIcon(metadata);
	}

	Ref<DirectoryInfo> ContentBrowserPanel::GetDirectory(const std::filesystem::path& filePath)
	{
		if (filePath.empty())
			return nullptr;

		if (filePath.is_absolute())
			return GetDirectory(std::filesystem::relative(filePath, m_Project->Directory));

		if (filePath == m_BaseDirectory->FilePath)
			return m_BaseDirectory;

		uint32_t depth = 1;
		uint32_t index = 0;
		Ref<DirectoryInfo> directory = m_BaseDirectory;
		while (index < directory->SubDirectories.size())
		{
			auto dir = directory->SubDirectories[index++];
			if (dir->FilePath == filePath)
				return dir;

			auto i0 = std::next(dir->FilePath.begin(), depth);
			auto i1 = std::next(filePath.begin(), depth);
			if (*i0 == *i1)
			{
				directory = dir;
				depth++;
				index = 0;
			}
		}
		return nullptr;
	}

	Ref<DirectoryInfo> ContentBrowserPanel::GetDirectory(Ref<ContentBrowserItem> item)
	{
		if (m_DirectoryHandleMap.find(item->GetHandle()) != m_DirectoryHandleMap.end())
			return m_DirectoryHandleMap.at(item->GetHandle());
		return nullptr;
	}

	Ref<ContentBrowserItem> ContentBrowserPanel::CreateDirectory(const std::string& name)
	{
		if (name.find_first_of("\\/:*?\"<>|") != std::string::npos)
			return nullptr;

		m_SkipNextFileEvents = true;
		std::error_code errorCode;
		std::filesystem::create_directory(m_Project->Directory / m_CurrentDirectory->FilePath / name, errorCode);
		if (errorCode)
		{
			m_SkipNextFileEvents = false;
			SK_CORE_ERROR("[ContentBrowserPanel] Failed to create Directory (Path: {0})", m_Project->Directory / m_CurrentDirectory->FilePath / name);
			SK_CORE_ERROR("[ContentBrowserPanel] Reason: {0}", errorCode.message());
			return nullptr;
		}

		AssetHandle directoryHandle = AssetHandle::Generate();
		Ref<DirectoryInfo> directory = DirectoryInfo::Create(m_CurrentDirectory, name, directoryHandle);
		m_DirectoryHandleMap[directoryHandle] = directory;
		m_CurrentDirectory->AddDirectory(directory);

		Ref<ContentBrowserItem> item = Ref<ContentBrowserItem>::Create(CBItemType::Directory, directoryHandle, name, Icons::FolderIcon);
		m_CurrentItems.Add(item);
		return item;
	}

}
