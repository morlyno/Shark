#include "skpch.h"
#include "Controls.h"

#include "Shark/Core/Project.h"
#include "Shark/Core/SelectionManager.h"
#include "Shark/Asset/AssetManager.h"
#include "Shark/Scene/Entity.h"

#include "Shark/UI/Widgets.h"
#include "Shark/UI/TextFilter.h"
#include "Shark/UI/EditorResources.h"

#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>

namespace Shark::UI {

	namespace utils {

		static void GridSeparator()
		{
			ImGuiContext& g = *GImGui;
			ImGuiWindow* window = g.CurrentWindow;

			ImGuiTable* table = ImGui::GetCurrentTable();
			ImGuiTableColumn& collumn = table->Columns[table->CurrentColumn];
			float x1 = collumn.MinX;
			float x2 = collumn.MaxX;

			// FIXME-WORKRECT: old hack (#205) until we decide of consistent behavior with WorkRect/Indent and Separator
			if (g.GroupStack.Size > 0 && g.GroupStack.back().WindowID == window->ID)
				x1 += window->DC.Indent.x;

			// We don't provide our width to the layout so that it doesn't get feed back into AutoFit
			float thickness_draw = 1.0f;
			float thickness_layout = 0.0f;

			const ImRect bb(ImVec2(x1, window->DC.CursorPos.y), ImVec2(x2, window->DC.CursorPos.y + thickness_draw));
			window->DrawList->AddLine(bb.Min, ImVec2(bb.Max.x, bb.Min.y), ImGui::GetColorU32(ImGuiCol_Separator));
		}

		template<typename T>
		static bool CheckboxFlagsT(const char* label, T* flags, T flags_value)
		{
			bool all_on = (*flags & flags_value) == flags_value;
			bool any_on = (*flags & flags_value) != 0;
			bool pressed;
			if (!all_on && any_on)
			{
				ImGuiContext& g = *GImGui;
				ImGuiItemFlags backup_item_flags = g.CurrentItemFlags;
				g.CurrentItemFlags |= ImGuiItemFlags_MixedValue;
				pressed = ImGui::Checkbox(label, &all_on);
				g.CurrentItemFlags = backup_item_flags;
			}
			else
			{
				pressed = ImGui::Checkbox(label, &all_on);

			}
			if (pressed)
			{
				if (all_on)
					*flags |= flags_value;
				else
					*flags &= ~flags_value;
			}
			return pressed;
		}

		static float CalcMaxPopupHeightFromItemCount(int items_count)
		{
			ImGuiContext& g = *GImGui;
			if (items_count <= 0)
				return FLT_MAX;
			return (g.FontSize + g.Style.ItemSpacing.y) * items_count - g.Style.ItemSpacing.y + (g.Style.WindowPadding.y * 2);
		}

	}

	bool ControlHelperBegin(ImGuiID id)
	{
		if (!ImGui::GetCurrentTable())
			return false;

		ImGui::PushID(id);
		ImGui::TableNextRow();
		ImGui::TableNextColumn();

		if (ImGui::TableGetRowIndex() > 0)
		{
			utils::GridSeparator();
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
		}

		ImGui::AlignTextToFramePadding();

		return true;
	}

	void ControlHelperEnd()
	{
		SK_CORE_ASSERT(ImGui::GetCurrentTable());
		ImGui::PopID();
	}

	bool ControlHeader(std::string_view label, bool openByDefault, bool spanColumns)
	{
		ImGuiTreeNodeFlags treeFlags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap;
		if (openByDefault)
			treeFlags |= ImGuiTreeNodeFlags_DefaultOpen;
		if (spanColumns)
			treeFlags |= ImGuiTreeNodeFlags_SpanAllColumns;

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		return ImGui::TreeNodeEx(label.data(), treeFlags);
	}

	bool Control(std::string_view label, bool& val)
	{
		if (!ControlHelperBegin(ImGui::GetID(label)))
			return false;

		ImGui::Text(label);
		ImGui::TableNextColumn();

		const bool modified = UI::Checkbox(GenerateID(), &val);

		ControlHelperEnd();
		return modified;
	}

	bool Control(std::string_view label, float& val, float speed, float min, float max, const char* fmt)
	{
		if (!ControlHelperBegin(ImGui::GetID(label)))
			return false;

		ImGui::Text(label);
		ImGui::TableNextColumn();

		ImGui::SetNextItemWidth(-1.0f);
		const bool modified = UI::DragFloat(GenerateID(), &val, speed, min, max, fmt);

		ControlHelperEnd();
		return modified;
	}

	bool Control(std::string_view label, double& val, float speed, double min, double max, const char* fmt)
	{
		if (!ControlHelperBegin(ImGui::GetID(label)))
			return false;

		ImGui::Text(label);
		ImGui::TableNextColumn();

		ImGui::SetNextItemWidth(-1.0f);
		const bool modified = UI::DragDouble(GenerateID(), &val, speed, (float)min, (float)max, fmt);

		ControlHelperEnd();
		return modified;
	}

	bool ControlSlider(std::string_view label, float& val, float min, float max, const char* fmt)
	{
		if (!ControlHelperBegin(ImGui::GetID(label)))
			return false;

		ImGui::Text(label);
		ImGui::TableNextColumn();

		ImGui::SetNextItemWidth(-1.0f);
		const bool modified = UI::SliderFloat(GenerateID(), &val, min, max, fmt);

		ControlHelperEnd();
		return modified;
	}

	bool ControlSlider(std::string_view label, int32_t& val, int32_t min, int32_t max, const char* fmt)
	{
		if (!ControlHelperBegin(ImGui::GetID(label)))
			return false;

		ImGui::Text(label);
		ImGui::TableNextColumn();

		ImGui::SetNextItemWidth(-1.0f);
		const bool modified = UI::SliderInt32(GenerateID(), &val, min, max, fmt);

		ControlHelperEnd();
		return modified;
	}

	bool ControlSlider(std::string_view label, uint32_t& val, uint32_t min, uint32_t max, const char* fmt)
	{
		if (!ControlHelperBegin(ImGui::GetID(label)))
			return false;

		ImGui::Text(label);
		ImGui::TableNextColumn();

		ImGui::SetNextItemWidth(-1.0f);
		const bool modified = UI::SliderUInt32(GenerateID(), &val, min, max, fmt);

		ControlHelperEnd();
		return modified;
	}

	bool Control(std::string_view label, int8_t& val, float speed, int8_t min, int8_t max, const char* fmt)
	{
		if (!ControlHelperBegin(ImGui::GetID(label)))
			return false;

		ImGui::Text(label);
		ImGui::TableNextColumn();

		ImGui::SetNextItemWidth(-1.0f);
		const bool modified = UI::DragInt8(GenerateID(), &val, speed, min, max, fmt);

		ControlHelperEnd();
		return modified;
	}

	bool Control(std::string_view label, int16_t& val, float speed, int16_t min, int16_t max, const char* fmt)
	{
		if (!ControlHelperBegin(ImGui::GetID(label)))
			return false;

		ImGui::Text(label);
		ImGui::TableNextColumn();

		ImGui::SetNextItemWidth(-1.0f);
		const bool modified = UI::DragInt16(GenerateID(), &val, speed, min, max, fmt);

		ControlHelperEnd();
		return modified;
	}

	bool Control(std::string_view label, int32_t& val, float speed, int32_t min, int32_t max, const char* fmt)
	{
		if (!ControlHelperBegin(ImGui::GetID(label)))
			return false;

		ImGui::Text(label);
		ImGui::TableNextColumn();

		ImGui::SetNextItemWidth(-1.0f);
		const bool modified = UI::DragInt32(GenerateID(), &val, speed, min, max, fmt);

		ControlHelperEnd();
		return modified;
	}

	bool Control(std::string_view label, int64_t& val, float speed, int64_t min, int64_t max, const char* fmt)
	{
		if (!ControlHelperBegin(ImGui::GetID(label)))
			return false;

		ImGui::Text(label);
		ImGui::TableNextColumn();

		ImGui::SetNextItemWidth(-1.0f);
		const bool modified = UI::DragInt64(GenerateID(), &val, speed, min, max, fmt);

		ControlHelperEnd();
		return modified;
	}

	bool Control(std::string_view label, uint8_t& val, float speed, uint8_t min, uint8_t max, const char* fmt)
	{
		if (!ControlHelperBegin(ImGui::GetID(label)))
			return false;

		ImGui::Text(label);
		ImGui::TableNextColumn();

		ImGui::SetNextItemWidth(-1.0f);
		const bool modified = UI::DragUInt8(GenerateID(), &val, speed, min, max, fmt);

		ControlHelperEnd();
		return modified;
	}

	bool Control(std::string_view label, uint16_t& val, float speed, uint16_t min, uint16_t max, const char* fmt)
	{
		if (!ControlHelperBegin(ImGui::GetID(label)))
			return false;

		ImGui::Text(label);
		ImGui::TableNextColumn();

		ImGui::SetNextItemWidth(-1.0f);
		const bool modified = UI::DragUInt16(GenerateID(), &val, speed, min, max, fmt);

		ControlHelperEnd();
		return modified;
	}

	bool Control(std::string_view label, uint32_t& val, float speed, uint32_t min, uint32_t max, const char* fmt, ImGuiSliderFlags flags)
	{
		if (!ControlHelperBegin(ImGui::GetID(label)))
			return false;

		ImGui::Text(label);
		ImGui::TableNextColumn();

		ImGui::SetNextItemWidth(-1.0f);
		const bool modified = UI::DragUInt32(GenerateID(), &val, speed, min, max, fmt);

		ControlHelperEnd();
		return modified;
	}

	bool Control(std::string_view label, uint64_t& val, float speed, uint64_t min, uint64_t max, const char* fmt)
	{
		if (!ControlHelperBegin(ImGui::GetID(label)))
			return false;

		ImGui::Text(label);
		ImGui::TableNextColumn();

		ImGui::SetNextItemWidth(-1.0f);
		const bool modified = UI::DragUInt64(GenerateID(), &val, speed, min, max, fmt);

		ControlHelperEnd();
		return modified;
	}

	bool Control(std::string_view label, glm::vec2& val, float speed, float min, float max, const char* fmt)
	{
		if (!ControlHelperBegin(ImGui::GetID(label)))
			return false;

		ImGui::Text(label);
		ImGui::TableNextColumn();

		ImGui::SetNextItemWidth(-1.0f);
		const bool modified = UI::DragFloat2(GenerateID(), glm::value_ptr(val), speed, min, max, fmt);

		ControlHelperEnd();
		return modified;
	}

	bool Control(std::string_view label, glm::vec3& val, float speed, float min, float max, const char* fmt)
	{
		if (!ControlHelperBegin(ImGui::GetID(label)))
			return false;

		ImGui::Text(label);
		ImGui::TableNextColumn();

		ImGui::SetNextItemWidth(-1.0f);
		const bool modified = UI::DragFloat3(GenerateID(), glm::value_ptr(val), speed, min, max, fmt);

		ControlHelperEnd();
		return modified;
	}

	bool Control(std::string_view label, glm::vec4& val, float speed, float min, float max, const char* fmt)
	{
		if (!ControlHelperBegin(ImGui::GetID(label)))
			return false;

		ImGui::Text(label);
		ImGui::TableNextColumn();

		ImGui::SetNextItemWidth(-1.0f);
		const bool modified = UI::DragFloat4(GenerateID(), glm::value_ptr(val), speed, min, max, fmt);

		ControlHelperEnd();
		return modified;
	}

	bool Control(std::string_view label, glm::ivec2& val, float speed, int32_t min, int32_t max, const char* fmt)
	{
		if (!ControlHelperBegin(ImGui::GetID(label)))
			return false;

		ImGui::Text(label);
		ImGui::TableNextColumn();

		ImGui::SetNextItemWidth(-1.0f);
		const bool modified = UI::DragScalarN(GenerateID(), ImGuiDataType_S32, glm::value_ptr(val), val.length(), speed, &min, &max, fmt, 0);

		ControlHelperEnd();
		return modified;
	}

	bool Control(std::string_view label, glm::ivec3& val, float speed, int32_t min, int32_t max, const char* fmt)
	{
		if (!ControlHelperBegin(ImGui::GetID(label)))
			return false;

		ImGui::Text(label);
		ImGui::TableNextColumn();

		ImGui::SetNextItemWidth(-1.0f);
		const bool modified = UI::DragScalarN(GenerateID(), ImGuiDataType_S32, glm::value_ptr(val), val.length(), speed, &min, &max, fmt, 0);

		ControlHelperEnd();
		return modified;
	}

	bool Control(std::string_view label, glm::ivec4& val, float speed, int32_t min, int32_t max, const char* fmt)
	{
		if (!ControlHelperBegin(ImGui::GetID(label)))
			return false;

		ImGui::Text(label);
		ImGui::TableNextColumn();

		ImGui::SetNextItemWidth(-1.0f);
		const bool modified = UI::DragScalarN(GenerateID(), ImGuiDataType_S32, glm::value_ptr(val), val.length(), speed, &min, &max, fmt, 0);

		ControlHelperEnd();
		return modified;
	}

	bool Control(std::string_view label, glm::uvec2& val, float speed, uint32_t min, uint32_t max, const char* fmt)
	{
		if (!ControlHelperBegin(ImGui::GetID(label)))
			return false;

		ImGui::Text(label);
		ImGui::TableNextColumn();

		ImGui::SetNextItemWidth(-1.0f);
		const bool modified = UI::DragScalarN(GenerateID(), ImGuiDataType_U32, glm::value_ptr(val), val.length(), speed, &min, &max, fmt, 0);

		ControlHelperEnd();
		return modified;
	}

	bool Control(std::string_view label, glm::uvec3& val, float speed, uint32_t min, uint32_t max, const char* fmt)
	{
		if (!ControlHelperBegin(ImGui::GetID(label)))
			return false;

		ImGui::Text(label);
		ImGui::TableNextColumn();

		ImGui::SetNextItemWidth(-1.0f);
		const bool modified = UI::DragScalarN(GenerateID(), ImGuiDataType_U32, glm::value_ptr(val), val.length(), speed, &min, &max, fmt, 0);

		ControlHelperEnd();
		return modified;
	}

	bool Control(std::string_view label, glm::uvec4& val, float speed, uint32_t min, uint32_t max, const char* fmt)
	{
		if (!ControlHelperBegin(ImGui::GetID(label)))
			return false;

		ImGui::Text(label);
		ImGui::TableNextColumn();

		ImGui::SetNextItemWidth(-1.0f);
		const bool modified = UI::DragScalarN(GenerateID(), ImGuiDataType_U32, glm::value_ptr(val), val.length(), speed, &min, &max, fmt, 0);

		ControlHelperEnd();
		return modified;
	}

	bool Control(std::string_view label, glm::mat4& matrix, float speed, float min, float max, const char* fmt)
	{
		if (!ControlHelperBegin(ImGui::GetID(label)))
			return false;

		ImGui::Text(label);
		ImGui::TableNextColumn();

		bool modified = false;

		ImGui::TableSetColumnIndex(1);
		ImGui::SetNextItemWidth(-1.0f);
		modified |= UI::DragFloat4(GenerateID(), glm::value_ptr(matrix[0]), speed, min, max, fmt);

		ImGui::SetNextItemWidth(-1.0f);
		modified |= UI::DragFloat4(GenerateID(), glm::value_ptr(matrix[1]), speed, min, max, fmt);

		ImGui::SetNextItemWidth(-1.0f);
		modified |= UI::DragFloat4(GenerateID(), glm::value_ptr(matrix[2]), speed, min, max, fmt);

		ImGui::SetNextItemWidth(-1.0f);
		modified |= UI::DragFloat4(GenerateID(), glm::value_ptr(matrix[3]), speed, min, max, fmt);

		ControlHelperEnd();
		return modified;
	}

	bool Control(std::string_view label, char* buffer, uint64_t bufferSize)
	{
		if (!ControlHelperBegin(ImGui::GetID(label)))
			return false;

		ImGui::Text(label);
		ImGui::TableNextColumn();

		ImGui::SetNextItemWidth(-1.0f);
		const bool modified = UI::InputText(GenerateID(), buffer, bufferSize);

		ControlHelperEnd();
		return modified;
	}

	bool Control(std::string_view label, const char* buffer, uint64_t bufferSize)
	{
		if (!ControlHelperBegin(ImGui::GetID(label)))
			return false;

		ImGui::Text(label);
		ImGui::TableNextColumn();

		ImGui::SetNextItemWidth(-1.0f);
		const bool modified = UI::InputText(GenerateID(), (char*)buffer, bufferSize, ImGuiItemFlags_ReadOnly);

		ControlHelperEnd();
		return modified;
	}

	bool Control(std::string_view label, std::string& val)
	{
		if (!ControlHelperBegin(ImGui::GetID(label)))
			return false;

		ImGui::Text(label);
		ImGui::TableNextColumn();

		ImGui::SetNextItemWidth(-1.0f);
		const bool modified = UI::InputText(GenerateID(), &val);

		ControlHelperEnd();
		return modified;
	}

	bool Control(std::string_view label, const std::string& val)
	{
		if (!ControlHelperBegin(ImGui::GetID(label)))
			return false;

		ImGui::Text(label);
		ImGui::TableNextColumn();

		ImGui::SetNextItemWidth(-1.0f);
		const bool modified = UI::InputText(GenerateID(), (char*)val.data(), val.size(), ImGuiInputTextFlags_ReadOnly);

		ControlHelperEnd();
		return modified;
	}

	bool ControlColor(std::string_view label, glm::vec3& color)
	{
		if (!ControlHelperBegin(ImGui::GetID(label)))
			return false;

		ImGui::Text(label);
		ImGui::TableNextColumn();

		ImGui::SetNextItemWidth(-1.0f);
		const bool modified = UI::ColorEdit3(GenerateID(), glm::value_ptr(color));

		ControlHelperEnd();
		return modified;
	}

	bool ControlColor(std::string_view label, glm::vec4& color)
	{
		if (!ControlHelperBegin(ImGui::GetID(label)))
			return false;

		ImGui::Text(label);
		ImGui::TableNextColumn();

		ImGui::SetNextItemWidth(-1.0f);
		const bool modified = UI::ColorEdit4(GenerateID(), glm::value_ptr(color));

		ControlHelperEnd();
		return modified;
	}

	bool ControlCombo(std::string_view label, bool& value, const std::string_view falseValue, const std::string_view trueValue)
	{
		if (!ControlHelperBegin(ImGui::GetID(label)))
			return false;

		ImGui::Text(label);
		ImGui::TableNextColumn();

		bool changed = false;
		std::string_view preview = value ? trueValue : falseValue;
		ImGui::SetNextItemWidth(-1.0f);

		ImGuiContext& g = *GImGui;
		const bool isMixedValue = g.CurrentItemFlags & ImGuiItemFlags_MixedValue;

		ImGui::SetNextItemWidth(-1.0f);
		if (UI::BeginCombo("##combo", isMixedValue ? nullptr : preview.data()))
		{
			if (ImGui::Selectable(falseValue.data(), value == false))
			{
				value = false;
				changed = true;
			}

			if (ImGui::Selectable(trueValue.data(), value == true))
			{
				value = true;
				changed = true;
			}
			UI::EndCombo();
		}

		if (isMixedValue)
			UI::DrawTextAligned("--", ImVec2(0.5f, 0.5f), UI::GetItemRect());

		ControlHelperEnd();
		return changed;
	}

	bool ControlAssetUnsave(std::string_view label, AssetHandle& assetHandle, const char* dragDropType)
	{
		if (!ControlHelperBegin(ImGui::GetID(label)))
			return false;

		ImGui::Text(label);
		ImGui::TableNextColumn();

		bool changed = false;
		const auto& metadata = Project::GetActiveEditorAssetManager()->GetMetadata(assetHandle);

		std::string name;
		if (metadata.IsMemoryAsset)
			name = fmt::format("0x{:x}", assetHandle);
		else
			name = metadata.FilePath.string();

		ImGui::SetNextItemWidth(-1.0f);
		ImGui::SetNextItemAllowOverlap();
		UI::InputText(GenerateID(), name.data(), name.length(), ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_NoHorizontalScroll);

		if (dragDropType)
		{
			if (ImGui::BeginDragDropTarget())
			{
				const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(dragDropType);
				if (payload)
				{
					const AssetHandle handle = *(AssetHandle*)payload->Data;
					assetHandle = handle;
					changed = true;
				}
				ImGui::EndDragDropTarget();
			}
		}

		bool clear = false;

		{
			const auto& style = ImGui::GetStyle();
			const float fontSize = ImGui::GetFontSize();
			const ImVec2 buttonSize = { fontSize + style.FramePadding.x * 2.0f, fontSize + style.FramePadding.y * 2.0f };

			ImGui::SameLine(0, 0);
			ShiftCursorX(-buttonSize.x);

			clear = ImGui::InvisibleButton("clear_button", buttonSize);
			UI::DrawImageButton(EditorResources::ClearIcon,
								UI::Colors::WithMultipliedValue(UI::Colors::Theme::Text, 0.9f),
								UI::Colors::WithMultipliedValue(UI::Colors::Theme::Text, 1.2f),
								UI::Colors::Theme::TextDarker,
								UI::RectExpand(UI::GetItemRect(), -style.FramePadding));
		}

		if (clear)
		{
			assetHandle = AssetHandle::Invalid;
		}

		ControlHelperEnd();
		return changed || clear;
	}

	// TODO(moro): Ignores ImGuiItemFlag_Readonly
	bool ControlAsset(std::string_view label, AssetType assetType, AssetHandle& assetHandle)
	{
		AssetControlSettings settings;
		return ControlAsset(label, assetType, assetHandle, settings);
	}

	bool ControlAsset(std::string_view label, std::string_view name, AssetType assetType, AssetHandle& assetHandle)
	{
		AssetControlSettings settings;
		settings.DisplayName = name;
		return ControlAsset(label, assetType, assetHandle, settings);
	}

	bool ControlAsset(std::string_view label, AssetType assetType, AssetHandle& assetHandle, const AssetControlSettings& settings)
	{
		if (!ControlHelperBegin(ImGui::GetID(label)))
			return false;

		ImGui::Text(label);
		ImGui::TableNextColumn();

		bool clear = false;
		bool changed = false;

		ImGui::InvisibleButton(label.data(), { ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeightWithSpacing() });

		auto& g = *GImGui;
		bool mixed_value = (g.LastItemData.InFlags & ImGuiItemFlags_MixedValue) != 0;
		if (mixed_value)
		{
			UI::DrawButton("--", ImVec2(0.5f, 0.5f), UI::GetItemRect());
		}
		else
		{
			if (assetHandle && !AssetManager::IsValidAssetHandle(assetHandle))
			{
				UI::ScopedColor textColor(ImGuiCol_Text, UI::Colors::Theme::TextError);
				UI::DrawButton("Null", UI::GetItemRect());
			}
			else
			{
				UI::ScopedColor textColor(ImGuiCol_Text, settings.TextColor);
				if (!assetHandle)
					UI::DrawButton("", ImVec2(0.0f, 0.5f), UI::GetItemRect());
				else if (!settings.DisplayName.empty())
					UI::DrawButton(settings.DisplayName, ImVec2(0.0f, 0.5f), UI::GetItemRect());
				else
				{
					UI::ScopedColor textColor(ImGuiCol_Text, settings.TextColor);
					const auto& metadata = Project::GetActiveEditorAssetManager()->GetMetadata(assetHandle);
					std::string name;
					if (metadata.IsMemoryAsset)
						name = fmt::format("0x{:x}", assetHandle);
					else
						name = metadata.FilePath.string();
					UI::DrawButton(name, ImVec2(0.0f, 0.5f), UI::GetItemRect());
				}
			}

		}

		static char s_SearchBuffer[260];
		static UI::TextFilter s_Filter("");

		ImGuiID popupID = ImGui::GetID("##selectAssetPopup"sv);
		if (ImGui::IsItemActivated())
		{
			memset(s_SearchBuffer, 0, std::size(s_SearchBuffer));
			s_Filter.SetFilter("");
			ImGui::OpenPopup(popupID);
		}

		ImRect lastItemRect = UI::GetItemRect();

		int popup_max_height_in_items = 12;
		ImGui::SetNextWindowSize({ 200, utils::CalcMaxPopupHeightFromItemCount(popup_max_height_in_items) });
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		const bool popupOpen = ImGui::BeginPopupEx(popupID, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar);
		ImGui::PopStyleVar();

		if (popupOpen)
		{
			const auto& style = ImGui::GetStyle();
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - style.WindowPadding.x * 2.0f);
			UI::ShiftCursor(style.WindowPadding);
			if (ImGui::IsWindowAppearing())
				ImGui::SetKeyboardFocusHere();
			if (UI::Widgets::Search(s_SearchBuffer))
				s_Filter.SetFilter(s_SearchBuffer);

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
				std::vector<AssetHandle> assets = AssetManager::GetAllAssetsOfType(assetType);

				for (AssetHandle handle : assets)
				{
					const AssetMetaData& metadata = Project::GetActiveEditorAssetManager()->GetMetadata(handle);
					if (metadata.IsMemoryAsset || metadata.IsEditorAsset)
						continue;

					std::string name = metadata.FilePath.stem().string();

					if (!s_Filter.PassesFilter(name))
						continue;

					if (ImGui::Selectable(name.c_str()))
					{
						assetHandle = handle;
						changed = true;
					}
				}
			}
			ImGui::EndChild();

			if (changed)
				ImGui::CloseCurrentPopup();

			ImGui::EndPopup();
		}

		if (settings.DropType)
		{
			if (ImGui::BeginDragDropTarget())
			{
				const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(settings.DropType);
				if (payload)
				{
					const AssetHandle handle = *(AssetHandle*)payload->Data;
					if (assetType == AssetManager::GetAssetType(handle))
					{
						assetHandle = handle;
						changed = true;
					}
				}
				ImGui::EndDragDropTarget();
			}
		}

		if (clear)
		{
			assetHandle = AssetHandle::Invalid;
		}

		ControlHelperEnd();
		return changed || clear;
	}

	bool ControlEntity(std::string_view label, UUID& entityID, const char* dragDropType)
	{
		if (!ControlHelperBegin(ImGui::GetID(label)))
			return false;

		ImGui::Text(label);
		ImGui::TableNextColumn();

		bool changed = false;
		const float buttonSize = ImGui::GetFrameHeightWithSpacing();

		ImGui::SetNextItemAllowOverlap();
		ImGui::InvisibleButton(label.data(), { ImGui::GetContentRegionAvail().x, buttonSize });

		auto& g = *GImGui;
		bool mixed_value = (g.LastItemData.InFlags & ImGuiItemFlags_MixedValue) != 0;
		if (mixed_value)
		{
			UI::DrawButton("--", ImVec2(0.5f, 0.5f), UI::GetItemRect());
		}
		else
		{
			Ref<Scene> scene = SelectionManager::GetActiveScene();
			if (entityID && !scene->IsValidEntityID(entityID))
			{
				UI::ScopedColor textColor(ImGuiCol_Text, UI::Colors::Theme::TextError);
				UI::DrawButton("Null", UI::GetItemRect());
			}
			else
			{
				Entity entity = scene->TryGetEntityByUUID(entityID);
				const std::string& name = entity ? entity.GetName() : std::string{};
				UI::DrawButton(name, ImVec2(0.0f, 0.5f), UI::GetItemRect());
			}
		}


		if (dragDropType)
		{
			if (ImGui::BeginDragDropTarget())
			{
				const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(dragDropType);
				if (payload)
				{
					UUID uuid = *(UUID*)payload->Data;
					Ref<Scene> scene = SelectionManager::GetActiveScene();
					if (scene->IsValidEntityID(uuid))
					{
						entityID = uuid;
						changed = true;
					}
				}
				ImGui::EndDragDropTarget();
			}
		}

		{
			UI::ScopedStyle frameBorder(ImGuiStyleVar_FrameBorderSize, 0.0f);
			UI::ScopedColorStack button(ImGuiCol_Button, 0x00000000,
										ImGuiCol_ButtonHovered, 0x00000000,
										ImGuiCol_ButtonActive, 0x00000000);

			ImGui::SameLine(0, 0);
			ShiftCursorX(-buttonSize);

			if (ImGui::Button("x", { buttonSize, buttonSize }))
			{
				entityID = UUID::Invalid;
				changed = true;
			}
		}

		ControlHelperEnd();
		return changed;
	}

}
