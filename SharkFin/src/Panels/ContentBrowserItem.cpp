#include "skfpch.h"
#include "ContentBrowserItem.h"

#include "Shark/Asset/ResourceManager.h"
#include "Shark/UI/UI.h"
#include "Shark/UI/Theme.h"
#include "Shark/Editor/Icons.h"
#include "Shark/Editor/EditorSettings.h"

#include "Panels/ContentBrowserPanel.h"

#include <imgui.h>
#include <imgui_internal.h>

namespace Shark {

	ContentBrowserItem::ContentBrowserItem(CBItemType type, AssetHandle handle, const std::string& name, Ref<Image2D> thumbnail)
		: m_Type(type), m_Handle(handle), m_Name(name), m_Thumbnail(thumbnail)
	{
	}

	ContentBrowserItem::ContentBrowserItem(Ref<DirectoryInfo> directory)
		: m_Type(CBItemType::Directory), m_Handle(directory->Handle), m_Name(directory->FilePath.stem().string()), m_Thumbnail(Icons::FolderIcon)
	{
	}

	ContentBrowserItem::ContentBrowserItem(const AssetMetaData& metadata, Ref<Image2D> thumbnail)
		: m_Type(CBItemType::Asset), m_Handle(metadata.Handle), m_Name(metadata.FilePath.stem().string()), m_Thumbnail(thumbnail)
	{
	}

	ContentBrowserItem::~ContentBrowserItem()
	{
	}

	CBItemAction::Flags ContentBrowserItem::Draw()
	{
		SK_CORE_ASSERT(IsStateSet(State::Deleted) == false);
		if (IsStateSet(State::Deleted))
			return CBItemAction::ReloadRequired;

		UI::ScopedID id(this);
		CBItemAction::Flags result = CBItemAction::None;

		const auto& editorSettings = EditorSettings::Get();
		const float thumbnailSize = editorSettings.ContentBrowserThumbnailSize;
		ImGuiStyle& style = ImGui::GetStyle();

		const float padding = 2.0f;
		const float framePadding = 2.0f;
		const float textHeight = ImGui::GetTextLineHeight();
		const float rounding = 6.0f;
		UI::MoveCursorX(-padding);
		const ImVec2 thumbnailTopLeft = ImGui::GetCursorScreenPos();
		const ImVec2 thumbnailBottemRight = { thumbnailTopLeft.x + thumbnailSize, thumbnailTopLeft.y + thumbnailSize };
		const ImVec2 infoTopLeft = { thumbnailTopLeft.x, thumbnailBottemRight.y };
		const ImVec2 infoBottemRight = { thumbnailBottemRight.x, thumbnailBottemRight.y + textHeight * 2.0f + padding * 4.0f + framePadding };

		const ImRect cellRect = { thumbnailTopLeft, infoBottemRight };
		m_IsHovered = cellRect.Contains(ImGui::GetMousePos()) && ImGui::IsAnyItemHovered() && GImGui->HoveredId != ImGui::GetID("##thumbnailButton");
		
		// Frame
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilled(thumbnailTopLeft, thumbnailBottemRight, UI::ToColor32(Theme::Colors::PropertyField), rounding, ImDrawFlags_RoundCornersTop);
		drawList->AddRectFilled(infoTopLeft, infoBottemRight, UI::ToColor32(Theme::Colors::InfoField), rounding, ImDrawFlags_RoundCornersBottom);

		{
			UI::ScopedStyle spacing(ImGuiStyleVar_ItemSpacing, { 0.0f, padding });

			ImGui::InvisibleButton("##thumbnailButton", { thumbnailSize, thumbnailSize });
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup))
			{
				if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
					result |= CBItemAction::Open;
				if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
				{
					result |= CBItemAction::Selected;
					SetState(State::Selcted, true);
				}
			}

			if (ImGui::BeginPopupContextItem())
			{
				if (!IsStateSet(State::Selcted))
				{
					SetState(State::Selcted, true);
					result |= CBItemAction::Selected;
				}

				if (ImGui::MenuItem("Reload", nullptr, false, m_Type == CBItemType::Asset))
					result |= CBItemAction::ReloadAsset;

				ImGui::Separator();

				if (ImGui::MenuItem("Open"))
					result |= CBItemAction::Open;

				if (ImGui::MenuItem("Show in Explorer"))
					result |= CBItemAction::OpenInExplorer;

				ImGui::Separator();

				if (ImGui::MenuItem("Rename", "F2"))
					StartRenameing();

				if (ImGui::MenuItem("Delete", "Del"))
				{
					Delete();
					result |= CBItemAction::Deleted;
				}

				ImGui::EndPopup();
			}

			if (m_Type == CBItemType::Asset)
			{
				if (ImGui::BeginDragDropSource())
				{
					ImGui::SetDragDropPayload("ASSET", &m_Handle, sizeof(AssetHandle));
					ImGui::EndDragDropSource();
				}
			}

			const float borderSize = 2.0f;
			drawList->AddImageRounded(
				m_Thumbnail->GetViewID(),
				thumbnailTopLeft + ImVec2(borderSize, borderSize),
				thumbnailBottemRight - ImVec2(borderSize, borderSize),
				ImVec2(0, 0),
				ImVec2(1, 1),
				IM_COL32_WHITE,
				rounding - 1,
				ImDrawFlags_RoundCornersTop
			);

			UI::ScopedIndent indent(style.FramePadding.x);
			ImGui::PushClipRect(infoTopLeft, infoBottemRight, true);
			UI::MoveCursorY(padding);

			SK_CORE_ASSERT(!(IsStateSet(State::Renaming) && IsStateSet(State::StartRenaming)));
			if (IsStateSet(State::StartRenaming))
			{
				ImGui::SetKeyboardFocusHere();
				SetState(State::StartRenaming, false);
				SetState(State::Renaming, true);
			}

			if (IsStateSet(State::Renaming))
			{
				ImGui::SetNextItemWidth(thumbnailSize - padding * 2.0f - style.FramePadding.x * 2.0f);
				UI::InputTextFiltered(UI::GenerateID(), RenameBuffer, (int)std::size(RenameBuffer), L"\\/:*?\"<>|");
				if (ImGui::IsItemDeactivatedAfterEdit())
					Rename(RenameBuffer);
				else if (ImGui::IsItemDeactivated())
					SetState(State::Renaming, false);
			}
			else
			{
				ImGui::Text(m_Name.c_str());
			}

			// Item Type
			auto typeString = GetTypeString();
			ImVec2 textSize = ImGui::CalcTextSize(typeString.c_str(), typeString.c_str() + typeString.size());
			UI::MoveCursorX(thumbnailSize - textSize.x - style.FramePadding.x * 2.0f);
			ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_Text) * 0.8f, typeString.c_str());
			ImGui::PopClipRect();
		}

		// Outline
		const ImVec2 borderPadding = { 1.0f, 1.0f };
		const bool coloredBorder = m_IsHovered || IsStateSet(State::Selcted);
		const ImU32 borderColor = UI::ToColor32(coloredBorder ? Theme::Colors::BorderColored : ImGui::GetStyleColorVec4(ImGuiCol_Border));
		const ImU32 shadowColor = UI::ToColor32(coloredBorder ? Theme::Colors::ShadowColored : ImGui::GetStyleColorVec4(ImGuiCol_BorderShadow));

		const ImVec2 borderMin = thumbnailTopLeft - borderPadding;
		const ImVec2 borderMax = infoBottemRight + borderPadding;
		float borderSize = style.FrameBorderSize;
		drawList->AddRect(borderMin + ImVec2(1, 1), borderMax + ImVec2(1, 1), shadowColor, rounding, 0, borderSize);
		drawList->AddRect(borderMin, borderMax, borderColor, rounding, 0, borderSize);

		UI::MoveCursorY(style.ItemSpacing.y - padding);

		return result;
	}

	void ContentBrowserItem::StartRenameing()
	{
		strcpy_s(RenameBuffer, m_Name.c_str());
		SetState(State::StartRenaming, true);
	}

	void ContentBrowserItem::Rename(const std::string& name)
	{
		SetState(State::Renaming, false);

		if (name.empty())
		{
			SK_CORE_WARN("[ContentBrowserItem] Can't rename item to empty name");
			return;
		}

		ContentBrowserPanel::Get().SkipNextFileEvents();

		if (m_Type == CBItemType::Directory)
		{
			ContentBrowserPanel& cbPanel = ContentBrowserPanel::Get();
 			Ref<DirectoryInfo> directory = cbPanel.GetDirectory(this);
			std::filesystem::path oldPath = cbPanel.GetProject()->Directory / directory->FilePath;
			std::filesystem::path newPath = oldPath;
			newPath.replace_filename(name);
			newPath.replace_extension();

			std::error_code error;
			std::filesystem::rename(oldPath, newPath, error);
			if (error)
			{
				SK_ERROR(error.message());
				return;
			}

			m_Name = newPath.stem().string();
			newPath = std::filesystem::relative(newPath, cbPanel.GetProject()->Directory);
			String::FormatDefault(newPath);
			directory->FilePath = newPath;

			return;
		}

		const auto& metadata = ResourceManager::GetMetaData(m_Handle);
		std::filesystem::path oldPath = ResourceManager::GetFileSystemPath(metadata);
		std::filesystem::path newPath = oldPath;
		newPath.replace_filename(name);
		newPath.replace_extension(metadata.FilePath.extension());

		std::error_code error;
		std::filesystem::rename(oldPath, newPath, error);
		if (error)
		{
			SK_ERROR(error.message());
			return;
		}

		m_Name = newPath.stem().string();
	}

	void ContentBrowserItem::Delete()
	{
		SetState(State::Deleted, true);

		if (m_Type == CBItemType::Directory)
		{
			auto& cb = ContentBrowserPanel::Get();
			Ref<DirectoryInfo> directory = cb.GetDirectory(this);
			PlatformUtils::MoveFileToRecycleBin(cb.m_Project->Directory / directory->FilePath);
			//std::filesystem::remove(cb.m_Project->Directory / directory->FilePath);
			return;
		}

		const auto& metadata = ResourceManager::GetMetaData(m_Handle);
		PlatformUtils::MoveFileToRecycleBin(ResourceManager::GetFileSystemPath(metadata));
		//std::filesystem::remove(ResourceManager::GetFileSystemPath(metadata));
	}

	void ContentBrowserItem::SetState(State::Flags state, bool enabled)
	{
		if (enabled)
			m_State |= state;
		else
			m_State &= ~state;
	}

	bool ContentBrowserItem::IsStateSet(State::Flags state) const
	{
		return (m_State & state) == state;
	}

	std::string ContentBrowserItem::GetTypeString()
	{
		SK_CORE_ASSERT(m_Type != CBItemType::None);

		if (m_Type == CBItemType::Directory)
			return "Directory";

		const auto& metadata = ResourceManager::GetMetaData(m_Handle);
		if (!metadata.IsValid())
			return "Asset";

		return AssetTypeToString(metadata.Type);
	}

	DirectoryInfo::DirectoryInfo(const std::filesystem::path& filePath, AssetHandle handle)
		: DirectoryInfo(nullptr, filePath, handle)
	{
	}

	DirectoryInfo::DirectoryInfo(Ref<DirectoryInfo> parent, const std::filesystem::path& filePath, AssetHandle handle)
		: Parent(parent), FilePath(String::FormatDefaultCopy(filePath)), Handle(handle)
	{
		Reload();
	}

	DirectoryInfo::~DirectoryInfo()
	{

	}

	void DirectoryInfo::Reload()
	{
		Assets.clear();
		SubDirectories.clear();

		for (const auto& entry : std::filesystem::directory_iterator(ContentBrowserPanel::Get().m_Project->Directory / FilePath))
		{
			CBItemType itemType = CBItemType::None;
			std::string name = entry.path().stem().string();
			std::filesystem::path filePath = std::filesystem::relative(entry.path(), ContentBrowserPanel::Get().m_Project->Directory);

			if (entry.is_directory())
			{
				AssetHandle handle = AssetHandle::Generate();
				SubDirectories.emplace_back(Ref<DirectoryInfo>::Create(this, filePath, handle));
				continue;
			}

			const auto& metadata = ResourceManager::GetMetaData(entry.path());
			if (metadata.IsValid())
				Assets.emplace_back(metadata.Handle);
		}

		std::sort(SubDirectories.begin(), SubDirectories.end(), [](const auto& lhs, const auto& rhs)
		{
			return lhs->FilePath < rhs->FilePath;
		});

		//SK_CORE_ASSERT(std::is_sorted(SubDirectories.begin(), SubDirectories.end(), [](const auto& lhs, const auto& rhs) { return lhs->FilePath < rhs->FilePath; }));
	}

	void DirectoryInfo::AddDirectory(Ref<DirectoryInfo> directory)
	{
		const auto where = std::lower_bound(SubDirectories.begin(), SubDirectories.end(), directory, [](const auto& lhs, const auto& rhs)
		{
			return lhs->FilePath < rhs->FilePath;
		});
		SubDirectories.insert(where, directory);
		SK_CORE_ASSERT(std::is_sorted(SubDirectories.begin(), SubDirectories.end(), [](const auto& lhs, const auto& rhs) { return lhs->FilePath < rhs->FilePath; }));
	}

	Ref<DirectoryInfo> DirectoryInfo::Create(Ref<DirectoryInfo> parent, const std::string& name, AssetHandle handle)
	{
		std::filesystem::path filePath = parent->FilePath / name;
		return Ref<DirectoryInfo>::Create(parent, filePath, handle);
	}

}
