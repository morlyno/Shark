#include "ContentBrowserPanel.h"

#include "Shark/Core/SelectionManager.h"
#include "Shark/Scene/Prefab.h"
#include "Shark/Asset/Assets.h"
#include "Shark/Asset/AssetUtils.h"
#include "Shark/Event/ApplicationEvent.h"

#include "Shark/UI/Widgets.h"
#include "Shark/UI/EditorResources.h"

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
		m_IconExtensionMap[".skscene"] = EditorResources::SceneIcon;
		m_IconExtensionMap[".sktex"] = EditorResources::TextureIcon;
		m_IconExtensionMap[".png"] = EditorResources::PNGIcon;
		m_IconExtensionMap[".jpg"] = EditorResources::JPGIcon;
		m_IconExtensionMap[".cs"] = EditorResources::ScriptIcon;

		memset(m_SearchBuffer, 0, sizeof(m_SearchBuffer));
	}

	ContentBrowserPanel::~ContentBrowserPanel()
	{
	}

	void ContentBrowserPanel::OnImGuiRender(bool& shown)
	{
		SK_PROFILE_FUNCTION();

		if (!m_ProjectConfig)
			return;

		if (m_ReloadScheduled)
			Reload();

		if (m_ChangeDirectory)
		{
			ChangeDirectory(m_NextDirectory, m_AddNextToHistory);
			m_NextDirectory = nullptr;
			m_ChangeDirectory = false;
		}

		HandleSelectionRequests();

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
					if (ImGui::BeginChild("##treeView", ImVec2(0, 0), ImGuiChildFlags_NavFlattened))
					{
						UI::ShiftCursorY(style.FramePadding.y);
						UI::ScopedIndent indent(style.WindowPadding.x * 0.5f);

						for (auto subdir : m_BaseDirectory->SubDirectories)
							DrawDirectoryHierarchy(subdir);

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
				UI::ShiftCursor(style.WindowPadding);
				DrawHeader();
				ImGui::Separator();

				UI::ScopedStyle childRounding(ImGuiStyleVar_ChildRounding, ImGui::GetCurrentWindowRead()->WindowRounding);
				if (ImGui::BeginChild("##contentView", ImVec2(0, 0), false, ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_NavFlattened/* | ImGuiWindowFlags_ScrollbarrNoRoundTop*/))
				{
					const float padding = 8.0f;
					const float panelWidth = ImGui::GetContentRegionAvail().x;
					const float cellSize = EditorSettings::Get().ContentBrowser.ThumbnailSize + padding;
					int columnsCount = (int)(panelWidth / cellSize);
					columnsCount = std::max(columnsCount, 1);
					{
						//UI::ScopedStyle rowSpacing(ImGuiStyleVar_ItemSpacing, { padding, 24.0f });
						UI::ScopedStyle rowSpacing(ImGuiStyleVar_CellPadding, { padding, padding });

						ImGuiMultiSelectFlags multiSelectFlags = /*ImGuiMultiSelectFlags_ClearOnEscape | */ImGuiMultiSelectFlags_ClearOnClickVoid | ImGuiMultiSelectFlags_BoxSelect2d | ImGuiMultiSelectFlags_NavWrapX;
						multiSelectFlags |= ImGuiMultiSelectFlags_NoBoxSelectOverItem;
						ImGuiMultiSelectIO* selectionRequests = ImGui::BeginMultiSelect(multiSelectFlags, (int)SelectionManager::GetSelections(m_SelectionID).size(), (int)m_CurrentItems.Items.size());
						ApplySelectionRequests(selectionRequests, true);
						if (ImGui::BeginTable("##content.table", columnsCount, ImGuiTableFlags_PadOuterX))
						{
							ImGui::TableNextRow();
							DrawItems();

							ImGui::EndTable();
						}
						selectionRequests = ImGui::EndMultiSelect();
						ApplySelectionRequests(selectionRequests, false);
					}

					if (ImGui::BeginPopupContextWindow("##DirectoryPopup", ImGuiMouseButton_Right | ImGuiPopupFlags_NoOpenOverItems))
					{
						if (ImGui::MenuItem("Open In Explorer"))
							Platform::Execute(ExecuteVerb::Explore, m_ProjectConfig->GetDirectory() / m_CurrentDirectory->Filepath);

						ImGui::Separator();

						if (ImGui::BeginMenu("New"))
						{
							if (ImGui::MenuItem("Directory"))
								CreateDirectory("New Directory", true);

							ImGui::Separator();

							if (ImGui::MenuItem("Scene"))
								CreateAsset<Scene>("New Scene.skscene", true);

							if (ImGui::MenuItem("Material"))
								CreateAsset<MaterialAsset>("New Material.skmat", true, Material::Create(Renderer::GetShaderLibrary()->Get("SharkPBR")));

							if (ImGui::MenuItem("Prefab"))
								CreateAsset<Prefab>("New Prefab.sfab", true);

							ImGui::EndMenu();
						}

						ImGui::EndPopup();
					}

				}
				ImGui::EndChild();

				if (ImGui::BeginDragDropTarget())
				{
					const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_ID");
					if (payload)
						HandleEntityPayload(payload, m_CurrentDirectory);
					ImGui::EndDragDropTarget();
				}

				ImGui::EndTable();
			}
			m_ChangesBlocked = false;
		}
		ImGui::End();

		const ImGuiStyle& style = ImGui::GetStyle();
		ImGui::SetNextWindowSize({ 500, { ImGui::GetFrameHeightWithSpacing() * 8 + UI::Fonts::Get("Large")->FontSize + style.FramePadding.y * 2.0f + style.ItemSpacing.y} }, ImGuiCond_Appearing);
		if (ImGui::BeginPopupModal(m_DeleteDialoguePopupID, "Delete Items", nullptr, ImGuiWindowFlags_NoSavedSettings))
		{
			{
				UI::ScopedFont title("Large");
				ImGui::Text("Deleting the following items");
			}

			if (ImGui::IsWindowAppearing())
			{
				m_DeleteDialogue.Items.clear();
				const auto& selections = SelectionManager::GetSelections(m_SelectionID);
				for (UUID id : selections)
					m_DeleteDialogue.Items.emplace_back(id, true);
				m_DeleteDialogue.SelectedCount = selections.size();
			}
			
			// { 500, ImGui::GetFrameHeightWithSpacing() * 5 }
			if (ImGui::BeginTable("##itemsList", 3, ImGuiTableFlags_ScrollY | ImGuiTableFlags_Borders | ImGuiTableFlags_NoSavedSettings, { ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing() - style.ItemSpacing.y }))
			{
				{
					UI::ScopedColorStack frameColors(ImGuiCol_FrameBg, IM_COL32_BLACK_TRANS,
													 ImGuiCol_FrameBgActive, IM_COL32_BLACK_TRANS,
													 ImGuiCol_FrameBgHovered, IM_COL32_BLACK_TRANS);
					UI::ScopedStyle borderSize(ImGuiStyleVar_FrameBorderSize, 0.0f);
					UI::ScopedStyle cellPadding(ImGuiStyleVar_CellPadding, { 0, 0 });

					ImGui::TableSetupColumn("##checkAll", ImGuiTableColumnFlags_WidthFixed, UI::Fonts::Get("Bold")->FontSize);
					ImGui::TableSetupColumn("Name");
					ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("MeshSource").x);
					ImGui::TableSetupScrollFreeze(0, 1);

					ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
					{
						bool temp = m_DeleteDialogue.SelectedCount == m_DeleteDialogue.Items.size();

						ImGui::TableSetColumnIndex(0);
						ImGui::PushID(0);
						if (ImGui::Checkbox("##checkAll", &temp))
						{
							// clear/set all
							for (auto& [id, selected] : m_DeleteDialogue.Items)
								selected = temp;
							m_DeleteDialogue.SelectedCount = temp ? m_DeleteDialogue.Items.size() : 0;
						}
						ImGui::PopID();
					}

					UI::Fonts::Push("Bold");
					for (int column = 1; column < 3; column++)
					{
						ImGui::TableSetColumnIndex(column);
						const char* columnName = ImGui::TableGetColumnName(column);
						ImGui::PushID(column);
						UI::ShiftCursorY(1);
						ImGui::TableHeader(columnName);
						ImGui::PopID();
					}
					UI::Fonts::Pop();

					struct Func
					{
						static void DisplayChildren(Ref<DirectoryInfo> directory, bool selected, const uint32_t depth, const float indent)
						{
							for (const auto& subdir : directory->SubDirectories)
							{
								UI::ScopedID scopedID(subdir);
								ImGui::TableNextRow();
								ImGui::TableSetColumnIndex(0);
								ImGui::BeginDisabled();
								bool dummy = selected;
								ImGui::Checkbox("##check", &dummy);
								ImGui::EndDisabled();

								ImGui::TableSetColumnIndex(1);
								UI::ShiftCursorX(indent * depth);
								const bool open = ImGui::TreeNodeEx(subdir->Name.c_str(), ImGuiTreeNodeFlags_SpanAllColumns | ImGuiTreeNodeFlags_OpenOnArrow);

								ImGui::TableSetColumnIndex(2);
								ImGui::Text("Directory");

								if (open)
								{
									DisplayChildren(subdir, selected, depth + 1, indent);
									ImGui::TreePop();
								}

							}

							for (const auto& filename : directory->Filenames)
							{
								UI::ScopedID scopedID(filename);
								ImGui::TableNextRow();
								ImGui::TableSetColumnIndex(0);
								ImGui::BeginDisabled();
								bool dummy = selected;
								ImGui::Checkbox("##check", &dummy);
								ImGui::EndDisabled();

								ImGui::TableSetColumnIndex(1);
								UI::ShiftCursorX(indent * depth);
								ImGui::TreeNodeEx(filename.c_str(), ImGuiTreeNodeFlags_SpanAllColumns | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_NoTreePushOnOpen);

								ImGui::TableSetColumnIndex(2);
								ImGui::Text("Asset");
							}
						}
					};

					for (auto& [id, selected] : m_DeleteDialogue.Items)
					{
						SK_CORE_VERIFY(m_CurrentItems.Contains(id));
						auto item = m_CurrentItems.Get(id);
						UI::ScopedID scopedID(id);

						ImGui::TableNextRow();
						ImGui::TableSetColumnIndex(0);
						if (ImGui::Checkbox("##check", &selected))
							m_DeleteDialogue.SelectedCount += selected ? 1 : -1;

						ImGui::TableSetColumnIndex(2);
						ImGui::Text(item->GetSpecificTypeName());

						ImGui::TableSetColumnIndex(1);
						if (item->GetType() == CBItemType::Asset)
						{
							if (ImGui::Selectable(item->GetName().c_str(), false, ImGuiSelectableFlags_SpanAllColumns))
							{
								selected = !selected;
								m_DeleteDialogue.SelectedCount += selected ? 1 : -1;
							}
						}
						else
						{
							const float indent = style.IndentSpacing;
							UI::ScopedStyle noIndent(ImGuiStyleVar_IndentSpacing, 0.0f);
							const bool open = ImGui::TreeNodeEx(item->GetName().c_str(), ImGuiTreeNodeFlags_SpanAllColumns | ImGuiTreeNodeFlags_OpenOnArrow);

							if (ImGui::IsItemActivated() && !ImGui::IsItemToggledOpen())
								selected = !selected;

							if (open)
							{
								Ref<DirectoryInfo> directory = GetDirectory(item->GetID());
								UI::ScopedColor text(ImGuiCol_Text, UI::Colors::Theme::TextDarker);
								Func::DisplayChildren(directory, selected, 1, indent);
								ImGui::TreePop();
							}
						}
					}
				}
				ImGui::EndTable();
			}

			ImVec2 size = { ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeightWithSpacing() };
			ImGui::BeginHorizontal("##buttonsHorizontal", size);
			ImGui::Spring();

			ImGui::SetNextItemShortcut(ImGuiKey_Escape);
			if (ImGui::Button("Cancel"))
				ImGui::CloseCurrentPopup();

			ImGui::BeginDisabled(m_DeleteDialogue.SelectedCount == 0);
			if (ImGui::Button("Delete"))
			{
				for (const auto& [id, selected] : m_DeleteDialogue.Items)
				{
					if (!selected)
						continue;

					auto item = m_CurrentItems.Get(id);
					item->Delete();
					m_CurrentItems.Remove(id);
					SelectionManager::Unselect(m_SelectionID, id);
				}

				CacheDirectories();
				NextDirectory(m_CurrentDirectory, false, false);
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndDisabled();
			ImGui::EndHorizontal();
			ImGui::EndPopup();
		}

		if (ImGui::BeginPopupModal(m_ErrorDialogueID, m_ErrorDialogue.Title.c_str()))
		{
			ImGui::Text(m_ErrorDialogue.Message);
			
			ImGui::BeginHorizontal("##responseHorizontal");
			ImGui::Spring();
			if ((m_ErrorDialogue.Response & ErrorResponse::OK) == ErrorResponse::OK && ImGui::Button("OK"))
				ImGui::CloseCurrentPopup();
			ImGui::EndHorizontal();

			ImGui::EndPopup();
		}

	}

	void ContentBrowserPanel::OnEvent(Event& event)
	{
		EventDispacher dispacher(event);
		dispacher.DispachEvent<KeyPressedEvent>(SK_BIND_EVENT_FN(ContentBrowserPanel::OnKeyPressedEvent));
	}

	void ContentBrowserPanel::OnProjectChanged(Ref<ProjectConfig> project)
	{
		SK_CORE_ASSERT(!m_ChangesBlocked);

		m_ProjectConfig = project;
		m_ReloadScheduled = false;
		m_BaseDirectory = nullptr;
		m_CurrentDirectory = nullptr;
		m_NextDirectory = nullptr;
		m_History.Reset(nullptr);
		m_CurrentItems.Clear();
		m_ThumbnailGenerator = nullptr;
		m_ThumbnailCache = nullptr;

		if (project)
		{
			m_ThumbnailGenerator = Ref<ThumbnailGenerator>::Create();
			m_ThumbnailCache = Ref<ThumbnailCache>::Create();

			m_BaseDirectory = Ref<DirectoryInfo>::Create(nullptr, m_ProjectConfig->GetAssetsDirectory());
			m_CurrentDirectory = m_BaseDirectory;
			Reload();
		}

	}

	void ContentBrowserPanel::RegisterAssetActicatedCallback(AssetType assetType, const AssetActivatedCallbackFn& func)
	{
		m_AssetActivatedCallbacks[assetType] = func;
	}

	void ContentBrowserPanel::Reload()
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_ASSERT(!m_ChangesBlocked);

		ParseDirectories(m_BaseDirectory);
		CacheDirectories();

		Ref<DirectoryInfo> activeDirectory = m_BaseDirectory;
		if (auto currentDirectroy = FindDirectory(m_CurrentDirectory->Filepath))
			activeDirectory = currentDirectroy;

		m_History.Reset(activeDirectory);
		ChangeDirectory(activeDirectory, false);

		m_ReloadScheduled = false;
	}

	void ContentBrowserPanel::CacheDirectories()
	{
		SK_CORE_VERIFY(m_BaseDirectory);
		m_DirectoryMap.clear();
		BuildDirectoryIDMap(m_BaseDirectory);
	}

	void ContentBrowserPanel::BuildDirectoryIDMap(Ref<DirectoryInfo> directory)
	{
		m_DirectoryMap[directory->ID] = directory;

		for (Ref<DirectoryInfo> subdir : directory->SubDirectories)
			BuildDirectoryIDMap(subdir);
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
				if (!Project::GetEditorAssetManager()->IsFileImported(entry.path()))
					Project::GetEditorAssetManager()->ImportAsset(entry.path());

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

		SelectionManager::DeselectAll(m_SelectionID);
		if (IsSearchActive())
		{
			m_CurrentItems = Search(m_SearchBuffer, m_BaseDirectory);
			return;
		}

		if (!directory)
			return;

		if (!FileSystem::Exists(directory->Filepath))
		{
			auto message = fmt::format("The directory no longer exists\n{}", directory->Filepath);
			ShowErrorDialogue(ErrorType::InvalidDirectory, message, ErrorResponse::OK);
			return;
		}

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

	void ContentBrowserPanel::SelectItem(UUID id, bool add)
	{
		if (!add)
		{
			m_ClearSelection = true;
			m_ItemsToSelect.clear();
		}

		m_ItemsToSelect.push_back(id);
		//ImGui::SetNavID(UI::GetIDWithSeed(id, 0), ImGuiNavLayer_Main, 0, ImRect{});
	}

	void ContentBrowserPanel::ShowErrorDialogue(ErrorType type, const std::string& message, ErrorResponse response)
	{
		ImGui::OpenPopup(m_ErrorDialogueID);
		m_ErrorDialogue.Type = type;
		m_ErrorDialogue.Message = message;
		m_ErrorDialogue.Response = response;

		switch (m_ErrorDialogue.Type)
		{
			case ErrorType::None: m_ErrorDialogue.Title = "Error"; break;
			case ErrorType::InvalidDirectory: m_ErrorDialogue.Title = "Invalid Directory"; break;
			case ErrorType::InvalidInput: m_ErrorDialogue.Title = "Invalid Input"; break;
			case ErrorType::OperationFailed: m_ErrorDialogue.Title = "Operation Failed"; break;
		}
	}

	void ContentBrowserPanel::GenerateThumbnails()
	{
		if (!EditorSettings::Get().ContentBrowser.GenerateThumbnails)
			return;

		for (const auto& currentItem : m_CurrentItems)
		{
			if (currentItem->GetType() != CBItemType::Asset)
				continue;

			AssetHandle assetHandle = currentItem->GetID();
			if (!s_SupportedThumbnailTypes.contains(AssetManager::GetAssetType(assetHandle)))
				continue;

			if (!m_ThumbnailCache->IsThumbnailCurrent(assetHandle) && AssetManager::IsValidAssetHandle(assetHandle))
			{

				if (!AssetManager::DependenciesLoaded(assetHandle, true))
					continue;

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
		filter.SetMode(String::Case::Ignore);

		for (auto subdir : directory->SubDirectories)
		{
			std::string name = subdir->Name;
			if (filter.PassesFilter(subdir->Name.c_str()))
			{
				foundItems.Add(Ref<ContentBrowserDirectory>::Create(this, subdir));
			}

			if (searchSubdirectories)
			{
				CBItemList items = Search(filterPaddern, subdir);
				foundItems.MergeWith(items);
			}
		}

		for (const auto& filename : directory->Filenames)
		{
			Ref<EditorAssetManager> assetManager = Project::GetEditorAssetManager();
			std::filesystem::path filepath = directory->Filepath / filename;

			const auto& metadata = assetManager->GetMetadata(filepath);
			if (!assetManager->IsValidAssetHandle(metadata.Handle))
				continue;

			AssetType assetType = assetManager->GetAssetType(metadata.Handle);
			std::string_view assetTypeString = magic_enum::enum_name(assetType);

			std::string text = fmt::format("{} {}", filename, assetTypeString);
			if (filter.PassesFilter(text) || filter.PassesFilter(text))
			{
				Ref<Texture2D> icon = GetAssetIcon(FileSystem::GetExtensionString(filepath));
				foundItems.Add(Ref<ContentBrowserAsset>::Create(this, metadata, icon));
			}
		}

		return foundItems;
	}

	CBItemList ContentBrowserPanel::GetItemsInDirectory(Ref<DirectoryInfo> directory)
	{
		CBItemList items;

		for (Ref<DirectoryInfo> subdir : directory->SubDirectories)
		{
			Ref<ContentBrowserItem> directoryItem = Ref<ContentBrowserDirectory>::Create(this, subdir);
			items.Items.push_back(directoryItem);
		}

		for (const auto& filename : directory->Filenames)
		{
			Ref<EditorAssetManager> assetManager = Project::GetEditorAssetManager();
			std::filesystem::path filepath = directory->Filepath / filename;
			const auto& metadata = assetManager->GetMetadata(filepath);
			if (assetManager->IsValidAssetHandle(metadata.Handle))
			{
				Ref<Texture2D> icon = GetAssetIcon(FileSystem::GetExtensionString(filepath));
				Ref<ContentBrowserItem> assetItem = Ref<ContentBrowserAsset>::Create(this, metadata, icon);
				items.Items.push_back(assetItem);
			}
		}

		return items;
	}

	Ref<DirectoryInfo> ContentBrowserPanel::GetDirectory(UUID id) const
	{
		if (m_DirectoryMap.contains(id))
			return m_DirectoryMap.at(id);
		return nullptr;
	}

	Ref<DirectoryInfo> ContentBrowserPanel::FindDirectory(const std::filesystem::path& filePath)
	{
		for (const auto& [id, info] : m_DirectoryMap)
		{
			if (info->Filepath == filePath)
				return info;
		}
		return nullptr;
	}

	bool ContentBrowserPanel::OnKeyPressedEvent(KeyPressedEvent& event)
	{
		if (!m_PanelFocused)
			return false;

		switch (event.GetKeyCode())
		{
			case KeyCode::Escape:
			{
				SelectionManager::DeselectAll(m_SelectionID);
				return true;
			}

#if TODO
			case KeyCode::Delete:
			{
				if (m_SelectedItem)
				{
					m_SelectedItem->Delete();
					return true;
				}
				break;
			}
#endif

#if 0
			case KeyCode::F2:
			{
				const auto& selections = SelectionManager::GetSelections(SelectionContext::ContentBrowser);
				if (selections.empty() || !m_CurrentItems.Contains(selections[0]))
					break;

				auto item = m_CurrentItems.Get(selections[0]);
				item->StartRenaming();
				break;
			}
#endif
		}

		return false;
	}

	void ContentBrowserPanel::DrawItems()
	{
		SK_PROFILE_FUNCTION();

		GenerateThumbnails();

		bool resortCurrentItems = false;

		uint32_t index = 0;
		for (Ref<ContentBrowserItem> currentItem : m_CurrentItems)
		{
			ImGui::TableNextColumn();

			ImGui::SetNextItemSelectionUserData(index++);
			CBItemAction action = currentItem->Draw();
			const CBItemType itemType = currentItem->GetType();

			if (action.IsSet(CBItemActionFlag::Activated))
			{
				if (itemType == CBItemType::Directory)
				{
					Ref<DirectoryInfo> directoryInfo = GetDirectory(currentItem->GetID());
					NextDirectory(directoryInfo);
				}

				if (itemType == CBItemType::Asset)
				{
					Ref<EditorAssetManager> assetManager = Project::GetEditorAssetManager();
					AssetHandle assetHandle = currentItem->GetID();
					if (assetManager->IsValidAssetHandle(assetHandle))
					{
						const auto& metadata = assetManager->GetMetadata(assetHandle);
						if (m_AssetActivatedCallbacks.contains(metadata.Type))
							m_AssetActivatedCallbacks.at(metadata.Type)(metadata);
					}
				}
			}

			if (action.IsSet(CBItemActionFlag::OpenExternal))
			{
				if (itemType == CBItemType::Asset)
				{
					std::filesystem::path filepath = Project::GetEditorAssetManager()->GetFilesystemPath(currentItem->GetID());
					Platform::Execute(ExecuteVerb::Run, filepath);
				}
			}

			if (action.IsSet(CBItemActionFlag::ShowInExplorer))
			{
				Ref<DirectoryInfo> directory = m_CurrentDirectory;
				if (itemType == CBItemType::Directory)
					directory = GetDirectory(currentItem->GetID());
				else if (IsSearchActive())
				{
					auto filesystemPath = Project::GetEditorAssetManager()->GetFilesystemPath(currentItem->GetID());
					directory = FindDirectory(FileSystem::GetParent(filesystemPath));
				}

				SK_CORE_VERIFY(directory);
				Platform::Execute(ExecuteVerb::Explore, m_ProjectConfig->GetAbsolute(directory->Filepath));
			}

			if (action.IsSet(CBItemActionFlag::Reload) && itemType == CBItemType::Asset)
			{
				Ref<EditorAssetManager> assetManager = Project::GetEditorAssetManager();
				AssetHandle assetHandle = currentItem->GetID();
				if (assetManager->IsValidAssetHandle(assetHandle))
					assetManager->ReloadAssetAsync(assetHandle);
			}

			if (action.IsSet(CBItemActionFlag::StartRenaming))
			{
				currentItem->StartRenaming();
			}

			if (action.IsSet(CBItemActionFlag::Renamed))
			{
				const auto& selections = SelectionManager::GetSelections(m_SelectionID);
				if (selections.size() > 1)
				{
					for (UUID id : selections)
					{
						if (!m_CurrentItems.Contains(id))
							continue;

						auto item = m_CurrentItems.Get(id);
						if (item == currentItem)
							continue;

						item->Rename(currentItem->GetDisplayName());
					}
				}

				if (currentItem->GetType() == CBItemType::Directory && IsSearchActive())
				{
					SK_CORE_VERIFY(false, "i don't think this is necessary");
					NextDirectory(m_CurrentDirectory, false, false);
				}

				resortCurrentItems = true;
			}

			if (action.IsSet(CBItemActionFlag::OpenDeleteDialogue))
			{
				ImGui::OpenPopup(m_DeleteDialoguePopupID);
			}

			if (action.IsSet(CBItemActionFlag::DropAccepted))
			{
				SK_CORE_VERIFY(currentItem->GetType() == CBItemType::Directory);

				const ImGuiPayload* payload = ImGui::GetDragDropPayload();
				if (payload->IsDataType("Asset"))
				{
					AssetHandle handle = *(AssetHandle*)payload->Data;
					if (AssetManager::IsValidAssetHandle(handle))
					{
						SK_CORE_VERIFY(m_CurrentItems.Contains(handle));
						auto item = m_CurrentItems.Get(handle);
						auto tagetDirectory = GetDirectory(currentItem->GetID());

						item->Move(tagetDirectory);
						NextDirectory(m_CurrentDirectory, false, false);
					}
				}

				if (payload->IsDataType("uuid.cb.directory"))
				{
					UUID id = *(UUID*)payload->Data;
					SK_CORE_VERIFY(m_CurrentItems.Contains(id));

					auto item = m_CurrentItems.Get(id);
					auto tagetDirectory = GetDirectory(currentItem->GetID());
					item->Move(tagetDirectory);
					NextDirectory(m_CurrentDirectory, false, false);
				}

				if (payload->IsDataType("ENTITY_ID"))
				{
					auto targetDirectory = GetDirectory(currentItem->GetID());
					HandleEntityPayload(payload, targetDirectory);
				}
			}

		}

		if (resortCurrentItems)
			m_CurrentItems.Sort();

	}

	void ContentBrowserPanel::DrawHeader()
	{
		UI::ScopedFont font("Medium");

		const ImGuiStyle& style = ImGui::GetStyle();
		ImGuiWindow* window = ImGui::GetCurrentWindow();

		//UI::ScopedClipRect clipRect(window->Rect());
		UI::ScopedStyle itemSpacing(ImGuiStyleVar_ItemSpacing, style.ItemSpacing * 0.5f);
		UI::ShiftCursor({ -style.WindowPadding.x * 0.5f, -style.WindowPadding.y * 0.5f });

		ImGui::BeginHorizontal("##header.horizontal", { ImGui::GetContentRegionAvail().x - style.WindowPadding.x * 2.0f, ImGui::GetFrameHeightWithSpacing() });
		{
			//UI::ScopedDisabled searchActiveDisabled(IsSearchActive());
			UI::ScopedColorStack dark(ImGuiCol_Text, UI::Colors::Theme::TextDarker,
									  ImGuiCol_Button, UI::Colors::Theme::ButtonDark,
									  ImGuiCol_ButtonActive, UI::Colors::Theme::ButtonActiveDark,
									  ImGuiCol_ButtonHovered, UI::Colors::Theme::ButtonHoveredDark);

			const ImVec2 buttonSize = { ImGui::GetFrameHeight(), ImGui::GetFrameHeight() };
			const ImVec2 buttonSizeNoFP = { ImGui::GetFontSize(), ImGui::GetFontSize() };

			const ImU32 buttonColN = UI::Colors::WithMultipliedValue(UI::Colors::Theme::Text, 0.9f);
			const ImU32 buttonColH = UI::Colors::WithMultipliedValue(UI::Colors::Theme::Text, 1.2f);
			const ImU32 buttonColP = UI::Colors::Theme::TextDarker;

			ImGui::BeginDisabled(IsSearchActive());

			// Move Back
			ImGui::BeginDisabled(!m_History.CanMoveBack());
			if (ImGui::InvisibleButton("Move Back", buttonSize))
				NextDirectory(m_History.MoveBack(), false);
			
			UI::DrawButtonFrame();
			UI::DrawImageButton(EditorResources::AngleLeftIcon, buttonColN, buttonColH, buttonColP, UI::RectExpand(UI::GetItemRect(), -style.FramePadding.y, -style.FramePadding.y));
			ImGui::EndDisabled();


			// Move Forward
			ImGui::BeginDisabled(!m_History.CanMoveForward());
			if (ImGui::InvisibleButton("Move Forward", buttonSize))
				NextDirectory(m_History.MoveForward(), false);

			UI::DrawButtonFrame();
			UI::DrawImageButton(EditorResources::AngleRightIcon, buttonColN, buttonColH, buttonColP, UI::RectExpand(UI::GetItemRect(), -style.FramePadding.y, -style.FramePadding.y));
			ImGui::EndDisabled();

			ImGui::EndDisabled();

			// Reload
			if (ImGui::InvisibleButton("Reload", buttonSize))
				m_ReloadScheduled = true;

			UI::DrawButtonFrame();
			UI::DrawImageButton(EditorResources::ReloadIcon, buttonColN, buttonColH, buttonColP, UI::RectExpand(UI::GetItemRect(), -style.FramePadding.y, -style.FramePadding.y));


			if (ImGui::InvisibleButton("Clear Thumbnail Cache", buttonSize))
			{
				if (ImGui::IsKeyDown(ImGuiKey_LeftShift))
					m_ThumbnailCache->ClearDiscCache();
				m_ThumbnailCache->Clear();
			}

			UI::DrawButtonFrame();
			UI::DrawImageButton(EditorResources::ClearIcon, buttonColN, buttonColH, buttonColP, UI::RectExpand(UI::GetItemRect(), -style.FramePadding.y, -style.FramePadding.y));
		}


		// Search
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.2f);
		if (UI::Widgets::Search(m_SearchBuffer))
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

		// Breadcrumb Trail
		{
			UI::ScopedStyleStack styleStack(ImGuiStyleVar_FramePadding, ImVec2{ 0.0f, style.FramePadding.y },
											ImGuiStyleVar_ItemSpacing, ImVec2{ 0.0f, style.ItemSpacing.y });

			for (uint32_t i = 0; i < m_BreadcrumbTrailData.size(); i++)
			{
				Ref<DirectoryInfo> breadcrumb = m_BreadcrumbTrailData[i];

				const auto& name = breadcrumb->Name;
				const ImVec2 pos = ImGui::GetCursorScreenPos();
				const ImVec2 textSize = ImGui::CalcTextSize(name.c_str(), name.c_str() + name.size());
				const ImVec2 buttonSize = textSize + style.FramePadding * 2.0f;

				if (ImGui::InvisibleButton(name.c_str(), buttonSize))
					NextDirectory(breadcrumb);

				if (ImGui::BeginDragDropTarget())
				{
					const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Asset");
					if (payload)
					{
						AssetHandle handle = *(AssetHandle*)payload->Data;
						SK_CORE_VERIFY(m_CurrentItems.Contains(handle));
						auto item = m_CurrentItems.Get(handle);
						item->Move(breadcrumb);
						NextDirectory(m_CurrentDirectory, false, false);
					}
					ImGui::EndDragDropTarget();
				}

				ImU32 textColor = ImGui::GetColorU32(ImGui::IsItemHovered() ? ImGuiCol_Text : ImGuiCol_TextDisabled);
				const ImVec2 text_pos = { pos.x, pos.y + window->DC.CurrLineTextBaseOffset };
				window->DrawList->AddText(text_pos, textColor, name.c_str(), name.c_str() + name.size());

				if (i < m_BreadcrumbTrailData.size() - 1)
					ImGui::TextDisabled("/");
			}
		}

		{
			UI::ScopedStyle frameBorder(ImGuiStyleVar_FrameBorderSize, 0.0f);
			UI::ScopedColorStack colors(
				ImGuiCol_Button, ImVec4{ 0, 0, 0, 0 },
				ImGuiCol_ButtonHovered, ImVec4{ 0, 0, 0, 0 },
				ImGuiCol_ButtonActive, ImVec4{ 0, 0, 0, 0 }
			);

			//const float iconSize = ImGui::GetFontSize();
			//const float settingsIconOffset = ImGui::GetContentRegionAvail().x - iconSize - style.WindowPadding.x - style.ItemSpacing.x;
			//ImGui::SameLine(settingsIconOffset);
			ImGui::Spring();

			const ImVec2 buttonSize = { ImGui::GetFrameHeight(), ImGui::GetFrameHeight() };
			if (ImGui::InvisibleButton("settings", buttonSize))
				ImGui::OpenPopup("cbSettings");



			UI::DrawImageButton(EditorResources::SettingsIcon,
								UI::Colors::WithMultipliedValue(UI::Colors::Theme::Text, 0.9f),
								UI::Colors::WithMultipliedValue(UI::Colors::Theme::Text, 1.2f),
								UI::Colors::Theme::TextDarker,
								UI::RectExpand(UI::GetItemRect(), -style.FramePadding.y, -style.FramePadding.y));

			if (ImGui::BeginPopup("cbSettings"))
			{
				auto& settings = EditorSettings::Get().ContentBrowser;
				ImGui::DragFloat("Thumbnail Size", &settings.ThumbnailSize, 1.0f, 16.0f, FLT_MAX);
				ImGui::Checkbox("Generate Thumbnails", &settings.GenerateThumbnails);
				ImGui::EndPopup();
			}
		}
		ImGui::EndHorizontal();
	}

	void ContentBrowserPanel::DrawDirectoryHierarchy(Ref<DirectoryInfo> directory)
	{
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow;
		if (directory->SubDirectories.empty())
			flags |= ImGuiTreeNodeFlags_Leaf;

		bool open = false;

		const bool current = directory == m_CurrentDirectory;

		{
			UI::ScopedColor selected(ImGuiCol_Header, UI::Colors::Theme::Selection);
			UI::ScopedColorConditional hovered(ImGuiCol_HeaderHovered, UI::Colors::WithMultipliedValue(UI::Colors::Theme::Selection, 1.2f), current);
			UI::ScopedColorConditional active(ImGuiCol_HeaderActive, UI::Colors::WithMultipliedValue(UI::Colors::Theme::Selection, 0.9f), current);

			if (current)
			{
				ImGuiWindow* window = ImGui::GetCurrentWindow();
				ImGuiContext& g = *GImGui;
				const ImGuiStyle& style = g.Style;

				const bool display_frame = (flags & ImGuiTreeNodeFlags_Framed) != 0;
				const ImVec2 padding = (display_frame || (flags & ImGuiTreeNodeFlags_FramePadding)) ? style.FramePadding : ImVec2(style.FramePadding.x, ImMin(window->DC.CurrLineTextBaseOffset, style.FramePadding.y));

				const char* label = directory->Name.c_str();
				const char* label_end = label + directory->Name.size();
				const ImVec2 label_size = ImGui::CalcTextSize(label, label_end, false);

				// We vertically grow up to current line height up the typical widget height.
				const float frame_height = std::max(std::min(window->DC.CurrLineSize.y, g.FontSize + style.FramePadding.y * 2), label_size.y + padding.y * 2);
				ImRect frame_bb;
				frame_bb.Min.x = window->ParentWorkRect.Min.x;
				frame_bb.Min.y = window->DC.CursorPos.y;
				frame_bb.Max.x = window->ParentWorkRect.Max.x;
				frame_bb.Max.y = window->DC.CursorPos.y + frame_height;
				if (display_frame)
				{
					const float outer_extend = IM_TRUNC(window->WindowPadding.x * 0.5f); // Framed header expand a little outside of current limits
					frame_bb.Min.x -= outer_extend;
					frame_bb.Max.x += outer_extend;
				}

				ImU32 colLeft = UI::Colors::Theme::Selection;
				ImU32 colRight = UI::Colors::Theme::SelectionCompliment;
				window->DrawList->AddRectFilledMultiColor(frame_bb.Min, frame_bb.Max, colLeft, colRight, colRight, colLeft);
			}

			open = ImGui::TreeNodeEx(directory->Name.c_str(), flags);
		}

		if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
			NextDirectory(directory);

		if (ImGui::BeginDragDropTarget())
		{
			const ImGuiPayload* assetPayload = ImGui::AcceptDragDropPayload("Asset");
			if (assetPayload)
			{
				AssetHandle handle = *(AssetHandle*)assetPayload->Data;
				if (m_CurrentItems.Contains(handle))
				{
					auto item = m_CurrentItems.Get(handle);
					item->Move(directory);
					NextDirectory(m_CurrentDirectory, false, false);
				}
			}

			const ImGuiPayload* directoryPayload = ImGui::AcceptDragDropPayload("uuid.cb.directory");
			if (directoryPayload)
			{
				UUID id = *(UUID*)directoryPayload->Data;
				SK_CORE_VERIFY(m_CurrentItems.Contains(id));

				auto item = m_CurrentItems.Get(id);
				item->Move(directory);
				NextDirectory(m_CurrentDirectory, false, false);
			}

		}

		if (open)
		{
			for (auto subdir : directory->SubDirectories)
				DrawDirectoryHierarchy(subdir);

			ImGui::TreePop();
		}
	}

	Ref<Texture2D> ContentBrowserPanel::GetAssetIcon(const std::string& extension)
	{
		if (!m_IconExtensionMap.contains(extension))
			return EditorResources::FileIcon;
		return m_IconExtensionMap.at(extension);
	}

	void ContentBrowserPanel::ApplySelectionRequests(ImGuiMultiSelectIO* selectionRequests, bool isBegin)
	{
		for (ImGuiSelectionRequest& request : selectionRequests->Requests)
		{
			switch (request.Type)
			{
				case ImGuiSelectionRequestType_SetAll:
				{
					SelectionManager::DeselectAll(m_SelectionID);
					if (!request.Selected)
						break;

					for (const auto& item : m_CurrentItems)
					{
						SelectionManager::Select(m_SelectionID, item->GetID());
					}
					break;
				}
				case ImGuiSelectionRequestType_SetRange:
				{
					for (int64_t i = request.RangeFirstItem; i <= request.RangeLastItem; i++)
					{
						Ref<ContentBrowserItem> item = m_CurrentItems.Items[i];
						SelectionManager::Toggle(m_SelectionID, item->GetID(), request.Selected);
					}
				}
			}
		}
	}

	void ContentBrowserPanel::HandleSelectionRequests()
	{
		if (m_ClearSelection)
		{
			SelectionManager::DeselectAll(m_SelectionID);
			m_ClearSelection = false;
		}

		for (UUID id : m_ItemsToSelect)
			SelectionManager::Select(m_SelectionID, id);
		m_ItemsToSelect.clear();
	}

	void ContentBrowserPanel::HandleEntityPayload(const ImGuiPayload* payload, Ref<DirectoryInfo> directory)
	{
		Buffer idsBuffer = { payload->Data, (uint64_t)payload->DataSize };
		if (idsBuffer.Count<UUID>() == 1)
		{
			UUID entityID;
			idsBuffer.Read(&entityID, sizeof(UUID));

			Entity entity = m_SceneContext->TryGetEntityByUUID(entityID);
			if (entity)
			{
				CreateAsset<Prefab>(directory, fmt::format("{}.sfab", entity.Tag()), false, entity);
			}
		}
	}

	Ref<ContentBrowserItem> ContentBrowserPanel::CreateDirectory(const std::string& name, bool startRenaming)
	{
		if (!FileSystem::IsValidFilename(name))
			return nullptr;

		std::filesystem::path directoryPath = m_CurrentDirectory->Filepath / name;
		FileSystem::AssureUniqueness(directoryPath);

		if (!FileSystem::CreateDirectory(directoryPath))
			return nullptr;

		Ref<DirectoryInfo> newDirectory = Ref<DirectoryInfo>::Create(m_CurrentDirectory, directoryPath);
		m_DirectoryMap[newDirectory->ID] = newDirectory;
		m_CurrentDirectory->AddDirectory(newDirectory);

		Ref<ContentBrowserItem> item = Ref<ContentBrowserDirectory>::Create(this, newDirectory);
		m_CurrentItems.Add(item);
		SelectItem(newDirectory->ID);

		if (startRenaming)
			item->StartRenaming();

		return item;
	}

}
