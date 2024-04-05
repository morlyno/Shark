#include "skfpch.h"
#include "ContentBrowserItem.h"

#include "Shark/UI/UI.h"
#include "Shark/UI/Theme.h"

#include "Icons.h"
#include "EditorSettings.h"
#include "Panels/ContentBrowser/ContentBrowserPanel.h"

#include "Shark/Debug/Profiler.h"

#include <imgui.h>
#include <imgui_internal.h>

namespace Shark {

	DirectoryInfo::DirectoryInfo(Weak<DirectoryInfo> parent, const std::filesystem::path& directoryPath)
	{
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

	ContentBrowserItem::ContentBrowserItem(Ref<ContentBrowserPanel> context, CBItemType type, const std::filesystem::path& filepath)
		: m_Context(context), m_Type(type), m_Path(context->GetProject()->GetAbsolute(filepath))
	{
		UpdateName();
		UpdateIcon();
		UpdateTypeName();

		if (type == CBItemType::Asset)
		{
			m_AssetHandle = context->GetProject()->GetActiveEditorAssetManager()->GetAssetHandleFromFilepath(m_Path);
		}
	}

	ContentBrowserItem::~ContentBrowserItem()
	{
	}

	void ContentBrowserItem::SetType(CBItemType type)
	{
		m_Type = type;
		UpdateIcon();
		UpdateTypeName();
	}

	CBItemAction ContentBrowserItem::Draw()
	{
		SK_PROFILE_FUNCTION();
		CBItemAction action;

		if (FlagSet(StateFlag::Deleted))
		{
			SK_CORE_ERROR_TAG("UI", "Draw was called on deleted ContentBrowserItem! {}", m_Path);
			action.SetFlag(CBItemActionFlag::RemoveItem);
			return action;
		}

		UI::ScopedID id(this);

		const auto& editorSettings = EditorSettings::Get();
		const float thumbnailSize = editorSettings.ContentBrowser.ThumbnailSize;
		const ImGuiStyle& style = ImGui::GetStyle();

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

		if (ImGui::IsClippedEx(cellRect, 0))
		{
			ImGui::Dummy(cellRect.GetSize());
			return action;
		}

		// Frame
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilled(thumbnailTopLeft, thumbnailBottemRight, UI::ToColor32(Theme::Colors::PropertyField), rounding, ImDrawFlags_RoundCornersTop);
		drawList->AddRectFilled(infoTopLeft, infoBottemRight, UI::ToColor32(Theme::Colors::InfoField), rounding, ImDrawFlags_RoundCornersBottom);

		bool hovered, held;
		bool pressed = ImGui::ButtonBehavior(cellRect, UI::GetID("##thumbnailButton"), &hovered, &held);

		if (hovered)
		{
			if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				action.SetFlag(CBItemActionFlag::Open);
			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
				action.SetFlag(CBItemActionFlag::Select);
		}

		{
			UI::ScopedStyle spacing(ImGuiStyleVar_ItemSpacing, { 0.0f, padding });

			ImGui::InvisibleButton("##thumbnailButton", { thumbnailSize, thumbnailSize });

			if (ImGui::BeginPopupContextItem())
			{
				if (!IsSelected())
				{
					action.SetFlag(CBItemActionFlag::Select);
				}

				std::string path = m_Context.GetRef()->GetProject()->GetRelative(m_Path).string();
				ImGui::Text("Filepath: %s", path.c_str());
				ImGui::Separator();

				if (ImGui::MenuItem("Reload", nullptr, false, m_Type == CBItemType::Asset))
					action.SetFlag(CBItemActionFlag::ReloadAsset);

				ImGui::Separator();

				if (ImGui::MenuItem("Open Externally"))
					action.SetFlag(CBItemActionFlag::OpenExternally);

				if (ImGui::MenuItem("Show in Explorer"))
					action.SetFlag(CBItemActionFlag::OpenInExplorer);

				ImGui::Separator();

				if (ImGui::MenuItem("Rename", "F2"))
					action.SetFlag(CBItemActionFlag::StartRenaming);

				if (ImGui::MenuItem("Delete", "Del"))
					action.Delete();

				ImGui::EndPopup();
			}

			if (m_Type == CBItemType::Asset)
			{
				if (ImGui::BeginDragDropSource())
				{
					AssetHandle handle = m_Context.GetRef()->GetProject()->GetEditorAssetManager()->GetAssetHandleFromFilepath(m_Path);
					ImGui::SetDragDropPayload(UI::DragDropID::Asset, &handle, sizeof(AssetHandle));
					UI::Text(m_Name);
					if (m_Thumbnail)
					{
						const float ratio = (float)m_Thumbnail->GetWidth() / (float)m_Thumbnail->GetHeight();
						UI::Texture(m_Thumbnail, ImVec2(64 * ratio, 64));
					}
					ImGui::EndDragDropSource();
				}
			}

			if (m_Type == CBItemType::Directory)
			{
				if (ImGui::BeginDragDropSource())
				{
					std::string pathString = m_Path.string();
					SK_CORE_VERIFY(pathString.length() < 260);
					char path[260];
					strcpy_s(path, pathString.c_str());
					ImGui::SetDragDropPayload(UI::DragDropID::Directroy, path, sizeof(path));
					ImGui::Text(path);
					ImGui::EndDragDropSource();
				}

				if (ImGui::BeginDragDropTarget())
				{
					const ImGuiPayload* assetPayload = ImGui::AcceptDragDropPayload(UI::DragDropID::Asset);
					if (assetPayload)
					{
						AssetHandle handle = *(AssetHandle*)assetPayload->Data;
						Ref<EditorAssetManager> assetManager = m_Context.GetRef()->GetProject()->GetEditorAssetManager();
						if (assetManager->IsValidAssetHandle(handle))
						{
							action.AssetDropped(handle);
						}
					}

					const ImGuiPayload* directoryPayload = ImGui::AcceptDragDropPayload(UI::DragDropID::Directroy);
					if (directoryPayload)
					{
						std::filesystem::path path = std::filesystem::path((const char*)directoryPayload->Data);
						SK_CORE_ASSERT(FileSystem::Exists(path));
						action.SetFlag(CBItemActionFlag::DirectoryDropped);
						action.m_DroppedDirectory = path;
					}

				}
			}

			const float borderSize = 2.0f;

			Ref<ThumbnailCache> thumbnailCache = m_Context->GetThumbnailCache();

			Ref<Image2D> image = m_Icon->GetImage();
			if (thumbnailCache->HasThumbnail(m_AssetHandle))
				image = thumbnailCache->GetThumbnail(m_AssetHandle);

			ImGuiLayer& imguiLayer = Application::Get().GetImGuiLayer();
			imguiLayer.AddImage(image);
			drawList->AddImageRounded(
				image->GetViewID(),
				thumbnailTopLeft + ImVec2(borderSize, borderSize),
				thumbnailBottemRight - ImVec2(borderSize, borderSize),
				ImVec2(0, 0),
				ImVec2(1, 1),
				IM_COL32_WHITE,
				rounding - 1,
				ImDrawFlags_RoundCornersTop
			);
			imguiLayer.BindFontSampler();

			UI::ScopedIndent indent(style.FramePadding.x);
			ImGui::PushClipRect(infoTopLeft, infoBottemRight, true);
			UI::MoveCursorY(padding);

			if (FlagSet(StateFlag::StartRenaming))
			{
				ImGui::SetKeyboardFocusHere();
				SetFlag(StateFlag::StartRenaming, false);
				SetFlag(StateFlag::Renaming, true);
			}

			if (FlagSet(StateFlag::Renaming))
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
				ImGui::InputText("##rename", m_RenameBuffer, (int)std::size(m_RenameBuffer), ImGuiInputTextFlags_CallbackCharFilter, filter, &invalidInput);
				if (invalidInput)
					action.ErrorPrompt("Invalid Filename");

				if (ImGui::IsItemDeactivatedAfterEdit())
					action.FinishRenaming(fmt::format("{}{}", m_RenameBuffer, FileSystem::GetExtensionString(m_Path)));
				if (ImGui::IsItemDeactivated())
					SetFlag(StateFlag::Renaming, false);
			}
			else
			{
				ImGui::Text(m_Name.c_str());
			}

			// Item Type
			ImVec2 textSize = ImGui::CalcTextSize(m_TypeName.c_str(), m_TypeName.c_str() + m_TypeName.size());
			UI::MoveCursorX(thumbnailSize - textSize.x - style.FramePadding.x * 2.0f);
			ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_Text) * 0.8f, m_TypeName.c_str());
			ImGui::PopClipRect();
		}

		// Outline
		const ImVec2 borderPadding = { 1.0f, 1.0f };
		const bool isSelected = IsSelected();
		const bool coloredBorder = m_IsHovered || isSelected;
		const ImU32 borderColor = UI::ToColor32(coloredBorder ? (isSelected ? Theme::Colors::BorderColored : Theme::Colors::BorderColoredWeak) : ImGui::GetStyleColorVec4(ImGuiCol_Border));
		const ImU32 shadowColor = UI::ToColor32(coloredBorder ? Theme::Colors::ShadowColored : ImGui::GetStyleColorVec4(ImGuiCol_BorderShadow));

		const ImVec2 borderMin = thumbnailTopLeft - borderPadding;
		const ImVec2 borderMax = infoBottemRight + borderPadding;
		float borderSize = style.FrameBorderSize;
		drawList->AddRect(borderMin + ImVec2(1, 1), borderMax + ImVec2(1, 1), shadowColor, rounding, 0, borderSize);
		drawList->AddRect(borderMin, borderMax, borderColor, rounding, 0, borderSize);

		UI::MoveCursorY(style.ItemSpacing.y - padding);
		ImGui::Dummy({ 0, 0 });

		return action;
	}

	void ContentBrowserItem::StartRenaming()
	{
		if (FlagSet(StateFlag::Renaming))
			return;

		strcpy_s(m_RenameBuffer, m_Name.c_str());
		SetFlag(StateFlag::StartRenaming, true);
	}

	bool ContentBrowserItem::Rename(std::string newName, bool addExtension)
	{
		if (!FileSystem::IsValidFilename(newName))
		{
			// TODO(moro): Error prompt
			return false;
		}

		if (addExtension)
			newName = fmt::format("{}{}", newName, FileSystem::GetExtensionString(m_Path));

		std::string errorMsg;
		FileSystem::Rename(m_Path, newName, errorMsg);
		if (!errorMsg.empty())
		{
			// TODO(moro): Error prompt
			return false;
		}

		FileSystem::ReplaceFilename(m_Path, newName);
		UpdateName();
		return true;
	}

	bool ContentBrowserItem::Delete()
	{
		if (!FileSystem::RemoveAll(m_Path))
		{
			// TODO(moro): Error prompt
			return false;
		}

		m_StateFlags = 0;
		SetFlag(StateFlag::Deleted, true);
		return true;
	}

	bool ContentBrowserItem::Move(const std::filesystem::path& newPath)
	{
		SK_CORE_ASSERT(m_Path.filename() != newPath.filename());
		if (!FileSystem::Move(m_Path, newPath))
		{
			// TODO(moro): Error prompt
			return false;
		}

		m_Path = newPath;
		return true;
	}

	void ContentBrowserItem::SetFlag(StateFlag flag, bool set)
	{
		if (set)
			m_StateFlags |= (uint32_t)flag;
		else
			m_StateFlags &= ~(uint32_t)flag;
	}

	bool ContentBrowserItem::FlagSet(StateFlag flag) const
	{
		return m_StateFlags & (uint32_t)flag;
	}

	std::string ContentBrowserItem::GetTypeString() const
	{
		if (m_Type == CBItemType::Directory)
			return "Directory";

		if (m_Type == CBItemType::Asset)
		{
			Ref<EditorAssetManager> assetManager = m_Context.GetRef()->GetProject()->GetEditorAssetManager();
			const AssetHandle handle = assetManager->GetAssetHandleFromFilepath(m_Path);
			const AssetType assetType = assetManager->GetAssetType(handle);
			return ToString(assetType);
		}

		return "File";
	}

	void ContentBrowserItem::UpdateIcon()
	{
		switch (m_Type)
		{
			case CBItemType::Directory:
			{
				m_Icon = m_Context.GetRef()->GetDirectoryIcon();
				break;
			}
			case CBItemType::Asset:
			{
				m_Icon = m_Context.GetRef()->GetFileIcon(m_Path);
				break;
			}
		}
	}

	void ContentBrowserItem::UpdateName()
	{
		if (m_Type == CBItemType::Directory)
			m_Name = m_Path.filename().string();
		else
			m_Name = m_Path.stem().string();
	}

	void ContentBrowserItem::UpdateTypeName()
	{
		m_TypeName = GetTypeString();
	}

}
