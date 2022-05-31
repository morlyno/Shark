#include "skpch.h"
#include "UI.h"

#include "Shark/Math/Math.h"
#include "Shark/Utils/String.h"

#include "Shark/Core/Application.h"

#include "Shark/Core/Buffer.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <misc/cpp/imgui_stdlib.h>

namespace ImGui {

	static float CalcMaxPopupHeightFromItemCount(int items_count)
	{
		ImGuiContext& g = *GImGui;
		if (items_count <= 0)
			return FLT_MAX;
		return (g.FontSize + g.Style.ItemSpacing.y) * items_count - g.Style.ItemSpacing.y + (g.Style.WindowPadding.y * 2);
	}

	bool TableNextColumn(ImGuiTableRowFlags row_flags = 0, float min_row_height = 0.0f)
	{
		ImGuiContext& g = *GImGui;
		ImGuiTable* table = g.CurrentTable;
		if (!table)
			return false;

		if (table->IsInsideRow && table->CurrentColumn + 1 < table->ColumnsCount)
		{
			if (table->CurrentColumn != -1)
				TableEndCell(table);
			TableBeginCell(table, table->CurrentColumn + 1);
		}
		else
		{
			TableNextRow(row_flags, min_row_height);
			TableBeginCell(table, 0);
		}

		// Return whether the column is visible. User may choose to skip submitting items based on this return value,
		// however they shouldn't skip submitting for columns that may have the tallest contribution to row height.
		int column_n = table->CurrentColumn;
		return (table->RequestOutputMaskByIndex & ((ImU64)1 << column_n)) != 0;
	}

	template<typename T>
	bool CheckboxFlagsT(const char* label, T* flags, T flags_value)
	{
		bool all_on = (*flags & flags_value) == flags_value;
		bool any_on = (*flags & flags_value) != 0;
		bool pressed;
		if (!all_on && any_on)
		{
			ImGuiContext& g = *GImGui;
			ImGuiItemFlags backup_item_flags = g.CurrentItemFlags;
			g.CurrentItemFlags |= ImGuiItemFlags_MixedValue;
			pressed = Checkbox(label, &all_on);
			g.CurrentItemFlags = backup_item_flags;
		}
		else
		{
			pressed = Checkbox(label, &all_on);

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

	// Horizontal/vertical separating line
	void SeparatorEx(float thickness, ImGuiSeparatorFlags flags)
	{
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return;

		ImGuiContext& g = *GImGui;
		IM_ASSERT(ImIsPowerOfTwo(flags & (ImGuiSeparatorFlags_Horizontal | ImGuiSeparatorFlags_Vertical)));   // Check that only 1 option is selected

		float thickness_draw = 1.0f;
		float thickness_layout = 0.0f;
		if (flags & ImGuiSeparatorFlags_Vertical)
		{
			// Vertical separator, for menu bars (use current line height). Not exposed because it is misleading and it doesn't have an effect on regular layout.
			float y1 = window->DC.CursorPos.y;
			float y2 = window->DC.CursorPos.y + window->DC.CurrLineSize.y;
			const ImRect bb(ImVec2(window->DC.CursorPos.x, y1), ImVec2(window->DC.CursorPos.x + thickness_draw, y2));
			ItemSize(ImVec2(thickness_layout, 0.0f));
			if (!ItemAdd(bb, 0))
				return;

			// Draw
			window->DrawList->AddLine(ImVec2(bb.Min.x, bb.Min.y), ImVec2(bb.Min.x, bb.Max.y), GetColorU32(ImGuiCol_Separator), thickness);
			if (g.LogEnabled)
				LogText(" |");
		}
		else if (flags & ImGuiSeparatorFlags_Horizontal)
		{
			// Horizontal Separator
			float x1 = window->Pos.x;
			float x2 = window->Pos.x + window->Size.x;

			// FIXME-WORKRECT: old hack (#205) until we decide of consistent behavior with WorkRect/Indent and Separator
			if (g.GroupStack.Size > 0 && g.GroupStack.back().WindowID == window->ID)
				x1 += window->DC.Indent.x;

			// FIXME-WORKRECT: In theory we should simply be using WorkRect.Min.x/Max.x everywhere but it isn't aesthetically what we want,
			// need to introduce a variant of WorkRect for that purpose. (#4787)
			if (ImGuiTable* table = g.CurrentTable)
			{
				x1 = table->Columns[table->CurrentColumn].MinX;
				x2 = table->Columns[table->CurrentColumn].MaxX;
			}

			ImGuiOldColumns* columns = (flags & ImGuiSeparatorFlags_SpanAllColumns) ? window->DC.CurrentColumns : NULL;
			if (columns)
				PushColumnsBackground();

			// We don't provide our width to the layout so that it doesn't get feed back into AutoFit
			// FIXME: This prevents ->CursorMaxPos based bounding box evaluation from working (e.g. TableEndCell)
			const ImRect bb(ImVec2(x1, window->DC.CursorPos.y), ImVec2(x2, window->DC.CursorPos.y + thickness_draw));
			ItemSize(ImVec2(0.0f, thickness_layout));
			const bool item_visible = ItemAdd(bb, 0);
			if (item_visible)
			{
				// Draw
				window->DrawList->AddLine(bb.Min, ImVec2(bb.Max.x, bb.Min.y), GetColorU32(ImGuiCol_Separator), thickness);
				if (g.LogEnabled)
					LogRenderedText(&bb.Min, "--------------------------------\n");

			}
			if (columns)
			{
				PopColumnsBackground();
				columns->LineMinY = window->DC.CursorPos.y;
			}
		}
	}

}

namespace Shark::UI {

	namespace Utils {

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

	}

	UIContext* GContext = nullptr;

	void SetBlend(bool blend)
	{
		ImGuiLayer& ctx = Application::Get().GetImGuiLayer();
		ctx.SubmitBlendCallback(blend);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   /// Controls ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	bool BeginControls()
	{
		auto& c = GContext->Control;
		SK_CORE_ASSERT(!c.Active, "Controls Begin/End mismatch");

		if (ImGui::BeginTable("ControlsTable", 2, ImGuiTableFlags_Resizable))
		{
			ImGuiStyle& style = ImGui::GetStyle();
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { style.ItemSpacing.x * 0.5f, style.ItemSpacing.y });
			ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, style.IndentSpacing * 0.5f);

			c.Active = true;
			return true;
		}
		return false;
	}

	bool BeginControlsGrid(GridFlags flags)
	{
		auto& c = GContext->Control;
		SK_CORE_ASSERT(!c.Active, "Controls Begin/End mismatch");

		if (ImGui::BeginTable("ControlsTable", 2, ImGuiTableFlags_Resizable))
		{
			ImGuiStyle& style = ImGui::GetStyle();
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { style.ItemSpacing.x * 0.5f, style.ItemSpacing.y });
			ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, style.IndentSpacing * 0.5f);

			c.ActiveGridFlags = flags;
			c.Active = true;
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
			ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, style.IndentSpacing * 0.5f);

			c.Active = true;
			return true;
		}
		return false;
	}

	bool BeginControlsGrid(ImGuiID syncID, GridFlags flags)
	{
		auto& c = GContext->Control;
		SK_CORE_ASSERT(!c.Active, "Controls Begin/End mismatch");

		if (ImGui::BeginTableEx("ControlsTable", syncID, 2, ImGuiTableFlags_Resizable))
		{
			ImGuiStyle& style = ImGui::GetStyle();
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { style.ItemSpacing.x * 0.5f, style.ItemSpacing.y });
			ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, style.IndentSpacing * 0.5f);

			c.ActiveGridFlags = flags;
			c.Active = true;
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
			c.WidgetCount = 0;
			c.ActiveGridFlags = GridFlag::None;
			ImGui::PopStyleVar(2);
			ImGui::EndTable();
		}
	}

	void EndControlsGrid()
	{
		return EndControls();
	}

	static void DrawControlSeperator(GridFlags grid)
	{
		if (grid == GridFlag::None)
			return;

		if (grid & GridFlag::Label)
		{
			ImGui::TableSetColumnIndex(0);
			UI::Utils::GridSeparator();
		}
		
		if (grid & GridFlag::Widget)
		{
			ImGui::TableSetColumnIndex(1);
			UI::Utils::GridSeparator();
		}

		ImGui::TableNextRow();
	}

	bool ControlBeginHelper(ImGuiID id)
	{
		auto& c = GContext->Control;

		if (!c.Active)
			return false;

		ImGui::PushID(id);
		ImGui::TableNextRow();

		if (c.WidgetCount++)
			DrawControlSeperator(c.ActiveGridFlags);

		return true;
	}

	bool ControlBeginHelper(std::string_view label)
	{
		auto& c = GContext->Control;

		if (!c.Active)
			return false;

		if (!label.empty())
			ImGui::PushID(label.data(), label.data() + label.size());
		ImGui::TableNextRow();

		if (c.WidgetCount++)
			DrawControlSeperator(c.ActiveGridFlags);

		return true;
	}

	void ControlEndHelper()
	{
		SK_CORE_ASSERT(ImGui::GetCurrentTable());
		ImGui::PopID();
	}

	template<typename T>
	static bool ControlDrag(std::string_view label, ImGuiDataType dataType, T* val, uint32_t components, const T* resetVal, const T* speed, const T* min, const T* max, std::string_view fmt)
	{
		static_assert(std::is_scalar_v<T>);

		if (!ControlBeginHelper(label))
			return false;

		ImGui::TableSetColumnIndex(0);
		Text(label, PrivateTextFlag::LabelDefault);
		ImGui::TableSetColumnIndex(1);

		bool changed = false;
		const bool HasFMT = !fmt.empty();
		if (!HasFMT)
			fmt = GContext->DefaultFormat[dataType];

		ImGuiStyle& style = ImGui::GetStyle();
		const float comps = (float)components;
		const float buttonSize = ImGui::GetFrameHeight();
		const float widthAvail = ImGui::GetContentRegionAvail().x;
		const float width = (widthAvail - style.ItemSpacing.x * (comps - 1.0f)) / comps - buttonSize;

		ImGui::PushItemWidth(width);

		size_t fmtOffset = 0;

		if (ImGui::Button("X", { buttonSize, buttonSize }))
		{
			val[0] = resetVal[0] ;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::DragScalar("##X", dataType, val, (float)speed[0], min, max, fmt.data());

		if (components > 1)
		{
			if (HasFMT)
			{
				SK_CORE_ASSERT(fmtOffset != std::string::npos, "Invalid fmt");
				fmtOffset = fmt.find('\n', fmtOffset + 1);
			}

			ImGui::SameLine();
			if (ImGui::Button("Y", { buttonSize, buttonSize }))
			{
				val[1] = resetVal[1];
				changed = true;
			};

			ImGui::SameLine(0.0f, 0.0f);
			changed |= ImGui::DragScalar("##Y", dataType, &val[1], (float)speed[1], &min[1], &max[1], fmt.data() + fmtOffset);
		}

		if (components > 2)
		{
			if (HasFMT)
			{
				SK_CORE_ASSERT(fmtOffset != std::string::npos, "Invalid fmt");
				fmtOffset = fmt.find('\n', fmtOffset + 1);
			}

			ImGui::SameLine();
			if (ImGui::Button("Z", { buttonSize, buttonSize }))
			{
				val[2] = resetVal[2];
				changed = true;
			}
			ImGui::SameLine(0.0f, 0.0f);
			changed |= ImGui::DragScalar("##Z", dataType, &val[2], (float)speed[2], &min[2], &max[2], fmt.data() + fmtOffset);
		}

		if (components > 3)
		{
			if (HasFMT)
			{
				SK_CORE_ASSERT(fmtOffset != std::string::npos, "Invalid fmt");
				fmtOffset = fmt.find('\n', fmtOffset + 1);
			}

			ImGui::SameLine();
			if (ImGui::Button("W", { buttonSize, buttonSize }))
			{
				val[3] = resetVal[3];
				changed = true;
			}
			ImGui::SameLine(0.0f, 0.0f);
			changed |= ImGui::DragScalar("##W", dataType, &val[3], (float)speed[3], &min[3], &max[3], fmt.data() + fmtOffset);
		}

		ImGui::PopItemWidth();

		ControlEndHelper();
		return changed;
	}

	template<typename T>
	static bool ControlSlider(std::string_view label, ImGuiDataType dataType, T* val, uint32_t components, const T* resetVal, const T* min, const T* max, std::string_view fmt)
	{
		if (!ControlBeginHelper(label))
			return false;

		ImGui::TableSetColumnIndex(0);
		Text(label, PrivateTextFlag::LabelDefault);
		ImGui::TableSetColumnIndex(1);

		bool changed = false;
		const bool HasFMT = !fmt.empty();
		if (!HasFMT)
			fmt = GContext->DefaultFormat[dataType];

		ImGuiStyle& style = ImGui::GetStyle();
		const float comps = (float)components;
		const float buttonSize = ImGui::GetFrameHeight();
		const float widthAvail = ImGui::GetContentRegionAvail().x;
		const float width = (widthAvail - style.ItemSpacing.x * (comps - 1)) / comps - buttonSize;

		ImGui::PushItemWidth(width);

		size_t fmtOffset = 0;

		if (ImGui::Button("X", { buttonSize, buttonSize }))
		{
			val[0] = resetVal[0];
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::SliderScalar("##X", dataType, &val[0], &min[0], &max[0], fmt.data());

		if (components > 1)
		{
			if (HasFMT)
			{
				SK_CORE_ASSERT(fmtOffset != std::string::npos, "Invalid fmt");
				fmtOffset = fmt.find('\n', fmtOffset + 1);
			}

			ImGui::SameLine();
			if (ImGui::Button("Y", { buttonSize, buttonSize }))
			{
				val[1] = resetVal[1];
				changed = true;
			}
			ImGui::SameLine(0.0f, 0.0f);
			changed |= ImGui::SliderScalar("##Y", dataType, &val[1], &min[1], &max[1], fmt.data() + fmtOffset);
		}

		if (components > 2)
		{
			if (HasFMT)
			{
				SK_CORE_ASSERT(fmtOffset != std::string::npos, "Invalid fmt");
				fmtOffset = fmt.find('\n', fmtOffset + 1);
			}

			ImGui::SameLine();
			if (ImGui::Button("Z", { buttonSize, buttonSize }))
			{
				val[2] = resetVal[2];
				changed = true;
			}
			ImGui::SameLine(0.0f, 0.0f);
			changed |= ImGui::SliderScalar("##Z", dataType, &val[2], &min[2], &max[2], fmt.data() + fmtOffset);
		}

		if (components > 3)
		{
			if (HasFMT)
			{
				SK_CORE_ASSERT(fmtOffset != std::string::npos, "Invalid fmt");
				fmtOffset = fmt.find('\n', fmtOffset + 1);
			}

			ImGui::SameLine();
			if (ImGui::Button("W", { buttonSize, buttonSize }))
			{
				val[3] = resetVal[3];
				changed = true;
			}
			ImGui::SameLine(0.0f, 0.0f);
			changed |= ImGui::SliderScalar("##W", dataType, &val[3], &min[3], &max[3], fmt.data() + fmtOffset);
		}

		ImGui::PopItemWidth();
		ControlEndHelper();
		return changed;
	}

	template<typename T>
	static bool ControlScalar(std::string_view label, ImGuiDataType dataType, T* val, uint32_t components, const T* resetVal, const T* speed, const T* min, const T* max, std::string_view fmt, ControlType type)
	{
		if (type == ControlType::Slider)
			return ControlSlider(label, dataType, val, components, resetVal, min, max, fmt);

		if (type == ControlType::Drag)
			return ControlDrag(label, dataType, val, components, resetVal, speed, min, max, fmt);

		SK_CORE_ASSERT(false, "Unkown ControlType");
		return false;
	}

	bool Control(std::string_view label, float& val,     float resetVal,            float min,            float max,            float speed,            std::string_view fmt, ControlType type) { return ControlScalar(label, ImGuiDataType_Float, &(val), 1, &(resetVal), &(speed), &(min), &(max), fmt, type); }
	bool Control(std::string_view label, glm::vec2& val, const glm::vec2& resetVal, const glm::vec2& min, const glm::vec2& max, const glm::vec2& speed, std::string_view fmt, ControlType type) { return ControlScalar(label, ImGuiDataType_Float, glm::value_ptr(val), 2, glm::value_ptr(resetVal), glm::value_ptr(speed), glm::value_ptr(min), glm::value_ptr(max), fmt, type); }
	bool Control(std::string_view label, glm::vec3& val, const glm::vec3& resetVal, const glm::vec3& min, const glm::vec3& max, const glm::vec3& speed, std::string_view fmt, ControlType type) { return ControlScalar(label, ImGuiDataType_Float, glm::value_ptr(val), 3, glm::value_ptr(resetVal), glm::value_ptr(speed), glm::value_ptr(min), glm::value_ptr(max), fmt, type); }
	bool Control(std::string_view label, glm::vec4& val, const glm::vec4& resetVal, const glm::vec4& min, const glm::vec4& max, const glm::vec4& speed, std::string_view fmt, ControlType type) { return ControlScalar(label, ImGuiDataType_Float, glm::value_ptr(val), 4, glm::value_ptr(resetVal), glm::value_ptr(speed), glm::value_ptr(min), glm::value_ptr(max), fmt, type); }

	bool Control(std::string_view label, int& val,        int resetVal,               int min,               int max,               int speed,               std::string_view fmt, ControlType type) { return ControlScalar(label, ImGuiDataType_S32, &(val), 1, &(resetVal), &(speed), &(min), &(max), fmt, type); }
	bool Control(std::string_view label, glm::ivec2& val, const glm::ivec2& resetVal, const glm::ivec2& min, const glm::ivec2& max, const glm::ivec2& speed, std::string_view fmt, ControlType type) { return ControlScalar(label, ImGuiDataType_S32, glm::value_ptr(val), 2, glm::value_ptr(resetVal), glm::value_ptr(speed), glm::value_ptr(min), glm::value_ptr(max), fmt, type); }
	bool Control(std::string_view label, glm::ivec3& val, const glm::ivec3& resetVal, const glm::ivec3& min, const glm::ivec3& max, const glm::ivec3& speed, std::string_view fmt, ControlType type) { return ControlScalar(label, ImGuiDataType_S32, glm::value_ptr(val), 3, glm::value_ptr(resetVal), glm::value_ptr(speed), glm::value_ptr(min), glm::value_ptr(max), fmt, type); }
	bool Control(std::string_view label, glm::ivec4& val, const glm::ivec4& resetVal, const glm::ivec4& min, const glm::ivec4& max, const glm::ivec4& speed, std::string_view fmt, ControlType type) { return ControlScalar(label, ImGuiDataType_S32, glm::value_ptr(val), 4, glm::value_ptr(resetVal), glm::value_ptr(speed), glm::value_ptr(min), glm::value_ptr(max), fmt, type); }

	bool Control(std::string_view label, uint32_t& val,   uint32_t resetVal,          uint32_t min,          uint32_t max,          uint32_t speed,          std::string_view fmt, ControlType type) { return ControlScalar(label, ImGuiDataType_U32, &(val), 1, &(resetVal), &(speed), &(min), &(max), fmt, type); }
	bool Control(std::string_view label, glm::uvec2& val, const glm::uvec2& resetVal, const glm::uvec2& min, const glm::uvec2& max, const glm::uvec2& speed, std::string_view fmt, ControlType type) { return ControlScalar(label, ImGuiDataType_U32, glm::value_ptr(val), 2, glm::value_ptr(resetVal), glm::value_ptr(speed), glm::value_ptr(min), glm::value_ptr(max), fmt, type); }
	bool Control(std::string_view label, glm::uvec3& val, const glm::uvec3& resetVal, const glm::uvec3& min, const glm::uvec3& max, const glm::uvec3& speed, std::string_view fmt, ControlType type) { return ControlScalar(label, ImGuiDataType_U32, glm::value_ptr(val), 3, glm::value_ptr(resetVal), glm::value_ptr(speed), glm::value_ptr(min), glm::value_ptr(max), fmt, type); }
	bool Control(std::string_view label, glm::uvec4& val, const glm::uvec4& resetVal, const glm::uvec4& min, const glm::uvec4& max, const glm::uvec4& speed, std::string_view fmt, ControlType type) { return ControlScalar(label, ImGuiDataType_U32, glm::value_ptr(val), 4, glm::value_ptr(resetVal), glm::value_ptr(speed), glm::value_ptr(min), glm::value_ptr(max), fmt, type); }
	
	bool ControlAngle(std::string_view label, float& radians,     float resetVal,            float min,            float max,            float speed,            std::string_view fmt, ControlType type) { auto degrees = glm::degrees(radians); const bool changed = Control(label, degrees, resetVal, min, max, speed, fmt, type); if (changed) { radians = glm::radians(degrees); return true; } return false; }
	bool ControlAngle(std::string_view label, glm::vec2& radians, const glm::vec2& resetVal, const glm::vec2& min, const glm::vec2& max, const glm::vec2& speed, std::string_view fmt, ControlType type) { auto degrees = glm::degrees(radians); const bool changed = Control(label, degrees, resetVal, min, max, speed, fmt, type); if (changed) { radians = glm::radians(degrees); return true; } return false; }
	bool ControlAngle(std::string_view label, glm::vec3& radians, const glm::vec3& resetVal, const glm::vec3& min, const glm::vec3& max, const glm::vec3& speed, std::string_view fmt, ControlType type) { auto degrees = glm::degrees(radians); const bool changed = Control(label, degrees, resetVal, min, max, speed, fmt, type); if (changed) { radians = glm::radians(degrees); return true; } return false; }
	bool ControlAngle(std::string_view label, glm::vec4& radians, const glm::vec4& resetVal, const glm::vec4& min, const glm::vec4& max, const glm::vec4& speed, std::string_view fmt, ControlType type) { auto degrees = glm::degrees(radians); const bool changed = Control(label, degrees, resetVal, min, max, speed, fmt, type); if (changed) { radians = glm::radians(degrees); return true; } return false; }

	bool ControlColor(std::string_view label, glm::vec4& color)
	{
		if (!ControlBeginHelper(label))
			return false;

		ImGui::TableSetColumnIndex(0);
		Text(label, PrivateTextFlag::LabelDefault);

		ImGui::TableSetColumnIndex(1);
		ImGui::SetNextItemWidth(-1.0f);
		const bool changed = ImGui::ColorEdit4("##ColorEdit4", glm::value_ptr(color));

		ControlEndHelper();
		return changed;
	}

	bool Control(std::string_view label, bool& val)
	{
		if (!ControlBeginHelper(label))
			return false;

		ImGui::TableSetColumnIndex(0);
		Text(label, PrivateTextFlag::LabelDefault);

		ImGui::TableSetColumnIndex(1);
		const bool changed = ImGui::Checkbox("##Checkbox", &val);

		ControlEndHelper();
		return changed;
	}

	template<typename T>
	static bool ControlFlagsT(std::string_view label, T& val, const T& flag)
	{
		if (!ControlBeginHelper(label))
			return false;

		ImGui::TableSetColumnIndex(0);
		Text(label, PrivateTextFlag::LabelDefault);

		ImGui::TableSetColumnIndex(1);
		const bool changed = ImGui::CheckboxFlagsT("##label", &val, flag);

		ControlEndHelper();
		return changed;
	}

	bool ControlFlags(std::string_view label,  int16_t& val,  int16_t flag) { return ControlFlagsT(label, val, flag); }
	bool ControlFlags(std::string_view label, uint16_t& val, uint16_t flag) { return ControlFlagsT(label, val, flag); }
	bool ControlFlags(std::string_view label,  int32_t& val,  int32_t flag) { return ControlFlagsT(label, val, flag); }
	bool ControlFlags(std::string_view label, uint32_t& val, uint32_t flag) { return ControlFlagsT(label, val, flag); }

	template<typename T>
	bool ControlComboT(std::string_view label, T& index, const std::string_view items[], uint32_t itemsCount)
	{
		if (!ControlBeginHelper(label))
			return false;

		ImGui::TableSetColumnIndex(0);
		Text(label, PrivateTextFlag::LabelDefault);

		ImGui::TableSetColumnIndex(1);

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

		ControlEndHelper();
		return changed;
	}

	bool Control(std::string_view label, uint32_t& index, const std::string_view items[], uint32_t itemsCount)
	{
		return ControlComboT(label, index, items, itemsCount);
	}

	bool Control(std::string_view label, uint16_t& index, const std::string_view items[], uint32_t itemsCount)
	{
		return ControlComboT(label, index, items, itemsCount);
	}

	bool Control(std::string_view label, int& index, const std::string_view items[], uint32_t itemsCount)
	{
		return ControlComboT(label, index, items, itemsCount);
	}

	void Control(std::string_view label, std::string_view str, TextFlags flags, TextFlags labelFlags)
	{
		if (!ControlBeginHelper(label))
			return;

		ImGui::TableSetColumnIndex(0);
		Text(label, labelFlags | PrivateTextFlag::LabelDefault);

		ImGui::TableSetColumnIndex(1);
		Text(str, flags | PrivateTextFlag::StringDefault);

		ControlEndHelper();
	}

	void Control(std::string_view label, std::string&& str, TextFlags flags, TextFlags labelFlags)
	{
		Control(label, std::string_view(str), flags, labelFlags);
	}

	void Control(std::string_view label, const char* str, TextFlags flags, TextFlags labelFlags)
	{
		Control(label, std::string_view(str), flags, labelFlags);
	}

	void Control(std::string_view label, const std::filesystem::path& filePath, TextFlags flags, TextFlags labelFalgs)
	{
		Control(label, filePath.string(), flags, labelFalgs);
	}

	void Control(std::string_view label, const std::string& str, TextFlags flags, TextFlags labelFalgs)
	{
		Control(label, std::string_view(str), flags, labelFalgs);
	}

	bool ControlCustomBegin(std::string_view label, TextFlags labelFlags)
	{
		if (!ControlBeginHelper(label))
			return false;

		ImGui::TableSetColumnIndex(0);
		Text(label, labelFlags | PrivateTextFlag::LabelDefault);

		ImGui::TableSetColumnIndex(1);
		return true;
	}

	void ControlCustomEnd()
	{
		ControlEndHelper();
	}

	void Control(ImGuiID id, void(*func)(void* data, ImGuiID id), void* data)
	{
		if (!ControlBeginHelper(id))
			return;

		func(data, id);

		ControlEndHelper();
	}

	void Control(std::string_view label, void(*func)(void* data, ImGuiID id), void* data)
	{
		const ImGuiID id = ImGui::GetID(label.data(), label.data() + label.size());
		if (!ControlBeginHelper(id))
			return;

		ImGui::TableSetColumnIndex(0);
		Text(label);

		ImGui::TableSetColumnIndex(1);
		func(data, id);

		ControlEndHelper();
	}


	void Text(std::string_view str, TextFlags flags)
	{
		UI::ScopedStyle s;
		if (flags & TextFlag::Disabled)
			s.Push(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));

		if (flags & TextFlag::Selectable)
			return TextSelectable(str);

		if (flags & TextFlag::Aligned)
			ImGui::AlignTextToFramePadding();

		ImGui::TextEx(str.data(), str.data() + str.size());
	}

	void Text(std::string&& str, TextFlags flags)
	{
		Text(std::string_view(str), flags);
	}

	void Text(const char* str, TextFlags flags)
	{
		Text(std::string_view(str), flags);
	}

	void Text(const std::filesystem::path& filePath, TextFlags flags)
	{
		std::string str = filePath.string();
		Text(std::string_view(str), flags);
	}

	void Text(const std::string& str, TextFlags flags)
	{
		Text(std::string_view(str), flags);
	}

	void TextSelectable(std::string_view str)
	{
		//Buffer buffer;
		//buffer.Allocate(str.size());
		//buffer.Write(str.data(), str.size());
		//ImGui::InputText("##InputText", buffer.As<char>(), buffer.Size, ImGuiInputTextFlags_ReadOnly);
		//buffer.Release();

		const auto& style = ImGui::GetStyle();
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, style.FramePadding.y));

		const ImVec2 textSize = ImGui::CalcTextSize(str.data(), str.data() + str.size());
		const ImVec2 itemSize = ImGui::CalcItemSize(ImVec2(0.0f, 0.0f), textSize.x + style.FramePadding.x * 2.0f + 1.0f, textSize.y + style.FramePadding.y * 2.0f);

		ImGui::InputTextEx("##InputText", nullptr, (char*)str.data(), (int)str.size(), itemSize, ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_NoHorizontalScroll);
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();
	}

	UIContext::UIContext()
	{
		DefaultFormat[ImGuiDataType_S8] = "%d";
		DefaultFormat[ImGuiDataType_U8] = "%u";
		DefaultFormat[ImGuiDataType_S16] = "%d";
		DefaultFormat[ImGuiDataType_U16] = "%u";
		DefaultFormat[ImGuiDataType_S32] = "%d";
		DefaultFormat[ImGuiDataType_U32] = "%u";
		DefaultFormat[ImGuiDataType_S64] = "%d";
		DefaultFormat[ImGuiDataType_U64] = "%u";
		DefaultFormat[ImGuiDataType_Float] = "%.2f";
		DefaultFormat[ImGuiDataType_Double] = "%.2f";
	}

	UIContext* CreateContext()
	{
		UIContext* ctx = new UIContext();
		if (!GContext)
			SetContext(ctx);
		return ctx;
	}

	void DestroyContext(UIContext* ctx)
	{
		if (ctx == nullptr)
			ctx = GContext;
		if (ctx == GContext)
			SetContext(nullptr);
		delete ctx;
	}

	void SetContext(UIContext* ctx)
	{
		GContext = ctx;
	}

	UIContext* GetContext()
	{
		return GContext;
	}

	void NewFrame()
	{
		auto& c = GContext->Control;
		SK_CORE_ASSERT(GContext->Control.Active == false, "Controls Begin/End mismatch");
		SK_CORE_ASSERT(GContext->Control.WidgetCount == 0);
		SK_CORE_ASSERT(GContext->Control.ActiveGridFlags == GridFlag::None);
	}

}
