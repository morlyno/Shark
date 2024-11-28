#include "skpch.h"
#include "Widgets.h"

#include "Shark/Core/SelectionManager.h"
#include "Shark/Asset/AssetManager.h"
#include "Shark/Scripting/ScriptEngine.h"
#include "Shark/Scene/Scene.h"
#include "Shark/Scene/Entity.h"
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

	bool UI::Widgets::SearchAssetPopup(AssetType assetType, AssetHandle& assetHandle)
	{
		static UI::TextFilter s_Filter("");
		return ItemSearchPopup(s_Filter, [assetType, &assetHandle](UI::TextFilter& filter, bool clear, bool& changed)
		{
			if (clear)
				assetHandle = AssetHandle::Invalid;

			std::vector<AssetHandle> assets = AssetManager::GetAllAssetsOfType(assetType);

			for (AssetHandle handle : assets)
			{
				const AssetMetaData& metadata = Project::GetActiveEditorAssetManager()->GetMetadata(handle);
				if (metadata.IsMemoryAsset || metadata.IsEditorAsset)
					continue;

				std::string name = metadata.FilePath.stem().string();

				if (!filter.PassesFilter(name))
					continue;

				if (ImGui::Selectable(name.c_str()))
				{
					assetHandle = handle;
					changed = true;
				}
			}
		});
	}

	bool UI::Widgets::SearchEntityPopup(UUID& entityID)
	{
		static UI::TextFilter s_Filter("");
		return ItemSearchPopup(s_Filter, [&entityID](UI::TextFilter& filter, bool clear, bool& changed)
		{
			if (clear)
				entityID = UUID::Invalid;

			Ref<Scene> scene = SelectionManager::GetActiveScene();

			auto entities = scene->GetAllEntitysWith<IDComponent>();
			for (auto ent : entities)
			{
				Entity entity = { ent, scene };

				const auto& tag = entity.Tag();
				if (!filter.PassesFilter(tag))
					continue;

				if (ImGui::Selectable(tag.c_str()))
				{
					entityID = entity.GetUUID();
					changed = true;
				}
			}
		});
	}

	bool UI::Widgets::SearchScriptPopup(uint64_t& scriptID)
	{
		static UI::TextFilter s_Filter("");
		return ItemSearchPopup(s_Filter, [&scriptID](UI::TextFilter& filter, bool clear, bool& changed)
		{
			auto& scriptEngine = ScriptEngine::Get();
			const auto& scripts = scriptEngine.GetScripts();

			if (clear)
				scriptID = 0;

			for (const auto& [id, metadata] : scripts)
			{
				if (!s_Filter.PassesFilter(metadata.FullName))
					continue;

				if (ImGui::Selectable(metadata.FullName.c_str()))
				{
					scriptID = id;
					changed = true;
				}
			}
		});
	}

}
