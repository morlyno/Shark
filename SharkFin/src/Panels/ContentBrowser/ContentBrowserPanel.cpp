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

						const bool open = ImGui::TreeNodeEx("Assets", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth);

						if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
							NextDirectory(m_BaseDirectory);

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

		if (m_GenerateThumbnailsFuture.valid() && m_GenerateThumbnailsFuture.wait_for(0s) == std::future_status::ready)
		{
			m_GenerateThumbnailsFuture = std::future<void>{};
		}
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
		m_StopGenerateThumbnails = true;
		m_GenerateThumbnailsFuture = {};

		if (project)
		{
			m_BaseDirectory = Ref<DirectoryInfo>::Create(nullptr, m_Project->GetAssetsDirectory());
			m_CurrentDirectory = m_BaseDirectory;
			m_BaseDirectory->Reload(m_Project);
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
		SK_CORE_ASSERT(!m_ChangesBlocked);

		m_BaseDirectory->Reload(m_Project);
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

		if (m_BrowserType == ContentBrowserType::Asset)
		{
			m_CurrentItems = GetItemsForAssetBrowser(directory);
		}
		else
		{
			m_CurrentItems = GetItemsForFileBrowser(directory);
		}

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

		GenerateThumbnails();
	}

	void ContentBrowserPanel::NextDirectory(Ref<DirectoryInfo> directory, bool addToHistory, bool clearSearch)
	{
		if (clearSearch)
			memset(m_SearchBuffer, 0, sizeof(m_SearchBuffer));

		m_NextDirectory = directory;
		m_AddNextToHistory = addToHistory;
		m_ChangeDirectory = true;
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
			if (assetManager->IsValidAssetHandle(handle))
			{
				AssetType assetType = assetManager->GetAssetType(handle);
				std::string assetTypeString = ToString(assetType);

				if (filter.PassFilter(filename.c_str()) || filter.PassFilter(assetTypeString.c_str()))
				{
					foundItems.Add(Ref<ContentBrowserItem>::Create(this, CBItemType::Asset, filepath));
				}
				continue;
			}

			if (m_BrowserType == ContentBrowserType::Filesystem && filter.PassFilter(filename.c_str()))
			{
				foundItems.Add(Ref<ContentBrowserItem>::Create(this, CBItemType::File, filepath));
			}
		}

		return foundItems;
	}

	CBItemList ContentBrowserPanel::GetItemsForAssetBrowser(Ref<DirectoryInfo> directory)
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

	CBItemList ContentBrowserPanel::GetItemsForFileBrowser(Ref<DirectoryInfo> directory)
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
			const AssetHandle handle = assetManager->GetAssetHandleFromFilepath(filepath);
			const CBItemType itemType = assetManager->IsValidAssetHandle(handle) ? CBItemType::Asset : CBItemType::File;
			Ref<ContentBrowserItem> assetItem = Ref<ContentBrowserItem>::Create(this, itemType, filepath);
			items.Items.push_back(assetItem);
		}

		return items;
	}

	Ref<DirectoryInfo> ContentBrowserPanel::GetDirectory(const std::filesystem::path& filePath)
	{
		if (m_DirectoryMap.contains(filePath))
			return m_DirectoryMap.at(filePath);
		return nullptr;
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
					{
						Application::Get().SubmitToMainThread([assetManager, assetHandle]()
						{
							assetManager->ReloadAsset(assetHandle);
						});
					}
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
						directoryInfo->Rename(action.GetNewName());
					}
				}
				if (itemType == CBItemType::Asset)
				{
					m_CurrentDirectory->RenameFile(currentItem->GetName(), action.GetNewName());
					m_Project->GetEditorAssetManager()->OnAssetRenamed(currentItem->GetPath(), action.GetNewName());
				}
				if (itemType == CBItemType::File)
				{
					m_CurrentDirectory->RenameFile(currentItem->GetName(), action.GetNewName());
				}

				currentItem->Rename(action.GetNewName());
				resortCurrentItems = true;
			}

			if (action.FlagSet(CBItemActionFlag::Remove))
			{
				if (itemType == CBItemType::Asset)
				{
					Ref<EditorAssetManager> assetManager = m_Project->GetEditorAssetManager();
					AssetHandle handle = assetManager->GetAssetHandleFromFilepath(currentItem->GetPath());
					assetManager->RemoveAsset(handle);
					currentItem->SetType(CBItemType::File);
				}
			}

			if (action.FlagSet(CBItemActionFlag::ImportFile))
			{
				Ref<EditorAssetManager> assetManager = m_Project->GetEditorAssetManager();
				assetManager->ImportAsset(currentItem->GetPath());
				currentItem->SetType(CBItemType::Asset);
			}

			ImGui::NextColumn();
		}

		for (Ref<ContentBrowserItem> deleted : deletedItems)
			m_CurrentItems.Remove(deleted);

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
			//UI::ScopedFont fontAwesome("FontAwesome");
			const ImVec2 buttonSize = { ImGui::GetFrameHeight(), ImGui::GetFrameHeight() };

			ImGui::BeginDisabled(!m_History.CanMoveBack());
			if (ImGui::ButtonEx("\xef\x84\x84", buttonSize))
			{
				NextDirectory(m_History.MoveBack(), false);
			}
			ImGui::EndDisabled();

			ImGui::SameLine();

			ImGui::BeginDisabled(!m_History.CanMoveForward());
			if (ImGui::Button("\xef\x84\x85", buttonSize))
			{
				NextDirectory(m_History.MoveForward(), false);
			}
			ImGui::EndDisabled();
		}

		ImGui::SameLine();
		if (ImGui::Button("Reload"))
			m_ReloadScheduled = true;

		// Search
		ImGui::SameLine();
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.2f);
		char backupBuffer[SearchBufferSize];
		memcpy(backupBuffer, m_SearchBuffer, sizeof(backupBuffer));
		if (UI::Search(UI::GenerateID(), m_SearchBuffer, SearchBufferSize))
		{
			NextDirectory(m_CurrentDirectory, false, false);
			SK_CORE_TRACE("{}", fmt::ptr(backupBuffer));

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
				if (ImGui::Combo("Type", (int*)&m_BrowserType, s_BrowserTypeString, std::size(s_BrowserTypeString)))
					NextDirectory(m_CurrentDirectory, false);

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

		const bool open = ImGui::TreeNodeEx(directory->Name.c_str(), flags);

		if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
			NextDirectory(directory);

		if (open)
		{
			for (auto subdir : directory->SubDirectories)
				DrawDirectoryHirachy(subdir);

			ImGui::TreePop();
		}
	}

	Ref<Texture2D> ContentBrowserPanel::GetIcon(const AssetMetaData& metadata)
	{
		std::string extension = metadata.FilePath.extension().string();
		if (m_IconExtensionMap.contains(extension))
			return m_IconExtensionMap.at(extension);
		return m_FileIcon;
	}

	Ref<Texture2D> ContentBrowserPanel::GetThumbnail(const AssetMetaData& metadata)
	{
		if (!metadata.IsValid())
			return m_FileIcon;

		Ref<EditorAssetManager> assetManager = m_Project->GetEditorAssetManager();

		switch (metadata.Type)
		{
			case AssetType::Font:
			{
				auto font = assetManager->GetAsset(metadata.Handle).As<Font>();
				return font->GetFontAtlas();
			}
			case AssetType::Texture:
			{
				return assetManager->GetAsset(metadata.Handle).As<Texture2D>();
			}
			case AssetType::TextureSource:
			{
				auto source = assetManager->GetAsset(metadata.Handle).As<TextureSource>();
				return Texture2D::Create({}, source);
			}
		}

		return GetIcon(metadata);
	}

	void ContentBrowserPanel::GenerateThumbnails()
	{
		SK_PROFILE_FUNCTION();

		SK_CORE_WARN_TAG("UI", "Generating Thumbnails");
		ScopedTimer timer("Generating Thumbnails");
		if (!EditorSettings::Get().ContentBrowser.GenerateThumbnails)
			return;

		m_StopGenerateThumbnails = false;

		auto currentItems = m_CurrentItems;
		for (Ref<ContentBrowserItem> item : currentItems)
		{
			if (m_StopGenerateThumbnails)
			{
				SK_CORE_WARN_TAG("UI", "Generating Thumbnails Stoped");
				break;
			}

			const auto& metadata = m_Project->GetEditorAssetManager()->GetMetadata(item->GetPath());
			if (metadata.Type == AssetType::Texture || metadata.Type == AssetType::TextureSource || metadata.Type == AssetType::Font)
			{
				SK_CORE_ASSERT(item->m_Thumbnail == nullptr);
				item->m_Thumbnail = GetThumbnail(metadata);
			}
		}

		m_StopGenerateThumbnails = false;
	}

	CBItemType ContentBrowserPanel::GetItemTypeFromPath(const std::filesystem::path& path) const
	{
		if (std::filesystem::is_directory(path))
			return CBItemType::Directory;

		Ref<EditorAssetManager> assetManager = m_Project->GetEditorAssetManager();
		AssetHandle handle = assetManager->GetAssetHandleFromFilepath(path);
		if (assetManager->IsValidAssetHandle(handle))
			return CBItemType::Asset;
		return CBItemType::File;
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
