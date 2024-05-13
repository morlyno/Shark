#include "skfpch.h"
#include "ContentBrowserPanel.h"

#include "Shark/Event/ApplicationEvent.h"
#include "Shark/Asset/Assets.h"
#include "Shark/Asset/AssetUtils.h"
#include "Shark/Serialization/TextureSerializers.h"

#include "Shark/ImGui/TextFilter.h"

#include "Icons.h"
#include "EditorSettings.h"

#include "Shark/Debug/Profiler.h"

namespace Shark {

	static std::unordered_set<AssetType> s_SupportedThumbnailTypes = {
		AssetType::Texture,
		AssetType::Material,
		AssetType::Mesh,
		AssetType::MeshSource,
		AssetType::Scene,
		AssetType::Environment
	};

	ContentBrowserPanel::ContentBrowserPanel(const std::string& panelName)
		: Panel(panelName)
	{
		m_IconExtensionMap[".skscene"] = Icons::SceneIcon;
		m_IconExtensionMap[".sktex"] = Icons::TextureIcon;
		m_IconExtensionMap[".png"] = Icons::PNGIcon;
		m_IconExtensionMap[".jpg"] = Icons::JPGIcon;
		m_IconExtensionMap[".cs"] = Icons::ScriptIcon;
		m_FileIcon = Icons::FileIcon;
		m_FolderIcon = Icons::FolderIcon;

		memset(m_SearchBuffer, 0, sizeof(m_SearchBuffer));
	}

	ContentBrowserPanel::~ContentBrowserPanel()
	{
	}

	void ContentBrowserPanel::OnImGuiRender(bool& shown)
	{
		SK_PROFILE_FUNCTION();

		if (!shown)
			return;

		if (!m_Project)
			return;

		if (m_ReloadScheduled)
			Reload();

		if (m_ChangeDirectory)
		{
			ChangeDirectory(m_NextDirectory, m_AddNextToHistory);
			m_NextDirectory = nullptr;
			m_ChangeDirectory = false;
		}

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
		const bool open = ImGui::Begin(m_PanelName.c_str(), &shown, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		ImGui::PopStyleVar();

		if (open)
		{
			m_PanelFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);

			m_ChangesBlocked = true;
			const ImGuiStyle& style = ImGui::GetStyle();

			UI::ScopedStyle cellPadding(ImGuiStyleVar_CellPadding, { 0.0f, style.CellPadding.y });
			if (ImGui::BeginTable("##cbTable", 2, ImGuiTableFlags_Resizable))
			{
				{
					UI::ScopedStyle childRounding(ImGuiStyleVar_ChildRounding, 0);

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					if (ImGui::BeginChild("##treeView"))
					{
						UI::MoveCursorY(style.FramePadding.y);
						UI::ScopedIndent indent(style.WindowPadding.x * 0.5f);

						for (auto subdir : m_BaseDirectory->SubDirectories)
							DrawDirectoryHirachy(subdir);

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
				}


				ImGui::TableSetColumnIndex(1);
				UI::MoveCursor(style.WindowPadding);
				DrawHeader();
				//UI::MoveCursorY(style.ItemSpacing.y);
				ImGui::Separator();
				//UI::MoveCursorY(-style.ItemSpacing.y);

				UI::ScopedStyle childRounding(ImGuiStyleVar_ChildRounding, ImGui::GetCurrentWindowRead()->WindowRounding);
				if (ImGui::BeginChild("##contentView", ImVec2(0, 0), false, ImGuiChildFlags_AlwaysUseWindowPadding/* | ImGuiWindowFlags_ScrollbarrNoRoundTop*/))
				{
					const float padding = 8.0f;
					const float panelWidth = ImGui::GetContentRegionAvail().x;
					const float cellSize = EditorSettings::Get().ContentBrowser.ThumbnailSize + padding;
					int columnsCount = (int)(panelWidth / cellSize);
					columnsCount = std::max(columnsCount, 1);
					{
						UI::ScopedStyle rowSpacing(ImGuiStyleVar_ItemSpacing, { padding, 12.0f });
						ImGui::Columns(columnsCount, nullptr, false);
						DrawItems();
					}

					if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered())
					{
						if (m_SelectedItem)
							m_SelectedItem->Unselect();
						m_SelectedItem = nullptr;
					}

					if (ImGui::BeginPopupContextWindow("##DirectoryPopup", ImGuiMouseButton_Right | ImGuiPopupFlags_NoOpenOverItems))
					{
						if (ImGui::MenuItem("Open In Explorer"))
							Platform::OpenExplorer(m_Project->GetDirectory() / m_CurrentDirectory->Filepath);

						ImGui::Separator();

						if (ImGui::BeginMenu("New"))
						{
							if (ImGui::MenuItem("Directory"))
								CreateDirectory(m_CurrentDirectory, "New Directory", true);

							ImGui::Separator();

							if (ImGui::MenuItem("Scene"))
								CreateAsset<Scene>(m_CurrentDirectory, "New Scene.skscene", true);

							if (ImGui::MenuItem("Material"))
								CreateAsset<MaterialAsset>(m_CurrentDirectory, "New Material.skmat", true, Material::Create(Renderer::GetShaderLibrary()->Get("SharkPBR")));

							ImGui::EndMenu();
						}

						ImGui::EndPopup();
					}

					if (m_ShowInvalidFileNameError)
					{
						m_InvalidFileNameTimer -= ImGui::GetIO().DeltaTime;
						if (m_InvalidFileNameTimer <= 0.0f)
							m_ShowInvalidFileNameError = false;


						ImGui::SetNextWindowPos(m_InvalidFileNameToolTipTopLeft);
						ImGui::Begin("##invalidInput", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoFocusOnAppearing);
						ImGui::Text("Filenames aren't allowed to containe any of the folloing character!");
						ImGui::TextEx(FileSystem::InvalidCharacters.data(), FileSystem::InvalidCharacters.data() + FileSystem::InvalidCharacters.size());
						ImGui::End();
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
		dispacher.DispachEvent<KeyPressedEvent>(SK_BIND_EVENT_FN(ContentBrowserPanel::OnKeyPressedEvent));
	}

	void ContentBrowserPanel::OnProjectChanged(Ref<Project> project)
	{
		SK_CORE_ASSERT(!m_ChangesBlocked);

		m_Project = project;
		m_ReloadScheduled = false;
		m_BaseDirectory = nullptr;
		m_CurrentDirectory = nullptr;
		m_NextDirectory = nullptr;
		m_History.Reset(nullptr);
		m_CurrentItems.Clear();
		m_SelectedItem = nullptr;
		m_ThumbnailGenerator = nullptr;
		m_ThumbnailCache = nullptr;

		if (project)
		{
			m_ThumbnailGenerator = Ref<ThumbnailGenerator>::Create();
			m_ThumbnailCache = Ref<ThumbnailCache>::Create();

			m_BaseDirectory = Ref<DirectoryInfo>::Create(nullptr, m_Project->GetAssetsDirectory());
			m_CurrentDirectory = m_BaseDirectory;
			Reload();
		}

	}

	void ContentBrowserPanel::RegisterOpenAssetCallback(AssetType assetType, const OpenAssetCallbackFn& func)
	{
		m_OpenAssetCallbacks[assetType] = func;
	}

	Ref<Texture2D> ContentBrowserPanel::GetFileIcon(const std::filesystem::path& filepath) const
	{
		std::string extension = FileSystem::GetExtensionString(filepath);
		if (m_IconExtensionMap.contains(extension))
			return m_IconExtensionMap.at(extension);
		return m_FileIcon;
	}

	void ContentBrowserPanel::Reload()
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_ASSERT(!m_ChangesBlocked);

		ParseDirectories(m_BaseDirectory);
		m_History.Reset(m_BaseDirectory);
		CacheDirectories(m_BaseDirectory);

		auto currentDirectroy = GetDirectory(m_CurrentDirectory->Filepath);
		ChangeDirectory(currentDirectroy ? currentDirectroy : m_BaseDirectory, false);

		m_ReloadScheduled = false;
	}

	void ContentBrowserPanel::CacheDirectories(Ref<DirectoryInfo> directory)
	{
		const auto key = m_Project->GetAbsolute(directory->Filepath).lexically_normal();
		m_DirectoryMap[key] = directory;

		for (Ref<DirectoryInfo> subdir : directory->SubDirectories)
			CacheDirectories(subdir);
	}

	void ContentBrowserPanel::ParseDirectories(Ref<DirectoryInfo> directory)
	{
		SK_PROFILE_FUNCTION();

		directory->SubDirectories.clear();
		directory->Filenames.clear();

		for (const auto& entry : std::filesystem::directory_iterator(directory->Filepath))
		{
			if (entry.is_directory())
			{
				directory->SubDirectories.emplace_back(Ref<DirectoryInfo>::Create(directory, entry.path()));
				continue;
			}

			if (entry.is_regular_file())
			{
				if (!m_Project->GetEditorAssetManager()->IsFileImported(entry.path()))
					m_Project->GetEditorAssetManager()->ImportAsset(entry.path());

				directory->Filenames.emplace_back(entry.path().filename().string());
			}
		}

		std::ranges::sort(directory->Filenames);
		std::sort(directory->SubDirectories.begin(), directory->SubDirectories.end(), [](const auto& lhs, const auto& rhs)
		{
			return lhs->Filepath < rhs->Filepath;
		});

		for (auto subdir : directory->SubDirectories)
			ParseDirectories(subdir);

	}

	void ContentBrowserPanel::ChangeDirectory(Ref<DirectoryInfo> directory, bool addToHistory)
	{
		SK_CORE_ASSERT(!m_ChangesBlocked);

		if (IsSearchActive())
		{
			m_CurrentItems = Search(m_SearchBuffer, m_BaseDirectory);
			return;
		}

		if (!directory)
			return;

		m_CurrentDirectory = directory;
		m_CurrentItems = GetItemsInDirectory(directory);

		if (addToHistory)
		{
			m_History.Add(directory);
		}

		m_BreadcrumbTrailData.clear();
		Ref<DirectoryInfo> breadcrumb = directory;
		while (breadcrumb)
		{
			m_BreadcrumbTrailData.insert(m_BreadcrumbTrailData.begin(), breadcrumb);
			breadcrumb = breadcrumb->Parent.TryGetRef();
		}
	}

	void ContentBrowserPanel::NextDirectory(Ref<DirectoryInfo> directory, bool addToHistory, bool clearSearch)
	{
		if (clearSearch)
			memset(m_SearchBuffer, 0, sizeof(m_SearchBuffer));

		m_NextDirectory = directory;
		m_AddNextToHistory = addToHistory;
		m_ChangeDirectory = true;
	}

	void ContentBrowserPanel::GenerateThumbnails()
	{
		if (!EditorSettings::Get().ContentBrowser.GenerateThumbnails)
			return;

		for (const auto& currentItem : m_CurrentItems)
		{
			if (currentItem->GetType() != CBItemType::Asset)
				continue;

			AssetHandle assetHandle = currentItem->GetAssetHandle();
			if (!s_SupportedThumbnailTypes.contains(AssetManager::GetAssetType(assetHandle)))
				continue;

			if (!m_ThumbnailCache->IsThumbnailCurrent(assetHandle) && AssetManager::IsValidAssetHandle(assetHandle))
			{
				Ref<Image2D> thumbnail = m_ThumbnailGenerator->GenerateThumbnail(assetHandle);
				if (thumbnail)
					m_ThumbnailCache->SetThumbnail(assetHandle, thumbnail);

				break;
			}
		}

	}

	CBItemList ContentBrowserPanel::Search(const std::string& filterPaddern, Ref<DirectoryInfo> directory, bool searchSubdirectories)
	{
		CBItemList foundItems;
		UI::TextFilter filter(filterPaddern.c_str());
		filter.SetMode(String::Case::Ingnore);

		for (auto subdir : directory->SubDirectories)
		{
			std::string name = subdir->Name;
			if (filter.PassFilter(subdir->Name.c_str()))
			{
				foundItems.Add(Ref<ContentBrowserItem>::Create(this, CBItemType::Directory, subdir->Filepath));
			}

			if (searchSubdirectories)
			{
				CBItemList items = Search(filterPaddern, subdir);
				foundItems.MergeWith(items);
			}
		}

		for (const auto& filename : directory->Filenames)
		{
			Ref<EditorAssetManager> assetManager = m_Project->GetEditorAssetManager();
			std::filesystem::path filepath = directory->Filepath / filename;

			AssetHandle handle = assetManager->GetAssetHandleFromFilepath(filepath);
			if (!assetManager->IsValidAssetHandle(handle))
				continue;

			AssetType assetType = assetManager->GetAssetType(handle);
			std::string assetTypeString = ToString(assetType);

			if (filter.PassFilter(filename.c_str()) || filter.PassFilter(assetTypeString.c_str()))
			{
				foundItems.Add(Ref<ContentBrowserItem>::Create(this, CBItemType::Asset, filepath));
			}
		}

		return foundItems;
	}

	CBItemList ContentBrowserPanel::GetItemsInDirectory(Ref<DirectoryInfo> directory)
	{
		CBItemList items;

		for (Ref<DirectoryInfo> subdir : directory->SubDirectories)
		{
			Ref<ContentBrowserItem> directoryItem = Ref<ContentBrowserItem>::Create(this, CBItemType::Directory, subdir->Filepath);
			items.Items.push_back(directoryItem);
		}

		for (const auto& filename : directory->Filenames)
		{
			Ref<EditorAssetManager> assetManager = m_Project->GetEditorAssetManager();
			std::filesystem::path filepath = directory->Filepath / filename;
			AssetHandle handle = assetManager->GetAssetHandleFromFilepath(filepath);
			if (assetManager->IsValidAssetHandle(handle))
			{
				Ref<ContentBrowserItem> assetItem = Ref<ContentBrowserItem>::Create(this, CBItemType::Asset, filepath);
				items.Items.push_back(assetItem);
			}
		}

		return items;
	}

	Ref<DirectoryInfo> ContentBrowserPanel::GetDirectory(const std::filesystem::path& filePath)
	{
		const auto key = GetKey(filePath);
		SK_CORE_ASSERT(m_DirectoryMap.contains(key) == FileSystem::Exists(key));
		if (m_DirectoryMap.contains(key))
			return m_DirectoryMap.at(key);
		return nullptr;
	}

	std::filesystem::path ContentBrowserPanel::GetKey(const std::filesystem::path& path) const
	{
		return m_Project->GetAbsolute(path).lexically_normal();
	}

	bool ContentBrowserPanel::OnKeyPressedEvent(KeyPressedEvent& event)
	{
		if (!m_PanelFocused)
			return false;

		switch (event.GetKeyCode())
		{

			case KeyCode::Delete:
			{
				if (m_SelectedItem)
				{
					m_SelectedItem->Delete();
					return true;
				}
				break;
			}

			case KeyCode::F2:
			{
				if (m_SelectedItem)
				{
					m_SelectedItem->StartRenaming();
					return true;
				}
				break;
			}

		}

		return false;
	}

	void ContentBrowserPanel::DrawItems()
	{
		SK_PROFILE_FUNCTION();

		GenerateThumbnails();

		std::vector<Ref<ContentBrowserItem>> deletedItems;
		bool resortCurrentItems = false;

		for (Ref<ContentBrowserItem> currentItem : m_CurrentItems)
		{
			CBItemAction action = currentItem->Draw();
			const CBItemType itemType = currentItem->GetType();

			if (action.FlagSet(CBItemActionFlag::Open))
			{
				if (itemType == CBItemType::Directory)
				{
					Ref<DirectoryInfo> directoryInfo = GetDirectory(currentItem->GetPath());
					NextDirectory(directoryInfo);
				}

				if (itemType == CBItemType::Asset)
				{
					Ref<EditorAssetManager> assetManager = m_Project->GetEditorAssetManager();
					const AssetMetaData& metadata = assetManager->GetMetadata(currentItem->GetPath());
					if (assetManager->IsValidAssetHandle(metadata.Handle))
					{
						const auto& callback = m_OpenAssetCallbacks[metadata.Type];
						if (callback)
						{
							callback(metadata);
						}
					}
				}
			}

			if (action.FlagSet(CBItemActionFlag::RemoveItem))
			{
				deletedItems.push_back(currentItem);
			}

			if (action.FlagSet(CBItemActionFlag::OpenExternally))
			{
				if (itemType != CBItemType::Directory)
				{
					Platform::OpenFile(currentItem->GetPath());
				}
			}

			if (action.FlagSet(CBItemActionFlag::OpenInExplorer))
			{
				Ref<DirectoryInfo> directory = m_CurrentDirectory;
				if (itemType == CBItemType::Directory)
					directory = GetDirectory(currentItem->GetPath());

				Platform::OpenExplorer(m_Project->GetAbsolute(directory->Filepath));
			}

			if (action.FlagSet(CBItemActionFlag::Select))
			{
				if (m_SelectedItem)
					m_SelectedItem->Unselect();
				currentItem->Select();
				m_SelectedItem = currentItem;
			}

			if (action.FlagSet(CBItemActionFlag::Delete))
			{
				currentItem->Delete();
				deletedItems.push_back(currentItem);
			}

			if (action.FlagSet(CBItemActionFlag::ReloadAsset))
			{
				if (itemType == CBItemType::Asset)
				{
					Ref<EditorAssetManager> assetManager = m_Project->GetEditorAssetManager();
					AssetHandle assetHandle = assetManager->GetAssetHandleFromFilepath(currentItem->GetPath());
					if (assetManager->IsValidAssetHandle(assetHandle))
						assetManager->ReloadAssetAsync(assetHandle);
				}
			}

			if (action.FlagSet(CBItemActionFlag::StartRenaming))
			{
				currentItem->StartRenaming();
			}

			if (action.FlagSet(CBItemActionFlag::FinishedRenaming))
			{
				if (itemType == CBItemType::Directory)
				{
					Ref<DirectoryInfo> directoryInfo = GetDirectory(currentItem->GetPath());
					if (directoryInfo)
					{
						RenameDirectory(directoryInfo, action.GetNewName());
					}
				}
				if (itemType == CBItemType::Asset)
				{
					m_CurrentDirectory->RenameFile(currentItem->GetName(), action.GetNewName());
					m_Project->GetEditorAssetManager()->AssetRenamed(currentItem->GetAssetHandle(), action.GetNewName());
				}

				currentItem->Rename(action.GetNewName(), false);
				resortCurrentItems = true;
			}

			if (action.FlagSet(CBItemActionFlag::ImportFile))
			{
				Ref<EditorAssetManager> assetManager = m_Project->GetEditorAssetManager();
				assetManager->ImportAsset(currentItem->GetPath());
				currentItem->SetType(CBItemType::Asset);
			}

			if (action.FlagSet(CBItemActionFlag::AssetDropped))
			{
				SK_CORE_ASSERT(itemType == CBItemType::Directory);
				Ref<DirectoryInfo> destinationDirectory = GetDirectory(currentItem->GetPath());
				MoveAsset(action.GetDroppedAsset(), destinationDirectory);
			}

			if (action.FlagSet(CBItemActionFlag::DirectoryDropped))
			{
				SK_CORE_ASSERT(itemType == CBItemType::Directory);

				Ref<DirectoryInfo> destinationDirectory = GetDirectory(currentItem->GetPath());
				Ref<DirectoryInfo> directory = GetDirectory(action.GetDroppedDirectory());
				MoveDirectory(directory, destinationDirectory);
			}

			ImGui::NextColumn();
		}

		for (Ref<ContentBrowserItem> deleted : deletedItems)
		{
			Ref<DirectoryInfo> parent = GetDirectory(deleted->GetPath().parent_path());

			if (deleted->GetType() == CBItemType::Directory)
				parent->RemoveDirectory(deleted->GetPath());
			else
				parent->RemoveFile(deleted->GetPath().filename().string());

			m_CurrentItems.Remove(deleted);
		}

		if (resortCurrentItems)
			m_CurrentItems.Sort();

	}

	void ContentBrowserPanel::DrawHeader()
	{
		const ImGuiStyle& style = ImGui::GetStyle();
		ImGuiWindow* window = ImGui::GetCurrentWindow();

		//UI::ScopedClipRect clipRect(window->Rect());
		UI::ScopedStyle itemSpacing(ImGuiStyleVar_ItemSpacing, style.ItemSpacing * 0.5f);
		UI::MoveCursor({ -style.WindowPadding.x * 0.5f, -style.WindowPadding.y * 0.5f });


		{
			UI::ScopedDisabled searchActiveDisabled(IsSearchActive());
			UI::ScopedColorStack dark(ImGuiCol_Text, Theme::Colors::TextDark,
									  ImGuiCol_Button, Theme::Colors::ButtonDark,
									  ImGuiCol_ButtonActive, Theme::Colors::ButtonActiveDark,
									  ImGuiCol_ButtonHovered, Theme::Colors::ButtonHoveredDark);

			const ImVec2 buttonSize = { ImGui::GetFrameHeight(), ImGui::GetFrameHeight() };
			const ImVec2 buttonSizeNoFP = { ImGui::GetFontSize(), ImGui::GetFontSize() };

			// Move Back
			ImGui::BeginDisabled(!m_History.CanMoveBack());
			if (ImGui::Button("\xef\x84\x84", buttonSize))
			{
				NextDirectory(m_History.MoveBack(), false);
			}
			ImGui::EndDisabled();


			// Move Forward
			ImGui::SameLine();
			ImGui::BeginDisabled(!m_History.CanMoveForward());
			if (ImGui::Button("\xef\x84\x85", buttonSize))
			{
				NextDirectory(m_History.MoveForward(), false);
			}
			ImGui::EndDisabled();


			// Reload
			ImGui::SameLine();
			if (ImGui::Button("\xef\x80\xa1"))
				m_ReloadScheduled = true;

			ImGui::SameLine();
			if (UI::ImageButton("Clear Thumbnail Cache", Icons::ClearIcon, { buttonSizeNoFP }, ImGui::GetStyleColorVec4(ImGuiCol_Text)))
			{
				if (Input::IsKeyDown(KeyCode::LeftShift))
					m_ThumbnailCache->ClearDiscCache();
				m_ThumbnailCache->Clear();
			}

		}

		// Search
		ImGui::SameLine();
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.2f);
		if (UI::Search(UI::GenerateID(), m_SearchBuffer, SearchBufferSize))
		{
			NextDirectory(m_CurrentDirectory, false, false);

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

		ImGui::SameLine();
		for (uint32_t i = 0; i < m_BreadcrumbTrailData.size(); i++)
		{
			Ref<DirectoryInfo> breadcrumb = m_BreadcrumbTrailData[i];

			const auto& name = breadcrumb->Name;
			const ImVec2 buttonSize = ImGui::CalcTextSize(name.c_str(), name.c_str() + name.size());
			const ImVec2 pos = ImGui::GetCursorScreenPos();
			if (ImGui::InvisibleButton(breadcrumb->Name.c_str(), buttonSize))
				NextDirectory(breadcrumb);

			ImU32 textColor = ImGui::GetColorU32(ImGui::IsItemHovered() ? ImGuiCol_Text : ImGuiCol_TextDisabled);
			const ImVec2 textPos = { pos.x, pos.y + style.FramePadding.y };
			window->DrawList->AddText(textPos, textColor, name.c_str(), name.c_str() + name.size());

			if (i != m_BreadcrumbTrailData.size() - 1)
			{
				ImGui::SameLine(0.0f, 0.0f);
				ImGui::TextDisabled("/");
				ImGui::SameLine(0.0f, 0.0f);
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

			ImGui::Button("\xef\x80\x93");
			if (ImGui::IsItemClicked())
				ImGui::OpenPopup("cbSettings");
			if (ImGui::BeginPopup("cbSettings"))
			{
				auto& settings = EditorSettings::Get().ContentBrowser;
				ImGui::DragFloat("Thubnail Size", &settings.ThumbnailSize, 1.0f, 16.0f, FLT_MAX);
				ImGui::Checkbox("Generate Thumbnails", &settings.GenerateThumbnails);
				ImGui::EndPopup();
			}
		}
	}

	void ContentBrowserPanel::DrawDirectoryHirachy(Ref<DirectoryInfo> directory)
	{
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow;
		if (directory->SubDirectories.empty())
			flags |= ImGuiTreeNodeFlags_Leaf;

		bool open = false;

		const bool current = directory == m_CurrentDirectory;
		if (current)
			flags |= ImGuiTreeNodeFlags_Selected;

		{
			UI::ScopedColorConditional active(ImGuiCol_Header, Theme::Colors::Colored, true);
			open = ImGui::TreeNodeEx(directory->Name.c_str(), flags);
		}

		if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
			NextDirectory(directory);

		if (ImGui::BeginDragDropTarget())
		{
			const ImGuiPayload* assetPayload = ImGui::AcceptDragDropPayload(UI::DragDropID::Asset);
			if (assetPayload)
			{
				AssetHandle handle = *(AssetHandle*)assetPayload->Data;
				Ref<EditorAssetManager> assetManager = m_Project->GetEditorAssetManager();
				if (assetManager->IsValidAssetHandle(handle))
				{
					MoveAsset(handle, directory);
				}
			}

			const ImGuiPayload* directoryPayload = ImGui::AcceptDragDropPayload(UI::DragDropID::Directroy);
			if (directoryPayload)
			{
				std::filesystem::path path = std::filesystem::path((const char*)directoryPayload->Data);
				SK_CORE_ASSERT(FileSystem::Exists(path));

				MoveDirectory(GetDirectory(path), directory);
			}

		}

		if (open)
		{
			for (auto subdir : directory->SubDirectories)
				DrawDirectoryHirachy(subdir);

			ImGui::TreePop();
		}
	}

	CBItemType ContentBrowserPanel::GetItemTypeFromPath(const std::filesystem::path& path) const
	{
		if (std::filesystem::is_directory(path))
			return CBItemType::Directory;

		Ref<EditorAssetManager> assetManager = m_Project->GetEditorAssetManager();
		AssetHandle handle = assetManager->GetAssetHandleFromFilepath(path);
		if (assetManager->IsValidAssetHandle(handle))
			return CBItemType::Asset;
		return CBItemType::None;
	}

	void ContentBrowserPanel::MoveAsset(AssetHandle handle, Ref<DirectoryInfo> destinationDirectory)
	{
		Ref<EditorAssetManager> assetManager = m_Project->GetEditorAssetManager();

		auto filesystemPath = assetManager->GetFilesystemPath(handle);
		std::string filename = filesystemPath.filename().string();

		Ref<DirectoryInfo> originDirectory = GetDirectory(filesystemPath.parent_path());
		std::filesystem::path newPath = destinationDirectory->Filepath / filename;

		Ref<ContentBrowserItem> targetItem = m_CurrentItems.TryGet(filesystemPath);
		if ((targetItem && targetItem->Move(newPath)) || FileSystem::Move(targetItem->GetPath(), newPath))
		{
			originDirectory->RemoveFile(filename);
			destinationDirectory->AddFile(filename);
			assetManager->AssetMoved(handle, newPath);
			NextDirectory(m_CurrentDirectory, false, false);
		}
	}

	void ContentBrowserPanel::MoveDirectory(Ref<DirectoryInfo> directory, Ref<DirectoryInfo> destinationDirectory, bool first)
	{
		std::filesystem::path oldDirectoryPath = directory->Filepath;
		std::filesystem::path newDirectoryPath = destinationDirectory->Filepath / directory->Name;

		if (first)
		{
			if (!FileSystem::Move(oldDirectoryPath, newDirectoryPath))
				return;

			Ref<DirectoryInfo> oldParent = directory->Parent.GetRef();
			oldParent->RemoveDirectory(directory);
			destinationDirectory->AddDirectory(directory);
		}

		directory->Filepath = destinationDirectory->Filepath / directory->Name;
		directory->Parent = destinationDirectory;

		for (const auto& filename : directory->Filenames)
		{
			Ref<EditorAssetManager> assetManager = m_Project->GetEditorAssetManager();
			std::filesystem::path filesystemPath = oldDirectoryPath / filename;
			AssetHandle handle = assetManager->GetAssetHandleFromFilepath(filesystemPath);
			assetManager->AssetMoved(handle, newDirectoryPath / filename);
		}

		m_DirectoryMap.erase(GetKey(oldDirectoryPath));
		for (auto subdir : directory->SubDirectories)
		{
			MoveDirectory(subdir, directory, false);
		}

		if (first)
		{
			CacheDirectories(directory);
			NextDirectory(m_CurrentDirectory, false, false);
		}
	}

	void ContentBrowserPanel::RenameDirectory(Ref<DirectoryInfo> directory, const std::string& newName)
	{
		std::filesystem::path oldDirectoryPath = directory->Filepath;
		std::filesystem::path newDirectoryPath = FileSystem::ChangeFilename(directory->Filepath, newName);
		directory->Rename(newName);

		for (const auto& filename : directory->Filenames)
		{
			Ref<EditorAssetManager> assetManager = m_Project->GetEditorAssetManager();
			AssetHandle handle = assetManager->GetAssetHandleFromFilepath(oldDirectoryPath / filename);
			assetManager->AssetMoved(handle, newDirectoryPath / filename);
		}

		m_DirectoryMap.erase(GetKey(oldDirectoryPath));
		for (auto subdir : directory->SubDirectories)
		{
			MoveDirectory(subdir, directory, false);
		}

		CacheDirectories(directory);
		NextDirectory(m_CurrentDirectory, false, false);
	}

	Ref<ContentBrowserItem> ContentBrowserPanel::CreateDirectory(Ref<DirectoryInfo> directory, const std::string& name, bool startRenaming)
	{
		if (!FileSystem::IsValidFilename(name))
			return nullptr;

		std::filesystem::path directoryPath = m_Project->GetDirectory() / m_CurrentDirectory->Filepath / name;

		if (!FileSystem::CreateDirectories(directoryPath))
			return nullptr;

		Ref<DirectoryInfo> newDirectory = Ref<DirectoryInfo>::Create(directory, directory->Filepath / name);

		const auto key = m_Project->GetAbsolute(newDirectory->Filepath).lexically_normal();
		m_DirectoryMap[key] = newDirectory;
		m_CurrentDirectory->AddDirectory(newDirectory);

		Ref<ContentBrowserItem> item = Ref<ContentBrowserItem>::Create(this, CBItemType::Directory, newDirectory->Filepath);
		m_CurrentItems.Add(item);

		if (startRenaming)
			item->StartRenaming();

		return item;
	}

}
