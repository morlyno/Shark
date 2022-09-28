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

	ImGuiID GenerateID()
	{
		ImGuiWindow* window = GImGui->CurrentWindow;
		return window->GetID(window->IDStack.front());
	}

	void SetBlend(bool blend)
	{
		ImGuiLayer& ctx = Application::Get().GetImGuiLayer();
		ctx.SubmitBlendCallback(blend);
	}

	ImU32 ToColor32(const ImVec4& color)
	{
		ImU32 out;
		out = ((ImU32)IM_F32_TO_INT8_SAT(color.x)) << IM_COL32_R_SHIFT;
		out |= ((ImU32)IM_F32_TO_INT8_SAT(color.y)) << IM_COL32_G_SHIFT;
		out |= ((ImU32)IM_F32_TO_INT8_SAT(color.z)) << IM_COL32_B_SHIFT;
		out |= ((ImU32)IM_F32_TO_INT8_SAT(color.w)) << IM_COL32_A_SHIFT;
		return out;
	}

	ImU32 ToColor32(const ImVec4& color, float alpha)
	{
		ImU32 out;
		out = ((ImU32)IM_F32_TO_INT8_SAT(color.x)) << IM_COL32_R_SHIFT;
		out |= ((ImU32)IM_F32_TO_INT8_SAT(color.y)) << IM_COL32_G_SHIFT;
		out |= ((ImU32)IM_F32_TO_INT8_SAT(color.z)) << IM_COL32_B_SHIFT;
		out |= ((ImU32)IM_F32_TO_INT8_SAT(alpha)) << IM_COL32_A_SHIFT;
		return out;
	}

	ImVec4 GetColor(ImGuiCol color, float override_alpha)
	{
		const ImVec4& col = ImGui::GetStyleColorVec4(color);
		return {
			col.x,
			col.y,
			col.z,
			override_alpha
		};
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
	static bool ControlDrag(std::string_view label, ImGuiDataType dataType, T* val, uint32_t components, const T* speed, const T* min, const T* max, const char* fmt)
	{
		static_assert(std::is_scalar_v<T>);

		if (!ControlBeginHelper(label))
			return false;

		ImGui::TableSetColumnIndex(0);
		Text(label, PrivateTextFlag::LabelDefault);
		ImGui::TableSetColumnIndex(1);

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

		ControlEndHelper();
		return changed;
	}

	template<typename T>
	static bool ControlSlider(std::string_view label, ImGuiDataType dataType, T* val, uint32_t components, const T* min, const T* max)
	{
		if (!ControlBeginHelper(label))
			return false;

		ImGui::TableSetColumnIndex(0);
		Text(label, PrivateTextFlag::LabelDefault);
		ImGui::TableSetColumnIndex(1);

		bool changed = false;

		ImGuiStyle& style = ImGui::GetStyle();
		const float comps = (float)components;
		const float widthAvail = ImGui::GetContentRegionAvail().x;
		const float width = (widthAvail - style.ItemSpacing.x * (comps - 1)) / comps;

		ImGui::PushItemWidth(width);

		changed |= ImGui::SliderScalar("##X", dataType, &val[0], &min[0], &max[0]);

		if (components > 1)
		{
			ImGui::SameLine();
			changed |= ImGui::SliderScalar("##Y", dataType, &val[1], &min[1], &max[1]);
		}

		if (components > 2)
		{
			ImGui::SameLine();
			changed |= ImGui::SliderScalar("##Z", dataType, &val[2], &min[2], &max[2]);
		}

		if (components > 3)
		{
			ImGui::SameLine();
			changed |= ImGui::SliderScalar("##W", dataType, &val[3], &min[3], &max[3]);
		}

		ImGui::PopItemWidth();
		ControlEndHelper();
		return changed;
	}

	template<typename T>
	static bool ControlScalar(std::string_view label, ImGuiDataType dataType, T* val, uint32_t components, const T* speed, const T* min, const T* max, const char* fmt = "%.2f")
	{

		// maby add opting to switch to slider
		return ControlDrag(label, dataType, val, components, speed, min, max, fmt);

		SK_CORE_ASSERT(false, "Unkown ControlType");
		return false;
	}

	bool Control(std::string_view label, float& val, float min, float max, float speed, const char* fmt)                     { return ControlScalar(label, ImGuiDataType_Float, &val, 1, &speed, &min, &max, fmt); }
	bool Control(std::string_view label, glm::vec2& val, const glm::vec2& min, const glm::vec2& max, const glm::vec2& speed) { return ControlScalar(label, ImGuiDataType_Float, glm::value_ptr(val), 2, glm::value_ptr(speed), glm::value_ptr(min), glm::value_ptr(max)); }
	bool Control(std::string_view label, glm::vec3& val, const glm::vec3& min, const glm::vec3& max, const glm::vec3& speed) { return ControlScalar(label, ImGuiDataType_Float, glm::value_ptr(val), 3, glm::value_ptr(speed), glm::value_ptr(min), glm::value_ptr(max)); }
	bool Control(std::string_view label, glm::vec4& val, const glm::vec4& min, const glm::vec4& max, const glm::vec4& speed) { return ControlScalar(label, ImGuiDataType_Float, glm::value_ptr(val), 4, glm::value_ptr(speed), glm::value_ptr(min), glm::value_ptr(max)); }

	bool Control(std::string_view label, int& val,        int min,               int max,               int speed)               { return ControlScalar(label, ImGuiDataType_S32, &val,                1, &speed,                &min,                &max); }
	bool Control(std::string_view label, glm::ivec2& val, const glm::ivec2& min, const glm::ivec2& max, const glm::ivec2& speed) { return ControlScalar(label, ImGuiDataType_S32, glm::value_ptr(val), 2, glm::value_ptr(speed), glm::value_ptr(min), glm::value_ptr(max)); }
	bool Control(std::string_view label, glm::ivec3& val, const glm::ivec3& min, const glm::ivec3& max, const glm::ivec3& speed) { return ControlScalar(label, ImGuiDataType_S32, glm::value_ptr(val), 3, glm::value_ptr(speed), glm::value_ptr(min), glm::value_ptr(max)); }
	bool Control(std::string_view label, glm::ivec4& val, const glm::ivec4& min, const glm::ivec4& max, const glm::ivec4& speed) { return ControlScalar(label, ImGuiDataType_S32, glm::value_ptr(val), 4, glm::value_ptr(speed), glm::value_ptr(min), glm::value_ptr(max)); }

	bool Control(std::string_view label, uint32_t& val,   uint32_t min,          uint32_t max,          uint32_t speed)          { return ControlScalar(label, ImGuiDataType_U32, &(val),              1, &speed,                &min,                &max); }
	bool Control(std::string_view label, glm::uvec2& val, const glm::uvec2& min, const glm::uvec2& max, const glm::uvec2& speed) { return ControlScalar(label, ImGuiDataType_U32, glm::value_ptr(val), 2, glm::value_ptr(speed), glm::value_ptr(min), glm::value_ptr(max)); }
	bool Control(std::string_view label, glm::uvec3& val, const glm::uvec3& min, const glm::uvec3& max, const glm::uvec3& speed) { return ControlScalar(label, ImGuiDataType_U32, glm::value_ptr(val), 3, glm::value_ptr(speed), glm::value_ptr(min), glm::value_ptr(max)); }
	bool Control(std::string_view label, glm::uvec4& val, const glm::uvec4& min, const glm::uvec4& max, const glm::uvec4& speed) { return ControlScalar(label, ImGuiDataType_U32, glm::value_ptr(val), 4, glm::value_ptr(speed), glm::value_ptr(min), glm::value_ptr(max)); }
	
	bool ControlAngle(std::string_view label, float& radians,     float min,            float max,            float speed)            { auto degrees = glm::degrees(radians); const bool changed = Control(label, degrees, min, max, speed); if (changed) { radians = glm::radians(degrees); return true; } return false; }
	bool ControlAngle(std::string_view label, glm::vec2& radians, const glm::vec2& min, const glm::vec2& max, const glm::vec2& speed) { auto degrees = glm::degrees(radians); const bool changed = Control(label, degrees, min, max, speed); if (changed) { radians = glm::radians(degrees); return true; } return false; }
	bool ControlAngle(std::string_view label, glm::vec3& radians, const glm::vec3& min, const glm::vec3& max, const glm::vec3& speed) { auto degrees = glm::degrees(radians); const bool changed = Control(label, degrees, min, max, speed); if (changed) { radians = glm::radians(degrees); return true; } return false; }
	bool ControlAngle(std::string_view label, glm::vec4& radians, const glm::vec4& min, const glm::vec4& max, const glm::vec4& speed) { auto degrees = glm::degrees(radians); const bool changed = Control(label, degrees, min, max, speed); if (changed) { radians = glm::radians(degrees); return true; } return false; }

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
		const ImGuiID id = GetID(label);
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
		UI::ScopedColorStack s;
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

	bool Search(ImGuiID id, char* buffer, int bufferSize)
	{
		ScopedID scopedID(id);
		const bool changed = ImGui::InputTextWithHint("##search", "Search ...", buffer, bufferSize);

		const float buttonSize = ImGui::GetItemRectSize().y;
		ImGui::SameLine(0, 0);
		MoveCursorX(-buttonSize);

		ScopedStyle frameBorder(ImGuiStyleVar_FrameBorderSize, 0.0f);
		ScopedColorStack colors(
			ImGuiCol_Button, ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f },
			ImGuiCol_ButtonActive, ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f },
			ImGuiCol_ButtonHovered, ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f }
		);

		ImGuiLastItemData lastItemData = GImGui->LastItemData;
		ImGui::BeginChild(id, ImVec2(buttonSize, buttonSize));
		const bool clear = ImGui::Button("x", { buttonSize, buttonSize });
		ImGui::EndChild();
		GImGui->LastItemData = lastItemData;

		if (clear)
		{
			memset(buffer, '\0', bufferSize);
			ImGui::SetKeyboardFocusHere(-1);
		}

		return changed || clear;
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
