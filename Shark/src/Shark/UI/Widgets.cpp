#include "skpch.h"
#include "Widgets.h"

#include "Shark/Asset/AssetManager.h"
#include "Shark/Scripting/ScriptEngine.h"
#include "Shark/Scene/Scene.h"
#include "Shark/Scene/Entity.h"
#include "Shark/UI/UICore.h"
#include "Shark/UI/UIUtilities.h"
#include "Shark/Utils/PlatformUtils.h"
#include "Shark/Utils/Utilities.h"

#include <imgui_node_editor.h>

namespace Shark {

	namespace details {

		static std::pair<std::string, bool> GetDisplayName(AssetHandle handle, const UI::InputAssetArgs& args)
		{
			if (!handle)
				return { "", true };

			if (!AssetManager::IsValidAssetHandle(handle))
			{
				return { "Invalid", false };
			}

			auto assetManager = Project::GetEditorAssetManager();

			const bool isMemoryAsset = assetManager->IsMemoryAsset(handle);
			const bool valid = isMemoryAsset || assetManager->HasExistingFilePath(handle);

			if (!args.DisplayName.empty())
				return { std::string(args.DisplayName), valid };

			if (isMemoryAsset)
				return { fmt::format("{}", handle), valid };

			const auto& metadata = assetManager->GetMetadata(handle);
			return { metadata.FilePath.string(), valid };
		}

	}

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
		return SearchAssetPopup({ { assetType } }, assetHandle);
	}

	bool UI::Widgets::SearchAssetPopup(std::span<const AssetType> assetTypes, AssetHandle& assetHandle)
	{
		static UI::TextFilter s_Filter("");
		return ItemSearchPopup(s_Filter, [&assetTypes, &assetHandle](UI::TextFilter& filter, bool clear, bool& changed)
		{
			if (clear)
				assetHandle = AssetHandle::Invalid;

			for (auto assetType : assetTypes)
			{
				if (assetTypes.size() > 1)
				{
					auto typeName = magic_enum::enum_name(assetType);
					ImGui::SeparatorTextEx(0, typeName.data(), typeName.data() + typeName.size(), 0.0f);
				}

				std::vector<AssetHandle> assets = AssetManager::GetAllAssetsOfType(assetType);

				for (AssetHandle handle : assets)
				{
					const AssetMetaData& metadata = Project::GetEditorAssetManager()->GetMetadata(handle);
					if (metadata.IsMemoryAsset || metadata.IsEditorAsset)
						continue;

					std::string name = metadata.FilePath.stem().string();

					if (!filter.PassesFilter(name))
						continue;

					// avoid conflicting ids for selectable
					UI::ScopedID id(metadata.Handle);
					if (ImGui::Selectable(name.c_str()))
					{
						assetHandle = handle;
						changed = true;
					}
				}
			}
		});
	}

	bool UI::Widgets::SearchEntityPopup(Ref<Scene> scene, UUID& entityID, ImGuiID customID)
	{
		static UI::TextFilter s_Filter("");
		return ItemSearchPopup(s_Filter, [scene, &entityID](UI::TextFilter& filter, bool clear, bool& changed)
		{
			if (clear)
				entityID = UUID::Invalid;

			if (!scene)
				return;

			auto entities = scene->GetAllEntitysWith<IDComponent>();
			for (auto ent : entities)
			{
				Entity entity = { ent, scene };

				const auto& tag = entity.Tag();
				if (!filter.PassesFilter(tag))
					continue;

				// avoid conflicting ids for selectable
				UI::ScopedID id(entity.GetUUID());
				if (ImGui::Selectable(tag.c_str(), entityID == entity.GetUUID()))
				{
					entityID = entity.GetUUID();
					changed = true;
				}
			}
		}, customID);
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

	bool UI::Widgets::EntityButton(std::string_view strID, const ImVec2& size, Ref<Scene> scene, UUID& entityID, const EntityButtonArgs& args)
	{
		bool pressed = false;

		{
			//UI::ScopedDisabled diabled(!args.Interactive);

			pressed = ImGui::InvisibleButton(strID.data(), size);
		}

		auto& g = *GImGui;
		if ((g.LastItemData.ItemFlags & ImGuiItemFlags_MixedValue) != 0)
		{
			DrawButton("--", ImVec2(0.5f, 0.5f), GetItemRect());
		}
		else
		{
			if (!scene)
			{
				if (entityID)
					DrawButton(args.DisplayName.empty() ? fmt::to_string(entityID) : args.DisplayName, GetItemRect());
				else
					DrawButton("", GetItemRect());
			}
			else if (entityID && !scene->IsValidEntityID(entityID))
			{
				ScopedColor textColor(ImGuiCol_Text, Colors::Theme::TextError);
				DrawButton("Invalid", GetItemRect());
			}
			else
			{
				Entity entity = scene->TryGetEntityByUUID(entityID);

				std::string_view displayName = "";
				if (entity)
				{
					displayName = args.DisplayName.empty() ? entity.GetName() : args.DisplayName;
				}

				DrawButton(displayName,
						   ImVec2(0.0f, 0.5f),
						   GetItemRect());
			}
		}

		return pressed;
	}

	bool UI::Widgets::InputEntity(std::string_view strID, const ImVec2& size, Ref<Scene> scene, UUID& entityID, const InputEntityArgs& args)
	{
		auto& g = *GImGui;
		ScopedID id(strID);
		bool modified = false;

		Widgets::EntityButton(strID, size, scene, entityID, args);
		modified = Widgets::SearchEntityPopup(scene, entityID);

		if (args.DropType && (g.LastItemData.ItemFlags & ImGuiItemFlags_ReadOnly) == 0)
		{
			if (ImGui::BeginDragDropTarget())
			{
				const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(args.DropType);
				if (payload)
				{
					auto id = *static_cast<const UUID*>(payload->Data);
					if (scene->IsValidEntityID(id))
					{
						entityID = id;
						modified = true;
					}
				}
				ImGui::EndDragDropTarget();
			}
		}

		return modified;
	}

	bool UI::Widgets::InputAsset(std::string_view strID, const ImVec2& size, std::span<const AssetType> assetTypes, AssetHandle& assetHandle, const InputAssetArgs& args)
	{
		bool modified = false;
		ScopedID id(strID);

		ImGui::InvisibleButton(strID.data(), size);

		auto& g = *GImGui;
		if ((g.LastItemData.ItemFlags & ImGuiItemFlags_MixedValue) != 0)
		{
			DrawButton("--", ImVec2(0.5f, 0.5f), GetItemRect());
		}
		else
		{
			auto [displayName, isValid] = details::GetDisplayName(assetHandle, args);
			ScopedColor textColor(ImGuiCol_Text, isValid ? args.TextColor : Colors::Theme::TextError);

			DrawButton(displayName, ImVec2(0.0f, 0.5f), GetItemRect());
		}

		modified = Widgets::SearchAssetPopup(assetTypes, assetHandle);

		if (args.DropType && (g.LastItemData.ItemFlags & ImGuiItemFlags_ReadOnly) == 0)
		{
			if (ImGui::BeginDragDropTarget())
			{
				const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(args.DropType);
				if (payload)
				{
					auto handle = *static_cast<const AssetHandle*>(payload->Data);
					if (Contains(assetTypes, AssetManager::GetAssetType(handle)))
					{
						assetHandle = handle;
						modified = true;
					}
				}
				ImGui::EndDragDropTarget();
			}
		}

		return modified;
	}

}
