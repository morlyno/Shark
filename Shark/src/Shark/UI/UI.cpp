#include "skpch.h"
#include "UI.h"

#include "Shark/Core/Application.h"
#include "Shark/Core/Buffer.h"
#include "Shark/Asset/ResourceManager.h"

#include "Shark/Math/Math.h"
#include "Shark/File/FileSystem.h"
#include "Shark/Utils/String.h"

#include "Shark/UI/Theme.h"

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

	void PushFramedTextAlign(const ImVec2& align)
	{
		GContext->FramedTextAlignStack.push(GContext->FramedTextAlign);
		GContext->FramedTextAlign = align;
	}

	void PopFramedTextAlign()
	{
		GContext->FramedTextAlign = GContext->FramedTextAlignStack.top();
		GContext->FramedTextAlignStack.pop();
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

	bool Control(std::string_view label, std::filesystem::path& path, const char* dragDropType)
	{
		if (!ControlBeginHelper(label))
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

		if (dragDropType)
		{
			if (ImGui::BeginDragDropTarget())
			{
				const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(dragDropType);
				if (payload)
				{
					char payloadPath[260];
					strcpy_s(payloadPath, std::min(260, payload->DataSize), (const char*)payload->Data);
					path = payloadPath;
					changed = true;
				}
				ImGui::EndDragDropTarget();
			}
		}

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
			UI::ScopedColor button(ImGuiCol_Button, { 0.0f, 0.0f, 0.0f, 0.0f });
			UI::ScopedColor buttonHovered(ImGuiCol_ButtonHovered, { 0.0f, 0.0f, 0.0f, 0.0f });
			UI::ScopedColor buttonActive(ImGuiCol_ButtonActive, { 0.0f, 0.0f, 0.0f, 0.0f });

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

	bool ControlAssetPath(std::string_view label, AssetHandle& assetHandle, const char* dragDropType)
	{
		if (!ControlBeginHelper(label))
			return false;

		ControlHelperDrawLabel(label);

		bool changed = false;
		const auto& metadata = ResourceManager::GetMetaData(assetHandle);

		std::string path = metadata.FilePath.string();
		TextFramed(path);

		if (dragDropType)
		{
			if (ImGui::BeginDragDropTarget())
			{
				const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(dragDropType);
				if (payload)
				{
					assetHandle = *(AssetHandle*)payload->Data;
					changed = true;
				}
				ImGui::EndDragDropTarget();
			}
		}

		ControlEndHelper();
		return changed;
	}

	bool ControlAssetPath(std::string_view label, std::filesystem::path& assetPath, const char* dragDropType)
	{
		if (!ControlBeginHelper(label))
			return false;

		ControlHelperDrawLabel(label);

		std::string strPath = assetPath.generic_string();
		bool changed = false;

		ImGui::SetNextItemWidth(-1.0f);
		if (ImGui::InputText("##control", &strPath))
		{
			assetPath = strPath;
			changed = true;
		}

		if (dragDropType)
		{
			if (ImGui::BeginDragDropTarget())
			{
				const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(dragDropType);
				if (payload)
				{
					char payloadPath[260];
					strcpy_s(payloadPath, std::min(260, payload->DataSize), (const char*)payload->Data);
					assetPath = payloadPath;
					changed = true;
				}
				ImGui::EndDragDropTarget();
			}
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

	void Property(std::string_view label, bool value)
	{
		if (!ControlBeginHelper(label))
			return;

		ControlHelperDrawLabel(label);
		ImGui::ReadOnlyCheckbox("##property", value);
		ControlEndHelper();
	}

	void Property(std::string_view label, TimeStep timestep)
	{
		Property(label, timestep.ToString());
	}

	void PropertyColor(std::string_view label, const glm::vec4& color)
	{
		if (!ControlBeginHelper(label))
			return;

		ControlHelperDrawLabel(label);
		glm::vec4 c = color;
		ImGui::SetNextItemWidth(-1.0f);
		ImGui::ColorEdit4("##property", glm::value_ptr(c));
		//ImGui::ReadOnlyCheckbox("##property", value);
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
		ImVec2 size = { ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight() };
		
		if (GImGui->NextItemData.Flags & ImGuiNextItemDataFlags_HasWidth)
			size.x = GImGui->NextItemData.Width;

		const ImRect textRect = { pos + style.FramePadding, pos + size - style.FramePadding };
		const ImRect frameRect = { pos, pos + size };

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
		ImGui::RenderFrame(frameRect.Min, frameRect.Max, frameColor, true, style.FrameRounding);
		ImGui::RenderTextClipped(textRect.Min, textRect.Max, text, text_end, nullptr, GContext->FramedTextAlign, &textRect);

		UI::ScopedID scopedID(text, text_end);
		if (ImGui::BeginPopupContextItem("TextSettingsPopup"))
		{
			if (ImGui::Selectable("Copy"))
				ImGui::SetClipboardText(text);

			ImGui::EndPopup();
		}
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

	bool InputPath(const char* label, char* buffer, int bufferSize, bool& out_InvalidInput)
	{
		auto filter = [](ImGuiInputTextCallbackData* data) -> int
		{
			if (FileSystem::InvalidCharactersW.find(data->EventChar) != std::wstring_view::npos)
			{
				*(bool*)data->UserData = true;
				return 1;
			}
			return 0;
		};

		return ImGui::InputText(label, buffer, bufferSize, ImGuiInputTextFlags_CallbackCharFilter, filter, &out_InvalidInput);
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
		SK_CORE_ASSERT(GContext->FramedTextAlignStack.size() == 0);
	}

}
