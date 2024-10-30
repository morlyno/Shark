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

#pragma endregion
