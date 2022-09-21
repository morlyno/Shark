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

	bool IsWindowFocused(ImGuiWindow* window, ImGuiFocusedFlags flags)
	{
		auto GetCombinedRootWindow = [](ImGuiWindow* window, bool popup_hierarchy, bool dock_hierarchy) -> ImGuiWindow*
		{
			ImGuiWindow* last_window = NULL;
			while (last_window != window)
			{
				last_window = window;
				window = window->RootWindow;
				if (popup_hierarchy)
					window = window->RootWindowPopupTree;
				if (dock_hierarchy)
					window = window->RootWindowDockTree;
			}
			return window;
		};

		ImGuiContext& g = *GImGui;
		ImGuiWindow* ref_window = g.NavWindow;
		ImGuiWindow* cur_window = window;

		if (ref_window == NULL)
			return false;
		if (flags & ImGuiFocusedFlags_AnyWindow)
			return true;

		IM_ASSERT(cur_window); // Not inside a Begin()/End()
		const bool popup_hierarchy = (flags & ImGuiFocusedFlags_NoPopupHierarchy) == 0;
		const bool dock_hierarchy = (flags & ImGuiFocusedFlags_DockHierarchy) != 0;
		if (flags & ImGuiHoveredFlags_RootWindow)
			cur_window = GetCombinedRootWindow(cur_window, popup_hierarchy, dock_hierarchy);

		if (flags & ImGuiHoveredFlags_ChildWindows)
			return IsWindowChildOf(ref_window, cur_window, popup_hierarchy, dock_hierarchy);
		else
			return (ref_window == cur_window);
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
			//ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, style.IndentSpacing * 0.5f);

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
			//ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, style.IndentSpacing * 0.5f);

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
			//ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, style.IndentSpacing * 0.5f);

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
			//ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, style.IndentSpacing * 0.5f);

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
			ImGui::PopStyleVar(1);
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
			Utils::GridSeparator();
		}
		
		if (grid & GridFlag::Widget)
		{
			ImGui::TableSetColumnIndex(1);
			Utils::GridSeparator();
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

	void ControlHelperDrawLabel(std::string_view label)
	{
		SK_CORE_ASSERT(ImGui::GetCurrentTable());
		ImGui::TableSetColumnIndex(0);
		ImGui::AlignTextToFramePadding();
		ImGui::TextEx(label.data(), label.data() + label.size());
		ImGui::TableSetColumnIndex(1);
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
	bool ControlScalar(std::string_view label, ImGuiDataType dataType, T& val, float speed, T min, T max, const char* fmt)
	{
		if (!ControlBeginHelper(label))
			return false;

		ImGui::TableSetColumnIndex(0);
		Text(label, PrivateTextFlag::LabelDefault);

		ImGui::TableSetColumnIndex(1);
		ImGui::SetNextItemWidth(-1.0f);
		const bool changed = ImGui::DragScalar("##control", dataType, &val, speed, &min, &max, fmt);

		ControlEndHelper();
		return changed;
	}

	template<glm::length_t L, typename T, glm::qualifier Q>
	bool ControlScalarVec(std::string_view label, ImGuiDataType dataType, glm::vec<L, T, Q>& val, float speed, T min, T max, const char* fmt)
	{
		if (!ControlBeginHelper(label))
			return false;

		ImGui::TableSetColumnIndex(0);
		Text(label, PrivateTextFlag::LabelDefault);

		ImGui::TableSetColumnIndex(1);
		ImGui::SetNextItemWidth(-1.0f);
		const bool changed = ImGui::DragScalarN("##control", dataType, &val, (int)L, speed, &min, &max, fmt);

		ControlEndHelper();
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

	bool Control(std::string_view label, uint32_t& val, float speed, uint32_t min, uint32_t max, const char* fmt)
	{
		return ControlScalar(label, ImGuiDataType_U32, val, speed, min, max, fmt);
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

	bool Control(std::string_view label, std::string& val)
	{
		if (!ControlBeginHelper(label))
			return false;

		ImGui::TableSetColumnIndex(0);
		Text(label, PrivateTextFlag::LabelDefault);
		ImGui::TableSetColumnIndex(1);
		ImGui::SetNextItemWidth(-1.0f);
		const bool changed = ImGui::InputText("##control", &val);
		ControlEndHelper();
		return changed;
	}

	bool Control(std::string_view label, UUID& uuid, const char* dragDropType)
	{
		if (!ControlBeginHelper(label))
			return false;

		ControlHelperDrawLabel(label);

		bool changed = false;
		char buffer[sizeof("0x0123456789ABCDEF")];
		if (uuid.IsValid())
			sprintf_s(buffer, "0x%llx", (uint64_t)uuid);
		else
			memset(buffer, 0, sizeof(buffer));
		ImGui::SetNextItemWidth(-1.0f);
		ImGui::InputTextWithHint("##control", "Null", buffer, sizeof(buffer), ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_NoHorizontalScroll);

		if (dragDropType)
		{
			if (ImGui::BeginDragDropTarget())
			{
				const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(dragDropType);
				if (payload)
				{
					uuid = *(UUID*)payload->Data;
					changed = true;
				}
				ImGui::EndDragDropTarget();
			}
		}

		{
			UI::ScopedStyle colorStack;
			colorStack.Push(ImGuiCol_Button, { 0.0f, 0.0f, 0.0f, 0.0f });
			colorStack.Push(ImGuiCol_ButtonHovered, { 0.0f, 0.0f, 0.0f, 0.0f });
			colorStack.Push(ImGuiCol_ButtonActive, { 0.0f, 0.0f, 0.0f, 0.0f });

			const float buttonSize = ImGui::GetItemRectSize().y;
			ImGui::SameLine(0, 0);
			MoveCursorX(-buttonSize);

			ImGui::BeginChild(UI::GetCurrentID(), ImVec2(buttonSize, buttonSize));
			if (ImGui::Button("x", { buttonSize, buttonSize }))
			{
				uuid = UUID::Null;
				changed = true;
			}
			ImGui::EndChild();
		}

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

	void Property(std::string_view label, const char* text, TextFlags flags)
	{
		Property(label, std::string_view(text), flags);
	}

	void Property(std::string_view label, std::string_view text, TextFlags flags)
	{
		if (!ControlBeginHelper(label))
			return;

		ImGui::TableSetColumnIndex(0);
		ImGui::AlignTextToFramePadding();
		Text(label);

		ImGui::TableSetColumnIndex(1);
		if (flags == TextFlag::None)
			TextFramed(text);
		else
			Text(text, flags);

		ControlEndHelper();
	}

	void Property(std::string_view label, const std::string& text, TextFlags flags)
	{
		Property(label, std::string_view(text), flags);
	}

	void Property(std::string_view label, const std::filesystem::path& path, TextFlags flags)
	{
		Property(label, path.string(), flags);
	}

	void Property(std::string_view label, const UUID& uuid)
	{
		if (!ControlBeginHelper(label))
			return;

		ImGui::TableSetColumnIndex(0);
		ImGui::AlignTextToFramePadding();
		Text(label);
		ImGui::TableSetColumnIndex(1);
		char buffer[sizeof("0x0123456789ABCDEF")];
		if (uuid.IsValid())
			sprintf_s(buffer, "0x%llx", (uint64_t)uuid);
		else
			memset(buffer, 0, sizeof(buffer));
		ImGui::SetNextItemWidth(-1.0f);
		ImGui::InputTextWithHint("##control", "Null", buffer, sizeof(buffer), ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_NoHorizontalScroll);

		ControlEndHelper();
	}

	void Property(std::string_view label, int value)
	{
		if (!ControlBeginHelper(label))
			return;

		const ImGuiStyle& style = ImGui::GetStyle();

		ImGui::TableSetColumnIndex(0);
		ImGui::AlignTextToFramePadding();
		Text(label);
		ImGui::TableSetColumnIndex(1);
		TextFramed("%d", value);
		ControlEndHelper();
	}

	void Text(std::string_view str, TextFlags flags)
	{
		ScopedStyle s;
		if (flags & TextFlag::Disabled)
			s.Push(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));

		if (flags & TextFlag::Selectable)
			return TextSelectable(str);

		if (flags & TextFlag::Aligned)
			ImGui::AlignTextToFramePadding();

		ImGui::TextEx(str.data(), str.data() + str.size());
	}

	void Text(const char* str, TextFlags flags)
	{
		return Text(std::string_view(str), flags);
	}

	void Text(const std::string& string, TextFlags flags)
	{
		return Text(std::string_view(string), flags);
	}

	void Text(const std::filesystem::path& path, TextFlags flags)
	{
		return Text(path.string(), flags);
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

	void TextFramed(std::string_view fmt, ...)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return;

		const ImGuiStyle style = ImGui::GetStyle();
		const ImVec2 pos = ImGui::GetCursorScreenPos();
		const ImVec2 size = { ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight() };
		const ImRect bb(pos, pos + size);
		ImGui::ItemSize(size, style.FramePadding.y);
		if (!ImGui::ItemAdd(bb, 0))
			return;

		va_list args;
		va_start(args, fmt);
		const char* text, *text_end;
		ImFormatStringToTempBufferV(&text, &text_end, fmt.data(), args);
		va_end(args);

		const ImU32 frameColor = ImGui::GetColorU32(ImGuiCol_FrameBg);
		ImGui::RenderFrame(bb.Min, bb.Max, frameColor, true, style.FrameRounding);
		ImGui::RenderTextClipped(bb.Min + style.FramePadding, bb.Max - style.FramePadding, text, text_end, nullptr, ImVec2(0.5f, 0.5f), &bb);
	}

	UIContext::UIContext()
	{
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
