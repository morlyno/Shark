#pragma once

#include "Shark/UI/UICore.h"
#include "Shark/UI/EditorResources.h"
#include "Shark/UI/TextFilter.h"

namespace Shark::UI {

	enum class DialogType
	{
		Open, Save
	};

}

namespace Shark::UI::Widgets {

	template<typename TString>
	bool Search(TString& searchString, const char* hint = "Search...", bool* grabFocus = nullptr, bool clearOnGrab = false);
	bool Search(TextFilter& filter, const char* hint = "Search...", bool* grabFocus = nullptr, bool clearOnGrab = false);

	bool InputFile(DialogType dialogType, std::string& path, const std::string& filters = "*.*|*.*", const std::filesystem::path& defaultPath = {});
	bool InputFile(DialogType dialogType, std::filesystem::path& path, const std::string& filters = "*.*|*.*", const std::filesystem::path& defaultPath = {});
	bool InputDirectory(DialogType dialogType, std::string& path, const std::filesystem::path& defaultPath = {});
	bool InputDirectory(DialogType dialogType, std::filesystem::path& path, const std::filesystem::path& defaultPath = {});
	
	template<typename TFunction>
	bool ItemSearchPopup(UI::TextFilter& search, const TFunction& itemFunction);

	bool SearchAssetPopup(AssetType assetType, AssetHandle& assetHandle);
	bool SearchEntityPopup(UUID& entityID);
	bool SearchScriptPopup(uint64_t& scriptID);

}

#pragma region Template Function Implementation

template<typename TString>
bool Shark::UI::Widgets::Search(TString& searchString, const char* hint, bool* grabFocus, bool clearOnGrab)
{
	UI::PushID();

	ImGui::SuspendLayout();

	bool changed = false;
	bool searching = false;

	const float itemStart = ImGui::GetCursorScreenPos().x;
	const ImVec2 framePadding = ImGui::GetStyle().FramePadding;
	const ImVec2 iconSize = { ImGui::GetTextLineHeight(), ImGui::GetTextLineHeight() };

	UI::ScopedStyle padding(ImGuiStyleVar_FramePadding, ImVec2(iconSize.x + framePadding.x + ImGui::GetStyle().ItemSpacing.x, framePadding.y));

	ImGui::SetNextItemAllowOverlap();
	if constexpr (std::is_same_v<TString, std::string>)
	{
		if (UI::InputText(GenerateID(), &searchString))
		{
			changed = true;
		}
		else if (ImGui::IsItemDeactivatedAfterEdit())
		{
			changed = true;
		}

		searching = searchString.size();
	}
	else
	{
		static_assert(std::is_same_v<decltype(&searchString[0]), char*>,
					  "searchString Parameter must be either std::string& or char*");

		if (UI::InputText(GenerateID(), searchString, std::size(searchString)))
		{
			changed = true;
		}
		else if (ImGui::IsItemDeactivatedAfterEdit())
		{
			changed = true;
		}

		searching = searchString[0] != 0;
	}

	const auto clearString = [&]()
	{
		if constexpr (std::is_same_v<TString, std::string>)
			searchString.clear();
		else
			memset(searchString, 0, std::size(searchString));
		changed = true;
	};

	if (grabFocus && *grabFocus)
	{
		if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !ImGui::IsAnyItemActive())
		{
			//ImGui::SetKeyboardFocusHere(-1);
			ImGui::ActivateItemByID(ImGui::GetItemID());
			if (clearOnGrab)
				clearString();
		}

		if (ImGui::IsItemFocused())
			*grabFocus = false;
	}

	ImGui::SameLine();
	UI::SetCursorScreenPosX(itemStart + framePadding.x);

	ImGui::ResumeLayout();

	ImGui::BeginHorizontal(UI::GenerateID(), ImGui::GetItemRectSize());

	// Search Icon
	{
		UI::ShiftCursorY(framePadding.y);
		UI::Image(EditorResources::SearchIcon, iconSize, { 0, 0 }, { 1, 1 }, { 1, 1, 1, 0.5f });
		UI::ShiftCursorY(-framePadding.y);

		if (!searching)
		{
			UI::ScopedColor text(ImGuiCol_Text, Colors::Theme::TextDarker);
			ImGui::TextUnformatted(hint);
		}
	}

	ImGui::Spring();

	if (searching)
	{
		const float buttonSize = ImGui::GetItemRectSize().y;
		if (ImGui::InvisibleButton(GenerateID(), { buttonSize, buttonSize }))
		{
			clearString();
		}

		UI::DrawImageButton(EditorResources::ClearIcon,
							IM_COL32(160, 160, 160, 200),
							IM_COL32(170, 170, 170, 255),
							IM_COL32(160, 160, 160, 150),
							UI::RectExpand(UI::GetItemRect(), -2.0f, -2.0f));

		ImGui::Spring(-1.0f, framePadding.x);
	}

	ImGui::EndHorizontal();
	UI::PopID();
	return changed;
}

template<typename TFunction>
bool Shark::UI::Widgets::ItemSearchPopup(UI::TextFilter& search, const TFunction& itemFunction)
{
	const auto CalcMaxPopupHeightFromItemCount = [](int items_count)
	{
		ImGuiContext& g = *GImGui;
		if (items_count <= 0)
			return FLT_MAX;
		return (g.FontSize + g.Style.ItemSpacing.y) * items_count - g.Style.ItemSpacing.y + (g.Style.WindowPadding.y * 2);
	};

	ImGuiID popupID = ImGui::GetID("##selectAssetPopup"sv);
	if (ImGui::IsItemActivated())
	{
		search.Clear();
		ImGui::OpenPopup(popupID);
	}

	ImRect lastItemRect = UI::GetItemRect();

	int popup_max_height_in_items = 12;

	ImGui::SetNextWindowSize({ 200, CalcMaxPopupHeightFromItemCount(popup_max_height_in_items) });
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	const bool popupOpen = ImGui::BeginPopupEx(popupID, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar);
	ImGui::PopStyleVar();

	bool clear = false;
	bool changed = false;


	if (popupOpen)
	{
		const auto& style = ImGui::GetStyle();
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - style.WindowPadding.x * 2.0f);
		UI::ShiftCursor(style.WindowPadding);
		if (ImGui::IsWindowAppearing())
			ImGui::SetKeyboardFocusHere();
		UI::Widgets::Search(search);

		{
			UI::ScopedStyle frameBorder(ImGuiStyleVar_FrameBorderSize, 0.0f);
			UI::ScopedStyle frameRounding(ImGuiStyleVar_FrameRounding, 0.0f);
			UI::ScopedColorStack button(ImGuiCol_Button, UI::Colors::WithMultipliedValue(UI::Colors::Theme::Background, 0.9f),
										ImGuiCol_ButtonHovered, UI::Colors::WithMultipliedValue(UI::Colors::Theme::Background, 1.1f),
										ImGuiCol_ButtonActive, UI::Colors::Theme::BackgroundDark);

			if (ImGui::Button("Clear", ImVec2(-1.0f, 0.0f)))
			{
				clear = true;
				ImGui::CloseCurrentPopup();
			}
		}

		UI::ScopedColor childBg(ImGuiCol_ChildBg, UI::Colors::Theme::BackgroundPopup);
		if (ImGui::BeginChild("##selectAssetChild", ImVec2(0, 0), ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_NavFlattened))
		{
			itemFunction(search, clear, changed);
		}
		ImGui::EndChild();

		if (changed)
			ImGui::CloseCurrentPopup();

		ImGui::EndPopup();
	}

	return changed || clear;
}

#pragma endregion
