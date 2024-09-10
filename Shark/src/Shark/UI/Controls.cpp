#include "skpch.h"
#include "Controls.h"

#include "Shark/Core/Project.h"
#include "Shark/Core/SelectionManager.h"
#include "Shark/Asset/AssetManager.h"
#include "Shark/Scene/Entity.h"

#include "Shark/UI/UI.h"
#include "Shark/UI/EditorResources.h"
#include "Shark/UI/Theme.h"
#include "Shark/ImGui/TextFilter.h"
#include "Shark/ImGui/ImGuiHelpers.h"

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

	extern UIContext* GContext;

	bool BeginControls()
	{
		auto& c = GContext->Control;
		SK_CORE_ASSERT(!c.Active, "Controls Begin/End mismatch");

		if (ImGui::BeginTable("ControlsTable", 2, ImGuiTableFlags_Resizable))
		{
			ImGuiStyle& style = ImGui::GetStyle();
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { style.ItemSpacing.x * 0.5f, style.ItemSpacing.y });
			//ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, style.IndentSpacing * 0.5f);

			c.Active = true;

			ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthStretch, 0.25f);
			ImGui::TableSetupColumn("Control", ImGuiTableColumnFlags_WidthStretch, 0.75f);

			return true;
		}
		return false;
	}

	bool BeginControlsGrid()
	{
		auto& c = GContext->Control;
		SK_CORE_ASSERT(!c.Active, "Controls Begin/End mismatch");

		if (ImGui::BeginTable("ControlsTable", 2, ImGuiTableFlags_Resizable))
		{
			ImGuiStyle& style = ImGui::GetStyle();
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { style.ItemSpacing.x * 0.5f, style.ItemSpacing.y });
			//ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, style.IndentSpacing * 0.5f);

			c.Active = true;
			c.DrawSeparator = true;

			ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthStretch, 0.25f);
			ImGui::TableSetupColumn("Control", ImGuiTableColumnFlags_WidthStretch, 0.75f);

			return true;
		}
		return false;
	}

	bool BeginControls(ImGuiID syncID)
	{
		auto& c = GContext->Control;
		SK_CORE_ASSERT(!c.Active, "Controls Begin/End mismatch");

		if (ImGui::BeginTableEx("ControlsTable", syncID, 2, ImGuiTableFlags_Resizable))
		{
			ImGuiStyle& style = ImGui::GetStyle();
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { style.ItemSpacing.x * 0.5f, style.ItemSpacing.y });
			//ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, style.IndentSpacing * 0.5f);

			c.Active = true;

			ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthStretch, 0.25f);
			ImGui::TableSetupColumn("Control", ImGuiTableColumnFlags_WidthStretch, 0.75f);

			return true;
		}
		return false;
	}

	bool BeginControlsGrid(ImGuiID syncID)
	{
		auto& c = GContext->Control;
		SK_CORE_ASSERT(!c.Active, "Controls Begin/End mismatch");

		if (ImGui::BeginTableEx("ControlsTable", syncID, 2, ImGuiTableFlags_Resizable))
		{
			ImGuiStyle& style = ImGui::GetStyle();
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { style.ItemSpacing.x * 0.5f, style.ItemSpacing.y });
			//ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, style.IndentSpacing * 0.5f);

			c.Active = true;
			c.DrawSeparator = true;

			ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthStretch, 0.25f);
			ImGui::TableSetupColumn("Control", ImGuiTableColumnFlags_WidthStretch, 0.75f);

			return true;
		}
		return false;
	}


	void EndControls()
	{
		auto& c = GContext->Control;

		if (c.Active)
		{
			c.Active = false;
			c.DrawSeparator = false;
			c.WidgetCount = 0;
			ImGui::PopStyleVar(1);
			ImGui::EndTable();
		}
	}

	void EndControlsGrid()
	{
		return EndControls();
	}

	void ControlSetupColumns(std::string_view label, ImGuiTableColumnFlags flags, float init_width_or_weight)
	{
		ImGui::TableSetupColumn(label.data(), flags, init_width_or_weight);
	}

	static void DrawControlSeperator()
	{
		ImGui::TableSetColumnIndex(0);
		utils::GridSeparator();

		ImGui::TableNextRow();
	}

	bool ControlHelperBegin(ImGuiID id)
	{
		auto& c = GContext->Control;

		if (!c.Active)
			return false;

		ImGui::PushID(id);
		ImGui::TableNextRow();

		if (c.DrawSeparator && c.WidgetCount++)
			DrawControlSeperator();

		return true;
	}

	bool ControlHelperBegin(std::string_view label)
	{
		return ControlHelperBegin(GetID(label));
	}

	void ControlHelperEnd()
	{
		SK_CORE_ASSERT(ImGui::GetCurrentTable());
		ImGui::PopID();
	}

	float ControlContentRegionWidth()
	{
		auto& c = GContext->Control;
		if (!c.Active)
			return 0.0f;

		uint32_t columnIndex = ImGui::TableGetColumnIndex();
		ImGui::TableSetColumnIndex(1);
		float contentRegionWidth = ImGui::GetContentRegionAvail().x;
		ImGui::TableSetColumnIndex(columnIndex);
		return contentRegionWidth;
	}

	void ControlHelperDrawLabel(std::string_view label)
	{
		SK_CORE_ASSERT(ImGui::GetCurrentTable());
		ImGui::TableSetColumnIndex(0);
		ImGui::AlignTextToFramePadding();
		ImGui::TextEx(label.data(), label.data() + label.size());
		ImGui::TableSetColumnIndex(1);
	}

	template<typename T>
	static bool ControlDrag(std::string_view label, ImGuiDataType dataType, T* val, uint32_t components, const T* speed, const T* min, const T* max, const char* fmt)
	{
		static_assert(std::is_scalar_v<T>);

		if (!ControlHelperBegin(label))
			return false;

		ControlHelperDrawLabel(label);

		bool changed = false;

		const ImGuiStyle& style = ImGui::GetStyle();
		const float comps = (float)components;
		//const float buttonSize = ImGui::GetFrameHeight();
		const float widthAvail = ImGui::GetContentRegionAvail().x;
		const float width = (widthAvail - style.ItemSpacing.x * (comps - 1.0f)) / comps;

		ImGui::PushItemWidth(width);

		size_t fmtOffset = 0;

		changed |= ImGui::DragScalar("##X", dataType, val, (float)speed[0] * 0.1f, min, max, fmt);

		if (components > 1)
		{
			ImGui::SameLine();
			changed |= ImGui::DragScalar("##Y", dataType, &val[1], (float)speed[1] * 0.1f, &min[1], &max[1], fmt);
		}

		if (components > 2)
		{
			ImGui::SameLine();
			changed |= ImGui::DragScalar("##Z", dataType, &val[2], (float)speed[2] * 0.1f, &min[2], &max[2], fmt);
		}

		if (components > 3)
		{
			ImGui::SameLine();
			changed |= ImGui::DragScalar("##W", dataType, &val[3], (float)speed[3] * 0.1f, &min[3], &max[3], fmt);
		}

		ImGui::PopItemWidth();

		ControlHelperEnd();
		return changed;
	}

	template<typename T>
	bool ControlScalar(std::string_view label, ImGuiDataType dataType, T& val, float speed, T min, T max, const char* fmt, ImGuiSliderFlags flags = ImGuiSliderFlags_None)
	{
		if (!ControlHelperBegin(label))
			return false;

		ControlHelperDrawLabel(label);

		ImGui::SetNextItemWidth(-1.0f);
		const bool changed = ImGui::DragScalar("##control", dataType, &val, speed, &min, &max, fmt, flags);

		ControlHelperEnd();
		return changed;
	}

	template<typename T>
	bool ControlSliderScalar(std::string_view label, ImGuiDataType dataType, T& val, T min, T max, const char* fmt, ImGuiSliderFlags flags = ImGuiSliderFlags_None)
	{
		if (!ControlHelperBegin(label))
			return false;

		ControlHelperDrawLabel(label);

		ImGui::SetNextItemWidth(-1.0f);
		const bool changed = ImGui::SliderScalar("##control", dataType, &val, &min, &max, fmt, flags);

		ControlHelperEnd();
		return changed;
	}

	template<glm::length_t L, typename T, glm::qualifier Q>
	bool ControlScalarVec(std::string_view label, ImGuiDataType dataType, glm::vec<L, T, Q>& val, float speed, T min, T max, const char* fmt)
	{
		if (!ControlHelperBegin(label))
			return false;

		ControlHelperDrawLabel(label);

		ImGui::SetNextItemWidth(-1.0f);
		const bool changed = ImGui::DragScalarN("##control", dataType, &val, (int)L, speed, &min, &max, fmt);

		ControlHelperEnd();
		return changed;
	}

	bool Control(std::string_view label, float& val, float speed, float min, float max, const char* fmt)
	{
		return ControlScalar(label, ImGuiDataType_Float, val, speed, min, max, fmt);
	}

	bool Control(std::string_view label, double& val, float speed, double min, double max, const char* fmt)
	{
		return ControlScalar(label, ImGuiDataType_Double, val, speed, min, max, fmt);
	}

	bool ControlSlider(std::string_view label, float& val, float min, float max, const char* fmt)
	{
		return ControlSliderScalar(label, ImGuiDataType_Float, val, min, max, fmt);
	}

	bool ControlSlider(std::string_view label, double& val, double min, double max, const char* fmt)
	{
		return ControlSliderScalar(label, ImGuiDataType_Double, val, min, max, fmt);
	}

	bool Control(std::string_view label, int8_t& val, float speed, int8_t min, int8_t max, const char* fmt)
	{
		return ControlScalar(label, ImGuiDataType_S8, val, speed, min, max, fmt);
	}

	bool Control(std::string_view label, int16_t& val, float speed, int16_t min, int16_t max, const char* fmt)
	{
		return ControlScalar(label, ImGuiDataType_S16, val, speed, min, max, fmt);
	}

	bool Control(std::string_view label, int32_t& val, float speed, int32_t min, int32_t max, const char* fmt)
	{
		return ControlScalar(label, ImGuiDataType_S32, val, speed, min, max, fmt);
	}

	bool Control(std::string_view label, int64_t& val, float speed, int64_t min, int64_t max, const char* fmt)
	{
		return ControlScalar(label, ImGuiDataType_S64, val, speed, min, max, fmt);
	}

	bool Control(std::string_view label, uint8_t& val, float speed, uint8_t min, uint8_t max, const char* fmt)
	{
		return ControlScalar(label, ImGuiDataType_U8, val, speed, min, max, fmt);
	}

	bool Control(std::string_view label, uint16_t& val, float speed, uint16_t min, uint16_t max, const char* fmt)
	{
		return ControlScalar(label, ImGuiDataType_U16, val, speed, min, max, fmt);
	}

	bool Control(std::string_view label, uint32_t& val, float speed, uint32_t min, uint32_t max, const char* fmt, ImGuiSliderFlags flags)
	{
		return ControlScalar(label, ImGuiDataType_U32, val, speed, min, max, fmt, flags);
	}

	bool Control(std::string_view label, uint64_t& val, float speed, uint64_t min, uint64_t max, const char* fmt)
	{
		return ControlScalar(label, ImGuiDataType_U64, val, speed, min, max, fmt);
	}

	bool Control(std::string_view label, glm::vec2& val, float speed, float min, float max, const char* fmt)
	{
		return ControlScalarVec(label, ImGuiDataType_Float, val, speed, min, max, fmt);
	}

	bool Control(std::string_view label, glm::vec3& val, float speed, float min, float max, const char* fmt)
	{
		return ControlScalarVec(label, ImGuiDataType_Float, val, speed, min, max, fmt);
	}

	bool Control(std::string_view label, glm::vec4& val, float speed, float min, float max, const char* fmt)
	{
		return ControlScalarVec(label, ImGuiDataType_Float, val, speed, min, max, fmt);
	}

	bool Control(std::string_view label, glm::ivec2& val, float speed, int32_t min, int32_t max, const char* fmt)
	{
		return ControlScalarVec(label, ImGuiDataType_S32, val, speed, min, max, fmt);
	}

	bool Control(std::string_view label, glm::ivec3& val, float speed, int32_t min, int32_t max, const char* fmt)
	{
		return ControlScalarVec(label, ImGuiDataType_S32, val, speed, min, max, fmt);
	}

	bool Control(std::string_view label, glm::ivec4& val, float speed, int32_t min, int32_t max, const char* fmt)
	{
		return ControlScalarVec(label, ImGuiDataType_S32, val, speed, min, max, fmt);
	}

	bool Control(std::string_view label, glm::uvec2& val, float speed, uint32_t min, uint32_t max, const char* fmt)
	{
		return ControlScalarVec(label, ImGuiDataType_U32, val, speed, min, max, fmt);
	}

	bool Control(std::string_view label, glm::uvec3& val, float speed, uint32_t min, uint32_t max, const char* fmt)
	{
		return ControlScalarVec(label, ImGuiDataType_U32, val, speed, min, max, fmt);
	}

	bool Control(std::string_view label, glm::uvec4& val, float speed, uint32_t min, uint32_t max, const char* fmt)
	{
		return ControlScalarVec(label, ImGuiDataType_U32, val, speed, min, max, fmt);
	}

	bool Control(std::string_view label, glm::mat4& matrix, float speed, float min, float max, const char* fmt)
	{
		if (!ControlHelperBegin(label))
			return false;

		ControlHelperDrawLabel(label);

		bool changed = false;

		ImGui::TableSetColumnIndex(1);
		ImGui::SetNextItemWidth(-1.0f);
		changed |= ImGui::DragFloat4("##control0", glm::value_ptr(matrix[0]), speed, min, max, fmt);

		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(1);
		ImGui::SetNextItemWidth(-1.0f);
		changed |= ImGui::DragFloat4("##control1", glm::value_ptr(matrix[1]), speed, min, max, fmt);

		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(1);
		ImGui::SetNextItemWidth(-1.0f);
		changed |= ImGui::DragFloat4("##control2", glm::value_ptr(matrix[2]), speed, min, max, fmt);

		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(1);
		ImGui::SetNextItemWidth(-1.0f);
		changed |= ImGui::DragFloat4("##control3", glm::value_ptr(matrix[3]), speed, min, max, fmt);

		ControlHelperEnd();
		return changed;
	}

	bool ControlColor(std::string_view label, glm::vec3& color)
	{
		if (!ControlHelperBegin(label))
			return false;

		ControlHelperDrawLabel(label);

		ImGui::SetNextItemWidth(-1.0f);
		const bool changed = ImGui::ColorEdit3("##ColorEdit3", glm::value_ptr(color));

		ControlHelperEnd();
		return changed;
	}

	bool ControlColor(std::string_view label, glm::vec4& color)
	{
		if (!ControlHelperBegin(label))
			return false;

		ControlHelperDrawLabel(label);

		ImGui::SetNextItemWidth(-1.0f);
		const bool changed = ImGui::ColorEdit4("##ColorEdit4", glm::value_ptr(color));

		ControlHelperEnd();
		return changed;
	}

	// TODO(moro): Ignores ImGuiItemFlag_Readonly
	bool Control(std::string_view label, bool& val)
	{
		if (!ControlHelperBegin(label))
			return false;

		ControlHelperDrawLabel(label);

		const bool changed = ImGui::Checkbox("##Checkbox", &val);

		ControlHelperEnd();
		return changed;
	}

	bool Control(std::string_view label, char* buffer, uint64_t bufferSize)
	{
		if (!ControlHelperBegin(label))
			return false;

		ControlHelperDrawLabel(label);
		ImGui::SetNextItemWidth(-1.0f);
		const bool changed = ImGui::InputText("##input", buffer, bufferSize);

		ControlHelperEnd();
		return changed;
	}

	bool Control(std::string_view label, std::string& val)
	{
		if (!ControlHelperBegin(label))
			return false;

		ControlHelperDrawLabel(label);

		ImGui::SetNextItemWidth(-1.0f);
		const bool changed = ImGui::InputText("##control", &val);
		ControlHelperEnd();
		return changed;
	}

	bool Control(std::string_view label, std::filesystem::path& path)
	{
		if (!ControlHelperBegin(label))
			return false;

		const float inputTextWidth = GImGui->NextItemData.Flags & ImGuiNextItemDataFlags_HasWidth ? GImGui->NextItemData.Width : -1.0f;

		ControlHelperDrawLabel(label);

		std::string strPath = path.generic_string();
		bool changed = false;

		ImGui::SetNextItemWidth(inputTextWidth);
		if (ImGui::InputText("##control", &strPath))
		{
			path = strPath;
			changed = true;
		}

		ControlHelperEnd();
		return changed;
	}

	bool Control(std::string_view label, UUID& uuid)
	{
		if (!ControlHelperBegin(label))
			return false;

		ControlHelperDrawLabel(label);

		bool changed = false;
		char buffer[21];
		if (uuid != UUID::Invalid)
			sprintf_s(buffer, "%llu", uuid.Value());
		else
			buffer[0] = '\0';

		ImGui::SetNextItemWidth(-1.0f);
		ImGui::SetNextItemAllowOverlap();
		ImGui::InputTextWithHint("##control", "Null", buffer, sizeof(buffer), ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_NoHorizontalScroll);

		{
			UI::ScopedColor button(ImGuiCol_Button, { 0.0f, 0.0f, 0.0f, 0.0f });
			UI::ScopedColor buttonHovered(ImGuiCol_ButtonHovered, { 0.0f, 0.0f, 0.0f, 0.0f });
			UI::ScopedColor buttonActive(ImGuiCol_ButtonActive, { 0.0f, 0.0f, 0.0f, 0.0f });

			const float buttonSize = ImGui::GetItemRectSize().y;
			ImGui::SameLine(0, 0);
			ShiftCursorX(-buttonSize);

			if (ImGui::Button("x", { buttonSize, buttonSize }))
			{
				uuid = UUID::Invalid;
				changed = true;
			}
		}

		ControlHelperEnd();
		return changed;
	}

	bool ControlAssetUnsave(std::string_view label, AssetHandle& assetHandle, const char* dragDropType)
	{
		if (!ControlHelperBegin(label))
			return false;

		ControlHelperDrawLabel(label);

		bool changed = false;
		const auto& metadata = Project::GetActiveEditorAssetManager()->GetMetadata(assetHandle);

		std::string name;
		if (metadata.IsMemoryAsset)
			name = fmt::format("0x{:x}", assetHandle);
		else
			name = metadata.FilePath.string();

		ImGui::SetNextItemWidth(-1.0f);
		ImGui::SetNextItemAllowOverlap();
		ImGui::InputText("##IDStr", name.data(), name.length(), ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_NoHorizontalScroll);

		//TextFramed(path);
		{
			if (ImGui::BeginPopupContextItem("Settings"))
			{
				char buffer[18];
				sprintf(buffer, "0x%16llx", assetHandle.Value());
				ImGui::InputText("##IDStr", buffer, 18, ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_NoHorizontalScroll);
				ImGui::EndPopup();
			}
		}

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
								UI::Colors::ColorWithMultipliedValue(UI::Colors::Theme::Text, 0.9f),
								UI::Colors::ColorWithMultipliedValue(UI::Colors::Theme::Text, 1.2f),
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
	bool ControlAsset(std::string_view label, AssetType assetType, AssetHandle& assetHandle, const char* dragDropType)
	{
		if (!ControlHelperBegin(label))
			return false;

		ControlHelperDrawLabel(label);

		bool clear = false;
		bool changed = false;
		const auto& metadata = Project::GetActiveEditorAssetManager()->GetMetadata(assetHandle);

		std::string name;
		if (metadata.IsMemoryAsset)
			name = fmt::format("0x{:x}", assetHandle);
		else
			name = metadata.FilePath.string();

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
				UI::DrawButton(name, ImVec2(0.0f, 0.5f), UI::GetItemRect());
			}
		}

		static char s_SearchBuffer[256];
		static UI::TextFilter s_Filter("");

		ImGuiID popupID = UI::GetID("##selectAssetPopup"sv);
		if (ImGui::IsItemActivated())
		{
			strcpy_s(s_SearchBuffer, "");
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
			if (UI::Search(UI::GenerateID(), s_SearchBuffer, std::size(s_SearchBuffer)))
				s_Filter.SetFilter(s_SearchBuffer);

			{
				UI::ScopedStyle frameBorder(ImGuiStyleVar_FrameBorderSize, 0.0f);
				UI::ScopedStyle frameRounding(ImGuiStyleVar_FrameRounding, 0.0f);
				UI::ScopedColorStack button(ImGuiCol_Button, UI::Colors::ColorWithMultipliedValue(UI::Colors::Theme::Background, 0.9f),
											ImGuiCol_ButtonHovered, UI::Colors::ColorWithMultipliedValue(UI::Colors::Theme::Background, 1.1f),
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

					if (!s_Filter.PassFilter(name))
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

		if (dragDropType)
		{
			if (ImGui::BeginDragDropTarget())
			{
				const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(dragDropType);
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
		if (!ControlHelperBegin(label))
			return false;

		ControlHelperDrawLabel(label);

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
			if (entityID && !scene->ValidEntityID(entityID))
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
					if (scene->ValidEntityID(uuid))
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

	bool ControlDragDrop(std::string_view label, std::string& val, const char* dragDropType)
	{
		if (!ControlHelperBegin(label))
			return false;

		ControlHelperDrawLabel(label);
		ImGui::SetNextItemWidth(-1.0f);
		ImGui::InputText("##input", val.data(), ImGuiInputTextFlags_ReadOnly);

		bool changed = false;
		if (ImGui::BeginDragDropTarget())
		{
			const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(dragDropType);
			if (payload)
			{
				val.assign((const char*)payload->Data, payload->DataSize);
				changed = true;
			}
			ImGui::EndDragDropTarget();
		}

		ControlHelperEnd();
		return changed;
	}

	bool ControlDragDrop(std::string_view label, std::filesystem::path& val, const char* dragDropType)
	{
		if (!ControlHelperBegin(label))
			return false;

		ControlHelperDrawLabel(label);
		std::string strVal = val.generic_string();
		ImGui::SetNextItemWidth(-1.0f);
		ImGui::InputText("##input", strVal.data(), ImGuiInputTextFlags_ReadOnly);

		bool changed = false;
		if (ImGui::BeginDragDropTarget())
		{
			const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(dragDropType);
			if (payload)
			{
				val.assign((const char*)payload->Data);
				changed = true;
			}
			ImGui::EndDragDropTarget();
		}

		ControlHelperEnd();
		return changed;
	}

	template<typename T>
	static bool ControlFlagsT(std::string_view label, T& val, const T& flag)
	{
		if (!ControlHelperBegin(label))
			return false;

		ControlHelperDrawLabel(label);

		const bool changed = utils::CheckboxFlagsT("##label", &val, flag);

		ControlHelperEnd();
		return changed;
	}

	bool ControlFlags(std::string_view label, int16_t& val, int16_t flag) { return ControlFlagsT(label, val, flag); }
	bool ControlFlags(std::string_view label, uint16_t& val, uint16_t flag) { return ControlFlagsT(label, val, flag); }
	bool ControlFlags(std::string_view label, int32_t& val, int32_t flag) { return ControlFlagsT(label, val, flag); }
	bool ControlFlags(std::string_view label, uint32_t& val, uint32_t flag) { return ControlFlagsT(label, val, flag); }

	template<typename T>
	bool ControlComboT(std::string_view label, T& index, const std::string_view items[], uint32_t itemsCount)
	{
		if (!ControlHelperBegin(label))
			return false;

		ControlHelperDrawLabel(label);

		bool changed = false;
		std::string_view preview = items[index];
		ImGui::SetNextItemWidth(-1.0f);
		if (ImGui::BeginCombo("#combo", preview.data()))
		{
			for (T i = 0; i < (T)itemsCount; i++)
			{
				std::string_view currentItem = items[i];
				if (ImGui::Selectable(currentItem.data(), i == index))
				{
					index = i;
					changed = true;
				}
			}

			ImGui::EndCombo();
		}

		ControlHelperEnd();
		return changed;
	}

	bool ControlCombo(std::string_view label, uint32_t& index, const std::string_view items[], uint32_t itemsCount)
	{
		return ControlComboT(label, index, items, itemsCount);
	}

	bool ControlCombo(std::string_view label, uint16_t& index, const std::string_view items[], uint32_t itemsCount)
	{
		return ControlComboT(label, index, items, itemsCount);
	}

	bool ControlCombo(std::string_view label, int& index, const std::string_view items[], uint32_t itemsCount)
	{
		return ControlComboT(label, index, items, itemsCount);
	}

	bool ControlCombo(std::string_view label, bool& value, const std::string_view falseValue, const std::string_view trueValue)
	{
		if (!ControlHelperBegin(label))
			return false;

		ControlHelperDrawLabel(label);

		bool changed = false;
		std::string_view preview = value ? trueValue : falseValue;
		ImGui::SetNextItemWidth(-1.0f);

		ImGuiContext& g = *GImGui;
		const bool isMixedValue = g.CurrentItemFlags & ImGuiItemFlags_MixedValue;

		if (ImGui::BeginCombo("##combo", isMixedValue ? nullptr : preview.data()))
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
			ImGui::EndCombo();
		}

		if (isMixedValue)
			UI::DrawTextAligned("--", ImVec2(0.5f, 0.5f), UI::GetItemRect());

		ControlHelperEnd();
		return changed;
	}

	void Property(std::string_view label, const char* text)
	{
		Property(label, std::string_view(text));
	}

	void Property(std::string_view label, std::string_view text)
	{
		if (!ControlHelperBegin(label))
			return;

		ImGui::TableSetColumnIndex(0);
		ImGui::AlignTextToFramePadding();
		ImGui::Text(label);

		ImGui::TableSetColumnIndex(1);
		TextFramed(text);

		ControlHelperEnd();
	}

	void Property(std::string_view label, const std::string& text)
	{
		Property(label, std::string_view(text));
	}

	void Property(std::string_view label, const std::filesystem::path& path)
	{
		Property(label, path.string());
	}

	void Property(std::string_view label, const UUID& uuid)
	{
		if (!ControlHelperBegin(label))
			return;

		ImGui::TableSetColumnIndex(0);
		ImGui::AlignTextToFramePadding();
		ImGui::Text(label);
		ImGui::TableSetColumnIndex(1);

		char buffer[21];
		if (uuid != UUID::Invalid)
			sprintf_s(buffer, "%llu", uuid.Value());
		else
			buffer[0] = '\0';

		ImGui::SetNextItemWidth(-1.0f);
		ImGui::InputTextWithHint("##control", "Null", buffer, sizeof(buffer), ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_NoHorizontalScroll);

		ControlHelperEnd();
	}

	void Property(std::string_view label, int value)
	{
		if (!ControlHelperBegin(label))
			return;

		const ImGuiStyle& style = ImGui::GetStyle();

		ImGui::TableSetColumnIndex(0);
		ImGui::AlignTextToFramePadding();
		ImGui::Text(label);
		ImGui::TableSetColumnIndex(1);
		TextFramed("%d", value);
		ControlHelperEnd();
	}

	void Property(std::string_view label, bool value)
	{
		ImGui::PushItemFlag(ImGuiItemFlags_ReadOnly, true);
		float v = value;
		Control(label, v);
		ImGui::PopItemFlag();
	}

	void Property(std::string_view label, float value)
	{
		ImGui::PushItemFlag(ImGuiItemFlags_ReadOnly, true);
		float v = value;
		Control(label, v);
		ImGui::PopItemFlag();
	}

	void Property(std::string_view label, uint32_t value)
	{
		ImGui::PushItemFlag(ImGuiItemFlags_ReadOnly, true);
		uint32_t v = value;
		Control(label, v);
		ImGui::PopItemFlag();
	}

	void Property(std::string_view label, uint64_t value)
	{
		ImGui::PushItemFlag(ImGuiItemFlags_ReadOnly, true);
		uint64_t v = value;
		Control(label, v);
		ImGui::PopItemFlag();
	}

	void Property(std::string_view label, const glm::vec2& value)
	{
		ImGui::PushItemFlag(ImGuiItemFlags_ReadOnly, true);
		glm::vec2 v = value;
		Control(label, v);
		ImGui::PopItemFlag();
	}

	void Property(std::string_view label, const glm::mat4& matrix)
	{
		ImGui::PushItemFlag(ImGuiItemFlags_ReadOnly, true);
		glm::mat4 m = matrix;
		Control(label, m);
		ImGui::PopItemFlag();
	}

	void Property(std::string_view label, TimeStep timestep)
	{
		Property(label, timestep.ToString());
	}

	void PropertyColor(std::string_view label, const glm::vec4& color)
	{
		if (!ControlHelperBegin(label))
			return;

		ControlHelperDrawLabel(label);
		glm::vec4 c = color;
		ImGui::SetNextItemWidth(-1.0f);
		ImGui::ColorEdit4("##property", glm::value_ptr(c));
		//ImGui::ReadOnlyCheckbox("##property", value);
		ControlHelperEnd();
	}

}
