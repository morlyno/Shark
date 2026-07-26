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

namespace Shark::UI {

	namespace utils {

		static std::string_view ToStringView(const fmt::memory_buffer& buf)
		{
			auto size = buf.size();
			fmt::detail::assume(size < std::string().max_size());
			return { buf.data(), size };
		}

		static fmt::memory_buffer& GetTempFormatBuffer()
		{
			static fmt::memory_buffer s_Buffer;
			s_Buffer.clear();
			return s_Buffer;
		}

		template<typename... TArgs>
		static std::string_view FormatToTempBuffer(fmt::format_string<TArgs...> fmtString, TArgs&&... args)
		{
			static fmt::memory_buffer s_Buffer;

			s_Buffer.clear();
			fmt::format_to(fmt::appender(s_Buffer), fmtString, std::forward<TArgs>(args)...);
			return ToStringView(s_Buffer);
		}

		static std::optional<std::string_view> GetDisplayName(Ref<Scene> scene, UUID entityID, std::string_view displayName)
		{
			if (!entityID)
				return "";

			if (scene && !scene->IsValidEntityID(entityID))
				return std::nullopt;

			if (!displayName.empty())
				return displayName;

			if (!scene)
				return FormatToTempBuffer("{}", entityID);

			const auto entity = scene->TryGetEntityByUUID(entityID);
			return entity.GetName();
		}

		static std::optional<std::string_view> GetDisplayName(AssetHandle handle, std::string_view displayName)
		{
			if (!handle)
				return "";

			if (!AssetManager::IsValidAssetHandle(handle))
				return std::nullopt;

			auto assetManager = Project::GetEditorAssetManager();

			const bool isMemoryAsset = assetManager->IsMemoryAsset(handle);
			const bool valid = isMemoryAsset || assetManager->HasExistingFilePath(handle);

			if (!valid)
				return std::nullopt;

			if (isMemoryAsset)
				return FormatToTempBuffer("{}", handle);

			const auto& metadata = assetManager->GetMetadata(handle);
			return FormatToTempBuffer("{}", metadata.FilePath);
		}

		static std::optional<std::string_view> GetDisplayName(uint64_t scriptID, std::string_view displayName)
		{
			if (!scriptID)
				return "";

			auto& scriptEngine = ScriptEngine::Get();
			if (!scriptEngine.IsValidScriptID(scriptID))
				return std::nullopt;

			if (!displayName.empty())
				return displayName;

			const auto& metadata = scriptEngine.GetScriptMetadata(scriptID);
			return metadata.FullName;
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

	template<StringLike String, StringLike RangeString>
	bool SearchStringPopupInternal(String& selected, std::span<const RangeString> strings)
	{
		static UI::TextFilter s_Filter("");
		return UI::Widgets::ItemSearchPopup(s_Filter, [&selected, &strings](UI::TextFilter& filter, bool clear, bool& changed)
		{
			if (clear)
				selected = {};

			for (const auto& string : strings)
			{
				if (!s_Filter.PassesFilter(string))
					continue;

				if (ImGui::Selectable(string.data(), string == selected))
				{
					selected = string;
					changed = true;
				}
			}
		});
	}

	template<std::integral Index, StringLike RangeString>
	bool SearchStringPopupInternal(Index& selected, std::span<const RangeString> strings, const Index unselectedIndex)
	{
		static UI::TextFilter s_Filter("");
		return UI::Widgets::ItemSearchPopup(s_Filter, [&selected, &strings, unselectedIndex](UI::TextFilter& filter, bool clear, bool& changed)
		{
			if (clear)
				selected = unselectedIndex;

			for (size_t i = 0; i < strings.size(); i++)
			{
				if (!s_Filter.PassesFilter(strings[i]))
					continue;

				if (ImGui::Selectable(strings[i].data(), i == selected))
				{
					selected = static_cast<Index>(i);
					changed = true;
				}
			}
		});
	}

	bool UI::Widgets::SearchStringPopup(size_t& selected, std::span<const std::string> strings, const size_t unselectedIndex)
	{
		return SearchStringPopupInternal(selected, strings, unselectedIndex);
	}

	bool UI::Widgets::SearchStringPopup(std::string& selected, std::span<const std::string> strings)
	{
		return SearchStringPopupInternal(selected, strings);
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

	bool UI::Widgets::SearchEntityPopup(Ref<Scene> scene, UUID& entityID)
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

	bool UI::Widgets::StringButton(std::string_view selected)
	{
		bool pressed = ImGui::InvisibleButton(selected.data(), { ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight() });

		if ((GImGui->LastItemData.ItemFlags & ImGuiItemFlags_MixedValue) != 0)
		{
			DrawButton("--", ImVec2(0.5f, 0.5f), GetItemRect());
		}
		else
		{
			DrawButton(selected, ImVec2(0.0f, 0.5f), GetItemRect());
		}

		return pressed;
	}

	bool UI::Widgets::EntityButton(Ref<Scene> scene, UUID entityID, const ButtonArgs& args)
	{
		ImGui::PushID(entityID);
		const bool pressed = ImGui::InvisibleButton("Button", {ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight()});
		ImGui::PopID();

		auto& g = *GImGui;
		if ((g.LastItemData.ItemFlags & ImGuiItemFlags_MixedValue) != 0)
		{
			DrawButton("--", ImVec2(0.5f, 0.5f), GetItemRect());
		}
		else
		{
			const auto display = utils::GetDisplayName(scene, entityID, args.DisplayName);

			const bool push = !display || args.TextColor.has_value();
			UI::ScopedColor textColor(ImGuiCol_Text,
									  display ?
									  args.TextColor.value_or(Colors::Theme::Text) :
									  args.ErrorTextColor.value_or(Colors::Theme::TextError),
									  push);

			DrawButton(display.value_or("Invalid"),
					   GetItemRect());
		}

		return pressed;
	}

	bool Widgets::AssetButton(AssetHandle handle, const ButtonArgs& args)
	{
		ImGui::PushID(handle);
		const bool pressed = ImGui::InvisibleButton("Button", { ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight() });
		ImGui::PopID();

		auto& g = *GImGui;
		if ((g.LastItemData.ItemFlags & ImGuiItemFlags_MixedValue) != 0)
		{
			DrawButton("--", ImVec2(0.5f, 0.5f), GetItemRect());
		}
		else
		{
			const auto display = utils::GetDisplayName(handle, args.DisplayName);

			const bool push = !display || args.TextColor.has_value();
			UI::ScopedColor textColor(ImGuiCol_Text,
									  display ?
									  args.TextColor.value_or(Colors::Theme::Text) :
									  args.ErrorTextColor.value_or(Colors::Theme::TextError),
									  push);

			DrawButton(display.value_or("Invalid"),
					   GetItemRect());
		}

		return pressed;
	}

	bool Widgets::ScriptButton(uint64_t& scriptID, const ButtonArgs& args)
	{
		ImGui::PushID(scriptID);
		const bool pressed = ImGui::InvisibleButton("Button", { ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight() });
		ImGui::PopID();

		auto& g = *GImGui;
		if ((g.LastItemData.ItemFlags & ImGuiItemFlags_MixedValue) != 0)
		{
			DrawButton("--", ImVec2(0.5f, 0.5f), GetItemRect());
		}
		else
		{
			const auto display = utils::GetDisplayName(scriptID, args.DisplayName);

			const bool push = !display || args.TextColor.has_value();
			UI::ScopedColor textColor(ImGuiCol_Text,
									  display ?
									  args.TextColor.value_or(Colors::Theme::Text) :
									  args.ErrorTextColor.value_or(Colors::Theme::TextError),
									  push);

			DrawButton(display.value_or("Invalid"),
					   GetItemRect());
		}

		return pressed;
	}

	bool UI::Widgets::SelectString(size_t& selected, std::span<const std::string> strings, size_t unselectIndex)
	{
		StringButton(selected < strings.size() ? strings[selected] : "");
		return SearchStringPopup(selected, strings, unselectIndex);
	}

	bool UI::Widgets::SelectString(std::string& selected, std::span<const std::string> strings)
	{
		StringButton(selected);
		return SearchStringPopup(selected, strings);
	}

	bool Widgets::SelectEntity(Ref<Scene> scene, UUID& entityID, const SelectEntityArgs& args)
	{
		bool modified = false;
		EntityButton(scene, entityID, args);

		if ((GImGui->LastItemData.ItemFlags & ImGuiItemFlags_ReadOnly) == 0)
		{
			modified = Widgets::SearchEntityPopup(scene, entityID);

			if (args.DropType && ImGui::BeginDragDropTarget())
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

	bool Widgets::SelectAsset(AssetType assetType, AssetHandle& assetHandle, const SelectAssetArgs& args)
	{
		return SelectAsset({ { assetType } }, assetHandle, args);
	}

	bool Widgets::SelectAsset(std::span<const AssetType> assetTypes, AssetHandle& assetHandle, const SelectAssetArgs& args)
	{
		bool modified = false;
		AssetButton(assetHandle, args);

		if ((GImGui->LastItemData.ItemFlags & ImGuiItemFlags_ReadOnly) == 0)
		{
			modified = Widgets::SearchAssetPopup(assetTypes, assetHandle);

			if (args.DropType && ImGui::BeginDragDropTarget())
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

	bool Widgets::SelectScript(uint64_t& scriptID, const ButtonArgs& args)
	{
		ScriptButton(scriptID, args);
		return SearchScriptPopup(scriptID);

		// #TODO #scripting #assets add drag drop when script files are better supported
	}

}
