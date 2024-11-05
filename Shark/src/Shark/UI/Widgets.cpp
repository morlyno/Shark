#include "skpch.h"
#include "Widgets.h"

#include "Shark/UI/UICore.h"
#include "Shark/UI/UIUtilities.h"
#include "Shark/Utils/PlatformUtils.h"

namespace Shark {

	bool UI::Widgets::Search(TextFilter& filter, const char* hint, bool* grabFocus, bool clearOnGrab)
	{
		std::string& buffer = filter.GetTextBuffer();
		if (Search<std::string>(buffer, hint, grabFocus, clearOnGrab))
		{
			filter.Rebuild();
			return true;
		}
		return false;
	}

	bool UI::Widgets::InputFile(DialogType dialogType, std::string& path, const std::string& filters, const std::filesystem::path& defaultPath)
	{
		bool modified = false;

		ImGui::SetNextItemAllowOverlap();
		UI::InputText(UI::GenerateID(), &path, ImGuiInputTextFlags_CallbackCharFilter, UI_INPUT_TEXT_FILTER(":*?\"<>|"));

		const ImVec2 buttonSize = ImGui::CalcTextSize("...") + ImGui::GetStyle().FramePadding * 2.0f;
		ImGui::SameLine(0.0f, 0.0f);
		UI::ShiftCursorX(-buttonSize.x);
		ImGui::InvisibleButton(UI::GenerateID(), buttonSize);

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		if (ImGui::IsItemActivated())
			drawList->AddText(ImGui::GetItemRectMin(), UI::Colors::WithMultipliedValue(UI::Colors::Theme::TextBrighter, 0.9f), "...");
		else if (ImGui::IsItemHovered())
			drawList->AddText(ImGui::GetItemRectMin(), UI::Colors::WithMultipliedValue(UI::Colors::Theme::TextBrighter, 1.2f), "...");
		else
			drawList->AddText(ImGui::GetItemRectMin(), UI::Colors::Theme::Text, "...");

		if (ImGui::IsItemActivated())
		{
			std::filesystem::path result;

			if (dialogType == DialogType::Open)
				result = Platform::OpenFileDialog(String::ToWide(filters), 2, defaultPath);
			else if (dialogType == DialogType::Save)
				result = Platform::SaveFileDialog(String::ToWide(filters), 2, defaultPath);

			if (!result.empty())
			{
				path = result.string();
				modified = true;
			}
		}

		return modified;
	}

	bool UI::Widgets::InputFile(DialogType dialogType, std::filesystem::path& path, const std::string& filters, const std::filesystem::path& defaultPath)
	{
		auto temp = path.string();
		if (InputFile(dialogType, temp, filters, defaultPath))
		{
			path = temp;
			return true;
		}
		return true;
	}

	bool UI::Widgets::InputDirectory(DialogType dialogType, std::string& path, const std::filesystem::path& defaultPath)
	{
		bool modified = false;

		ImGui::SetNextItemAllowOverlap();
		UI::InputText(UI::GenerateID(), &path, ImGuiInputTextFlags_CallbackCharFilter, UI_INPUT_TEXT_FILTER(":*?\"<>|"));

		const ImVec2 buttonSize = ImGui::CalcTextSize("...") + ImGui::GetStyle().FramePadding * 2.0f;
		ImGui::SameLine(0.0f, 0.0f);
		UI::ShiftCursorX(-buttonSize.x);
		ImGui::InvisibleButton(UI::GenerateID(), buttonSize);

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		if (ImGui::IsItemActivated())
			drawList->AddText(ImGui::GetItemRectMin(), UI::Colors::WithMultipliedValue(UI::Colors::Theme::TextBrighter, 0.9f), "...");
		else if (ImGui::IsItemHovered())
			drawList->AddText(ImGui::GetItemRectMin(), UI::Colors::WithMultipliedValue(UI::Colors::Theme::TextBrighter, 1.2f), "...");
		else
			drawList->AddText(ImGui::GetItemRectMin(), UI::Colors::Theme::Text, "...");

		if (ImGui::IsItemHovered())
			ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);

		if (ImGui::IsItemActivated())
		{
			std::filesystem::path result;

			if (dialogType == DialogType::Open)
				result = Platform::OpenDirectoryDialog(defaultPath);
			else if (dialogType == DialogType::Save)
				result = Platform::SaveDirectoryDialog(defaultPath);

			if (!result.empty())
			{
				path = result.string();
				modified = true;
			}
		}

		return modified;
	}

	bool UI::Widgets::InputDirectory(DialogType dialogType, std::filesystem::path& path, const std::filesystem::path& defaultPath)
	{
		auto temp = path.string();
		if (InputDirectory(dialogType, temp, defaultPath))
		{
			path = temp;
			return true;
		}
		return false;
	}

}
