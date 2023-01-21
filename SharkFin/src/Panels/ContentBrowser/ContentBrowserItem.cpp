#include "skfpch.h"
#include "ContentBrowserItem.h"

#include "Shark/Asset/ResourceManager.h"
#include "Shark/UI/UI.h"
#include "Shark/UI/Theme.h"
#include "Shark/Editor/Icons.h"
#include "Shark/Editor/EditorSettings.h"

#include "Panels/ContentBrowser/ContentBrowserPanel.h"

#include <imgui.h>
#include <imgui_internal.h>

namespace Shark {

	ContentBrowserItem::ContentBrowserItem(CBItemType type, AssetHandle handle, const std::string& name, Ref<Image2D> icon)
		: m_Type(type), m_Handle(handle), m_Name(name), m_Icon(icon)
	{
	}

	ContentBrowserItem::ContentBrowserItem(Ref<DirectoryInfo> directory)
		: m_Type(CBItemType::Directory), m_Handle(directory->Handle), m_Name(directory->FilePath.stem().string()), m_Icon(Icons::FolderIcon)
	{
	}

	ContentBrowserItem::ContentBrowserItem(const AssetMetaData& metadata, Ref<Image2D> icon)
		: m_Type(CBItemType::Asset), m_Handle(metadata.Handle), m_Name(metadata.FilePath.stem().string()), m_Icon(icon)
	{
	}

	ContentBrowserItem::~ContentBrowserItem()
	{
	}

	CBItemAction::Flags ContentBrowserItem::Draw()
	{
		SK_CORE_ASSERT(IsStateSet(State::Deleted) == false);
		if (IsStateSet(State::Deleted))
		{
			SK_CORE_ERROR_TAG("UI", "Tried to draw deleted ContentBrowserItem! ({0})", m_Name);
			return CBItemAction::ReloadRequired;
		}

		UI::ScopedID id(this);
		CBItemAction::Flags result = CBItemAction::None;

		const auto& editorSettings = EditorSettings::Get();
		const float thumbnailSize = editorSettings.ContentBrowser.ThumbnailSize;
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

		bool hovered, held;
		bool pressed = ImGui::ButtonBehavior(cellRect, UI::GetID("##thumbnailButton"), &hovered, &held);

		if (hovered)
		{
			if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				result |= CBItemAction::Open;
			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			{
				result |= CBItemAction::Selected;
				SetState(State::Selcted, true);
			}
		}

		{
			UI::ScopedStyle spacing(ImGuiStyleVar_ItemSpacing, { 0.0f, padding });

			ImGui::InvisibleButton("##thumbnailButton", { thumbnailSize, thumbnailSize });

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
					ImGui::Text(m_Name);
					if (m_Thumbnail)
					{
						const float ratio = (float)m_Thumbnail->GetWidth() / (float)m_Thumbnail->GetHeight();
						ImGui::Image(m_Thumbnail->GetViewID(), ImVec2(64 * ratio, 64));
					}
					ImGui::EndDragDropSource();
				}
			}

			if (m_Type == CBItemType::Directory)
			{
				if (ImGui::BeginDragDropSource())
				{
					auto& cbPanel = ContentBrowserPanel::Get();
					Ref<DirectoryInfo> directory = cbPanel.GetDirectory(m_Handle);
					std::string absolutPath = cbPanel.GetProject()->GetAbsolue(directory->FilePath).string();
					char path[260];
					strcpy_s(path, absolutPath.c_str());
					ImGui::SetDragDropPayload(UI_DRAGDROP_DIRECTORY_TYPE, path, sizeof(path));
					ImGui::Text(path);
					ImGui::EndDragDropSource();
				}
			}

			const float borderSize = 2.0f;
			drawList->AddImageRounded(
				m_Thumbnail ? m_Thumbnail->GetViewID() : m_Icon->GetViewID(),
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
				auto filter = [](ImGuiInputTextCallbackData* data) -> int
				{
					if (FileSystem::InvalidCharactersW.find(data->EventChar) != std::wstring_view::npos)
					{
						*(bool*)data->UserData = true;
						return 1;
					}
					return 0;
				};

				bool invalidInput = false;
				ImGui::InputText("##rename", RenameBuffer, (int)std::size(RenameBuffer), ImGuiInputTextFlags_CallbackCharFilter, filter, &invalidInput);
				if (invalidInput)
					result |= CBItemAction::InvalidFilenameInput;

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
		const bool isSelected = IsStateSet(State::Selcted);
		const bool coloredBorder = m_IsHovered || IsStateSet(State::Selcted);
		const ImU32 borderColor = UI::ToColor32(coloredBorder ? (isSelected ? Theme::Colors::BorderColored : Theme::Colors::BorderColoredWeak) : ImGui::GetStyleColorVec4(ImGuiCol_Border));
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

		// TODO(moro): error prompt
		if (name.empty())
			return;

		ContentBrowserPanel::Get().SkipNextFileEvents();

		if (m_Type == CBItemType::Directory)
		{
			ContentBrowserPanel& cbPanel = ContentBrowserPanel::Get();
 			Ref<DirectoryInfo> directory = cbPanel.GetDirectory(m_Handle);
			std::filesystem::path oldPath = cbPanel.GetProject()->Directory / directory->FilePath;
			std::filesystem::path newPath = oldPath;
			newPath.replace_filename(name);
			newPath.replace_extension();

			std::error_code error;
			std::filesystem::rename(oldPath, newPath, error);
			if (error) // TODO(moro): error prompt
				return;

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
		if (error) // TODO(moro): error prompt
			return;

		m_Name = newPath.stem().string();
	}

	void ContentBrowserItem::Delete()
	{
		SetState(State::Deleted, true);

		if (m_Type == CBItemType::Directory)
		{
			auto& cb = ContentBrowserPanel::Get();
			Ref<DirectoryInfo> directory = cb.GetDirectory(m_Handle);
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

		return ToString(metadata.Type);
	}

	DirectoryInfo::DirectoryInfo(Ref<DirectoryInfo> parent, const std::filesystem::path& filePath, AssetHandle handle)
		: Parent(parent), FilePath(ContentBrowserPanel::Get().GetProject()->GetRelative(filePath)), Name(filePath.stem().string()), Handle(handle)
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

		auto project = ContentBrowserPanel::Get().GetProject();

		for (const auto& entry : std::filesystem::directory_iterator(project->Directory / FilePath))
		{
			if (entry.is_directory())
			{
				AssetHandle handle = AssetHandle::Generate();
				std::filesystem::path filePath = project->GetRelative(entry.path());
				Ref<DirectoryInfo> directory = Ref<DirectoryInfo>::Create(this, filePath, handle);
				SubDirectories.emplace_back(directory);
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

	void DirectoryInfo::AddAsset(AssetHandle handle)
	{
		Assets.emplace_back(handle);
	}

}
