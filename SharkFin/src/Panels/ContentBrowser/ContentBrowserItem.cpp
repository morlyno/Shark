#include "skfpch.h"
#include "ContentBrowserItem.h"

#include "Shark/Core/SelectionManager.h"

#include "Shark/UI/UICore.h"
#include "Shark/UI/EditorResources.h"

#include "EditorSettings.h"
#include "Panels/ContentBrowser/ContentBrowserPanel.h"

#include "Shark/Debug/Profiler.h"

#include <imgui.h>


namespace Shark {

	DirectoryInfo::DirectoryInfo(Weak<DirectoryInfo> parent, const std::filesystem::path& directoryPath)
		: DirectoryInfo(UUID::Generate(), parent, directoryPath)
	{
	}

	DirectoryInfo::DirectoryInfo(UUID id, Weak<DirectoryInfo> parent, const std::filesystem::path& directoryPath)
	{
		ID = id;
		Parent = parent;
		Filepath = directoryPath;
		Name = directoryPath.filename().string();
	}

	DirectoryInfo::~DirectoryInfo()
	{

	}

	void DirectoryInfo::AddDirectory(Ref<DirectoryInfo> directory)
	{
		const auto where = std::lower_bound(SubDirectories.begin(), SubDirectories.end(), directory, [](const auto& lhs, const auto& rhs)
		{
			return lhs->Filepath < rhs->Filepath;
		});
		SubDirectories.insert(where, directory);
	}

	void DirectoryInfo::AddFile(const std::string& filename)
	{
		const auto where = std::ranges::lower_bound(Filenames, filename);
		Filenames.insert(where, filename);
	}

	void DirectoryInfo::RemoveFile(const std::string& filename)
	{
		std::erase(Filenames, filename);
	}

	void DirectoryInfo::RemoveDirectory(Ref<DirectoryInfo> directory)
	{
		std::erase(SubDirectories, directory);
	}

	void DirectoryInfo::RemoveDirectory(const std::filesystem::path& dirPath)
	{
		std::erase_if(SubDirectories, [&dirPath](Ref<DirectoryInfo> dir) { return dir->Filepath == dirPath; });
	}

	void DirectoryInfo::Rename(const std::string& newName)
	{
		Name = newName;
		FileSystem::ReplaceFilename(Filepath, newName);
	}

	void DirectoryInfo::RenameFile(const std::string& oldName, const std::string& newName)
	{
		auto it = std::ranges::find(Filenames, oldName);
		if (it != Filenames.end())
		{
			std::filesystem::path name = *it;
			FileSystem::ReplaceStem(name, newName);
			*it = name.string();
			std::ranges::sort(Filenames);
		}
	}

	static char s_RenameBuffer[MAX_INPUT_BUFFER_LENGTH];

	ContentBrowserItem::ContentBrowserItem(Ref<ContentBrowserPanel> context, CBItemType type, UUID id, const std::string& name, Ref<Texture2D> icon)
		: m_Context(context), m_Type(type), m_ID(id), m_FileName(name), m_Icon(icon)
	{
		SetDisplayNameFromFileName();
	}

	ContentBrowserItem::~ContentBrowserItem()
	{
	}

	CBItemAction ContentBrowserItem::Draw()
	{
		SK_PROFILE_FUNCTION();
		CBItemAction action;

		UI::ScopedID id(ImGui::GetIDWithSeed(this, 0));

		Ref<ContentBrowserPanel> context = m_Context.GetRef();
		Ref<ThumbnailCache> thumbnailCache = context->GetThumbnailCache();
		const EditorSettings& editorSettings = EditorSettings::Get();
		const ImGuiStyle& style = ImGui::GetStyle();

		//==================================//
		//= Variables ======================//
		const float thumbnailSize = editorSettings.ContentBrowser.ThumbnailSize;
		const float thumbnailBorderSize = 2.0f;
		const float lineHeight = ImGui::GetTextLineHeight();
		const float linePadding = 2.0f;
		const float cornerRounding = 6.0f;
		const ImDrawFlags cornerFlags = m_Type == CBItemType::Directory ? ImDrawFlags_RoundCornersAll : ImDrawFlags_RoundCornersBottom;

		ImRect thumbnailRect;
		thumbnailRect.Min = ImGui::GetCursorScreenPos();
		thumbnailRect.Max = thumbnailRect.Min + ImVec2(thumbnailSize, thumbnailSize);

		ImRect propertyRect;
		propertyRect.Min = ImVec2(thumbnailRect.Min.x, thumbnailRect.Max.y);
		propertyRect.Max = ImVec2(thumbnailRect.Max.x, thumbnailRect.Max.y + lineHeight * 3.0f + linePadding * 4.0f);

		ImRect itemRect = { thumbnailRect.Min, propertyRect.Max };
		bool isSelected = SelectionManager::IsSelected(SelectionContext::ContentBrowser, m_ID);


		//==================================//
		//= Setup ==========================//

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0.0f, linePadding });

		ImGui::SetNextItemAllowOverlap();
		UI::MultiSelectInvisibleButton(ImGui::GetIDWithSeed(m_ID, 0), isSelected, itemRect.GetSize());

		const ImGuiID buttonID = ImGui::GetItemID();
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip | ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay))
			ImGui::SetTooltip("%s", m_FileName.c_str());

		const bool isHovered = ImGui::IsItemHovered();

		//==================================//
		//= Draw Background ================//

		if (m_Type == CBItemType::Asset)
		{
			UI::DrawBackground(UI::RectOffset(itemRect, 2.0f, 2.0f), IM_COL32(10, 10, 10, 255), cornerRounding, ImDrawFlags_RoundCornersBottom);
			UI::DrawBackground(thumbnailRect, UI::Colors::Theme::BackgroundDark);
			UI::DrawBackground(propertyRect, UI::Colors::Theme::BackgroundPopup, cornerRounding, ImDrawFlags_RoundCornersBottom);
			//UI::DrawBorder(propertyRect, UI::Colors::Theme::BackgroundDark, cornerRounding, ImDrawFlags_RoundCornersBottom);
		}

		if (m_Type == CBItemType::Directory && (isHovered || isSelected))
		{
			UI::DrawBackground(UI::RectOffset(itemRect, 2.0f, 2.0f), IM_COL32(10, 10, 10, 255), cornerRounding);
			UI::DrawBackground(itemRect, UI::Colors::Theme::BackgroundPopup, cornerRounding);
		}

		//==================================//
		//= Draw Thumbnail =================//

		ImRect buttonRect = UI::GetItemRect();
		buttonRect.Max.y = thumbnailRect.Max.y;

		DrawPopupMenu(action);
		UpdateDragDrop(action);

		if (m_Type == CBItemType::Directory)
		{
			UI::DrawImageButton(m_Icon,
								IM_COL32(255, 255, 255, 235),
								IM_COL32(255, 255, 255, 255),
								IM_COL32(255, 255, 255, 225),
								UI::RectExpand(buttonRect, -thumbnailBorderSize, -thumbnailBorderSize));
		}
		else
		{
			Ref<Image2D> thumbnail = m_Icon->GetImage();
			if (thumbnailCache->HasThumbnail(m_ID))
				thumbnail = thumbnailCache->GetThumbnail(m_ID);

			//UI::DrawImage(Icons::AlphaBackground, UI::RectExpand(buttonRect, -thumbnailBorderSize, -thumbnailBorderSize), { 0, 0 }, { 2, 2 });
			UI::DrawImageButton(thumbnail,
								IM_COL32(255, 255, 255, 255),
								IM_COL32(255, 255, 255, 255),
								IM_COL32(255, 255, 255, 255),
								UI::RectExpand(buttonRect, -thumbnailBorderSize, -thumbnailBorderSize));
		}

		if (ImGui::IsItemHovered())
		{
			if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				action.Set(CBItemActionFlag::Activated);

			if (ImGui::IsItemActivated() && !ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsKeyPressed(ImGuiKey_Space))
				action.Set(CBItemActionFlag::Activated);
		}

		UI::ShiftCursorY(-propertyRect.GetHeight());

		//==================================//
		//= Draw Property ==================//

		UI::ScopedIndent indent(style.FramePadding.x);
		//ImGui::PushClipRect(propertyRect.Min, propertyRect.Max, true);

		if (m_IsRenameing)
		{
			bool invalidInput = false;

			UI::ScopedStyleStack inputStyles(ImGuiStyleVar_FramePadding, { 0.0f, 0.0f },
											 ImGuiStyleVar_FrameBorderSize, 0.0f);
			UI::ScopedColorStack inputColors(ImGuiCol_FrameBg, 0,
											 ImGuiCol_FrameBgActive, 0,
											 ImGuiCol_FrameBgHovered, 0);

			ImGui::SetNextItemWidth(-1.0f);
			UI::InputText("##rename", s_RenameBuffer, std::size(s_RenameBuffer), ImGuiInputTextFlags_CallbackCharFilter, UI_INPUT_TEXT_FILTER("\\/:*?\"<>|"));

			if (ImGui::IsItemDeactivatedAfterEdit())
			{
				Rename(s_RenameBuffer);
				action.Set(CBItemActionFlag::Renamed);
			}

			if (!ImGui::IsItemActive())
				m_IsRenameing = false;
		}
		else
		{
			if (m_Type == CBItemType::Directory)
			{
				UI::DrawTextAligned(m_DisplayName, ImVec2(0.5f, 0.0f), propertyRect);
			}
			else
			{
				ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + (thumbnailSize - thumbnailBorderSize * 2.0f - style.FramePadding.x));
				ImGui::TextWrapped(m_DisplayName.c_str());
				ImGui::PopTextWrapPos();
			}
		}

		UI::SetCursorScreenPosY(propertyRect.Max.y - lineHeight - style.FramePadding.y);

		// Item Type
		if (m_Type == CBItemType::Asset)
		{
			const auto& metadata = Project::GetActiveEditorAssetManager()->GetMetadata(m_ID);
			auto assetTypeName = magic_enum::enum_name(metadata.Type);
			ImVec2 textSize = ImGui::CalcTextSize(assetTypeName.data(), assetTypeName.data() + assetTypeName.size());
			UI::ShiftCursorX(thumbnailSize - textSize.x - style.FramePadding.x * 2.0f);

			UI::ScopedColor textColor(ImGuiCol_Text, UI::Colors::Theme::TextDarker);
			ImGui::TextEx(assetTypeName.data(), assetTypeName.data() + assetTypeName.size());
		}

		//ImGui::PopClipRect();

		//==================================//
		//= Draw Outline ===================//

		if (m_Type == CBItemType::Asset || isSelected)
		{
			const float outlineSize = 3.0f;
			UI::ScopedStyle borderSize(ImGuiStyleVar_FrameBorderSize, outlineSize);
			UI::ScopedClipRect clipRect(UI::RectExpand(ImGui::GetCurrentWindowRead()->ClipRect, outlineSize + 1.0f, outlineSize + 1.0f));

			const ImU32 borderColor = (isHovered || isSelected) ? (isHovered ? UI::Colors::WithMultipliedValue(UI::Colors::Theme::Selection, 1.2f) : UI::Colors::Theme::Selection) : 0;
			//UI::DrawBorder(UI::RectExpand(itemRect, outlineSize, outlineSize), borderColor, cornerRounding, ImDrawFlags_RoundCornersBottom);
			UI::DrawBorder(UI::RectExpand(itemRect, -1.0f, -1.0f), borderColor, cornerRounding, cornerFlags);
		}

		{
			UI::ScopedStyle rounding(ImGuiStyleVar_FrameRounding, cornerRounding);
			ImGui::RenderNavHighlight(itemRect, buttonID, ImGuiNavHighlightFlags_Compact, cornerFlags);
		}

		ImGui::PopStyleVar();

		UI::SetCursorScreenPosY(itemRect.Max.y);
		ImGui::Dummy({ 0, 0 });

		return action;
	}

	void ContentBrowserItem::DrawPopupMenu(CBItemAction& action)
	{
		if (SelectionManager::IsSelected(SelectionContext::ContentBrowser, m_ID))
		{
			if (ImGui::Shortcut(ImGuiKey_F2))
				action.Set(CBItemActionFlag::StartRenaming);

			if (ImGui::Shortcut(ImGuiKey_Delete))
				action.Set(CBItemActionFlag::OpenDeleteDialogue);
		}


		if (ImGui::BeginPopupContextItem())
		{
			ImGui::Text("Filename: %s", m_FileName.c_str());
			ImGui::Separator();

			if (ImGui::MenuItem("Reload", nullptr, false, m_Type == CBItemType::Asset))
				action.Set(CBItemActionFlag::Reload);

			DrawCustomContextItems();

			ImGui::Separator();

			if (ImGui::MenuItem("Open Externally"))
				action.Set(CBItemActionFlag::OpenExternal);

			if (ImGui::MenuItem("Show in Explorer"))
				action.Set(CBItemActionFlag::ShowInExplorer);

			ImGui::Separator();

			if (ImGui::MenuItem("Rename", "F2"))
				action.Set(CBItemActionFlag::StartRenaming);

			if (ImGui::MenuItem("Delete", "Del"))
				action.Set(CBItemActionFlag::OpenDeleteDialogue);

			ImGui::EndPopup();
		}
	}

	std::string_view ContentBrowserItem::GetSpecificTypeName() const
	{
		if (m_Type == CBItemType::Directory)
			return "Directory";

		const auto& metadata = Project::GetActiveEditorAssetManager()->GetMetadata(m_ID);
		return magic_enum::enum_name(metadata.Type);
	}

	void ContentBrowserItem::StartRenaming()
	{
		m_IsRenameing = true;
		strcpy_s(s_RenameBuffer, m_DisplayName.c_str());

		ImGui::PushID(ImGui::GetIDWithSeed(this, 0));
		ImGui::ActivateItemByID(ImGui::GetID("##rename"));
		ImGui::PopID();
	}

	void ContentBrowserItem::StopRenaming()
	{
		m_IsRenameing = false;
	}

	void ContentBrowserItem::SetDisplayNameFromFileName()
	{
		if (m_Type == CBItemType::Directory)
			m_DisplayName = FileSystem::GetFilenameString(m_FileName);
		else
			m_DisplayName = FileSystem::GetStemString(m_FileName);
	}

	void ContentBrowserItem::OnRenamed(const std::string& newFilename)
	{
		m_FileName = newFilename;
		SetDisplayNameFromFileName();
	}

	ContentBrowserAsset::ContentBrowserAsset(Ref<ContentBrowserPanel> context, const AssetMetaData& metadata, Ref<Texture2D> icon)
		: ContentBrowserItem(context, CBItemType::Asset, metadata.Handle, FileSystem::GetFilenameString(metadata.FilePath), icon), m_Metadata(metadata)
	{
	}

	ContentBrowserAsset::~ContentBrowserAsset()
	{
	}

	bool ContentBrowserAsset::Move(Ref<DirectoryInfo> destinationDirectory)
	{
		SK_CORE_VERIFY(destinationDirectory);

		auto filesystemPath = Project::GetActiveAssetsDirectory() / m_Metadata.FilePath;
		auto destinationPath = FileSystem::UniquePath(destinationDirectory->Filepath / m_FileName);

		if (!FileSystem::Move(filesystemPath, destinationPath))
		{
			// TODO(moro): Error prompt
			return false;
		}

		Ref<DirectoryInfo> originalParent = m_Context->FindDirectory(filesystemPath.parent_path());
		originalParent->RemoveFile(m_FileName);
		destinationDirectory->AddFile(m_FileName);
		Project::GetActiveEditorAssetManager()->AssetMoved(m_ID, destinationPath);
		return true;
	}

	bool ContentBrowserAsset::Rename(const std::string& newName)
	{
		auto filesystemPath = Project::GetActiveAssetsDirectory() / m_Metadata.FilePath;
		
		SK_CORE_VERIFY(m_Metadata.FilePath.has_extension());
		std::string uniqueName = fmt::format("{}{}", newName, FileSystem::GetExtensionString(m_Metadata.FilePath));
		FileSystem::AssureUniquenessInDirectory(filesystemPath.parent_path(), uniqueName);

		if (!FileSystem::Rename(filesystemPath, uniqueName))
		{
			// TODO(moro): Error prompt
			return false;
		}

		Ref<DirectoryInfo> parent = m_Context->FindDirectory(filesystemPath.parent_path());
		parent->RenameFile(FileSystem::GetFilenameString(m_Metadata.FilePath), uniqueName);

		// Updates metadata so m_Metadata.FilePath is current afterwards
		Project::GetActiveEditorAssetManager()->AssetRenamed(m_Metadata.Handle, uniqueName);

		OnRenamed(uniqueName);
		return true;
	}

	bool ContentBrowserAsset::Delete()
	{
		auto filesystemPath = Project::GetActiveAssetsDirectory() / m_Metadata.FilePath;

		std::string errorMsg;
		if (!FileSystem::Remove(filesystemPath, errorMsg))
		{
			auto dialogueMessage = fmt::format("Failed to delete Asset!\nFile: {}\nMessage: {}", m_Metadata.FilePath, errorMsg);
			m_Context->ShowErrorDialogue(ErrorType::OperationFailed, dialogueMessage, ErrorResponse::OK);
			// TODO(moro): Error prompt
			return false;
		}

		Ref<DirectoryInfo> parent = m_Context->FindDirectory(filesystemPath.parent_path());
		parent->RemoveFile(FileSystem::GetFilenameString(m_Metadata.FilePath));

		Project::GetActiveEditorAssetManager()->AssetDeleted(m_Metadata.Handle);
		return true;
	}

	void ContentBrowserAsset::DrawCustomContextItems()
	{
		if (m_Metadata.Type == AssetType::Mesh)
		{
			AssetHandle sourceHandle = AssetHandle::Invalid;
			if (m_Metadata.Status == AssetStatus::Ready)
			{
				Ref<Mesh> mesh = AssetManager::GetAsset<Mesh>(m_Metadata.Handle);
				sourceHandle = mesh->GetMeshSource();
				if (!AssetManager::IsValidAssetHandle(sourceHandle))
					sourceHandle = AssetHandle::Invalid;
			}

			if (m_Metadata.Status != AssetStatus::Ready)
			{
				ImGui::SetItemTooltip("Mesh not loaded");
			}

			const bool isValid = AssetManager::IsValidAssetHandle(sourceHandle);
			if (ImGui::MenuItem("Select MeshSource", nullptr, nullptr, isValid))
			{
				auto assetManager = Project::GetActiveEditorAssetManager();
				const auto& metadata = assetManager->GetMetadata(sourceHandle);
				Ref<DirectoryInfo> directory = m_Context->FindDirectory(FileSystem::GetParent(assetManager->GetFilesystemPath(metadata)));
				if (directory)
				{
					m_Context->NextDirectory(directory);
					m_Context->SelectItem(metadata.Handle);
				}
			}

		}

	}

	void ContentBrowserAsset::UpdateDragDrop(CBItemAction& action)
	{
		if (ImGui::BeginDragDropSource())
		{
			ImGui::SetDragDropPayload("Asset", &m_ID, sizeof(UUID));
			ImGui::Text(m_DisplayName);
			
			Ref<Image2D> thumbnail = m_Icon->GetImage();
			Ref<ThumbnailCache> cache = m_Context->GetThumbnailCache();
			if (cache->HasThumbnail(m_ID))
				thumbnail = cache->GetThumbnail(m_ID);

			const float ratio = thumbnail->GetVerticalAspectRatio();
			UI::Image(thumbnail, ImVec2(128, 128 * ratio));
			ImGui::EndDragDropSource();
		}
	}

	ContentBrowserDirectory::ContentBrowserDirectory(Ref<ContentBrowserPanel> context, Ref<DirectoryInfo> directoryInfo)
		: ContentBrowserItem(context, CBItemType::Directory, directoryInfo->ID, FileSystem::GetFilenameString(directoryInfo->Filepath), EditorResources::FolderIcon), m_DirectoryInfo(directoryInfo)
	{
	}

	ContentBrowserDirectory::~ContentBrowserDirectory()
	{
	}

	bool ContentBrowserDirectory::Move(Ref<DirectoryInfo> destinationDirectory)
	{
		SK_CORE_VERIFY(destinationDirectory);

		auto destinationPath = FileSystem::UniquePath(destinationDirectory->Filepath / m_DirectoryInfo->Name);
		if (!FileSystem::Move(m_DirectoryInfo->Filepath, destinationPath))
		{
			// TODO(moro): Error prompt
			return false;
		}

		m_DirectoryInfo->Parent->RemoveDirectory(m_DirectoryInfo);
		m_DirectoryInfo->Parent = destinationDirectory;
		destinationDirectory->AddDirectory(m_DirectoryInfo);

		UpdateSubdirectories(m_DirectoryInfo);
		return true;
	}

	bool ContentBrowserDirectory::Rename(const std::string& newName)
	{
		std::string uniqueName = newName;
		FileSystem::AssureUniquenessInDirectory(m_DirectoryInfo->Filepath.parent_path(), uniqueName);

		if (!FileSystem::Rename(m_DirectoryInfo->Filepath, uniqueName))
		{
			// TODO(moro): Error prompt
			return false;
		}

		m_DirectoryInfo->Rename(uniqueName);
		UpdateSubdirectories(m_DirectoryInfo);

		OnRenamed(uniqueName);
		return true;
	}

	bool ContentBrowserDirectory::Delete()
	{
		if (!FileSystem::RemoveAll(m_DirectoryInfo->Filepath))
		{
			// TODO(moro): Error prompt
			return false;
		}

		m_DirectoryInfo->Parent->RemoveDirectory(m_DirectoryInfo);
		DeleteSubdirectories(m_DirectoryInfo);
		return true;
	}

	void ContentBrowserDirectory::UpdateDragDrop(CBItemAction& action)
	{
		if (ImGui::BeginDragDropSource())
		{
			ImGui::SetDragDropPayload("uuid.cb.directory", &m_ID, sizeof(m_ID));
			ImGui::Text(m_DisplayName);
			UI::Image(m_Icon, { 128, 128 * m_Icon->GetVerticalAspectRatio() });
			ImGui::EndDragDropSource();
		}

		if (ImGui::BeginDragDropTarget())
		{
			const ImGuiPayload* assetPayload = ImGui::AcceptDragDropPayload("Asset", ImGuiDragDropFlags_AcceptBeforeDelivery);
			if (assetPayload && assetPayload->IsDelivery())
				action.Set(CBItemActionFlag::DropAccepted);

			const ImGuiPayload* directoryPayload = ImGui::AcceptDragDropPayload("uuid.cb.directory", ImGuiDragDropFlags_AcceptBeforeDelivery);
			if (directoryPayload && directoryPayload->IsDelivery())
				action.Set(CBItemActionFlag::DropAccepted);
			ImGui::EndDragDropTarget();
		}
	}

	void ContentBrowserDirectory::UpdateSubdirectories(Ref<DirectoryInfo> directory)
	{
		SK_CORE_VERIFY(directory->Parent);
		std::filesystem::path oldDirectoryPath = directory->Filepath;
		directory->Filepath = directory->Parent->Filepath / directory->Name;

		Ref<EditorAssetManager> assetManager = Project::GetActiveEditorAssetManager();
		for (const std::string& filename : directory->Filenames)
		{
			AssetHandle handle = assetManager->GetAssetHandleFromFilepath(oldDirectoryPath / filename);
			if (assetManager->IsValidAssetHandle(handle))
				assetManager->AssetMoved(handle, directory->Filepath / filename);
		}

		for (Ref<DirectoryInfo> subdir : directory->SubDirectories)
			UpdateSubdirectories(subdir);
	}

	void ContentBrowserDirectory::DeleteSubdirectories(Ref<DirectoryInfo> directory)
	{
		for (Ref<DirectoryInfo> subdir : directory->SubDirectories)
			DeleteSubdirectories(subdir);

		Ref<EditorAssetManager> assetManager = Project::GetActiveEditorAssetManager();
		for (const std::string& filename : directory->Filenames)
		{
			AssetHandle handle = assetManager->GetAssetHandleFromFilepath(directory->Filepath / filename);
			if (assetManager->IsValidAssetHandle(handle))
				assetManager->AssetDeleted(handle);
		}
	}

}
