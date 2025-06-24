#pragma once

#include "Shark/UI/Theme.h"

#include "Shark/UI/ImGui/ImGuiHelpers.h"
#include "Shark/UI/ImGui/ImGuiFonts.h"	

#include <imgui.h>
#include <imgui_internal.h>

namespace Shark::UI {

	//////////////////////////////////////////////////////////////////
	//// Scoped Utilities

	struct ScopedID
	{
		ScopedID(ImGuiID id) { ImGui::PushOverrideID(id); }
		ScopedID(UUID id) { ImGui::PushID(id); }
		ScopedID(int intID) { ImGui::PushID(intID); }
		ScopedID(void* ptrID) { ImGui::PushID(ptrID); }
		ScopedID(const char* strID, const char* strEnd) { ImGui::PushID(strID, strEnd); }
		ScopedID(const std::string& strID) { ImGui::PushID(strID); }
		ScopedID(const std::string_view& strID) { ImGui::PushID(strID); }
		~ScopedID() { ImGui::PopID(); }
	};

	struct ScopedStyle
	{
		ScopedStyle(ImGuiStyleVar style, float val) { ImGui::PushStyleVar(style, val); }
		ScopedStyle(ImGuiStyleVar style, const ImVec2& val) { ImGui::PushStyleVar(style, val); }
		~ScopedStyle() { ImGui::PopStyleVar(); }
	};

	template<bool TConditional = false>
	struct ScopedColor
	{
		ScopedColor(ImGuiCol color, const ImColor& val)
		{
			ImGui::PushStyleColor(color, val.Value);
		}

		ScopedColor(ImGuiCol color, const ImColor& val, bool push) requires (TConditional)
			: m_Pushed(push)
		{
			if (push)
				ImGui::PushStyleColor(color, val.Value);
		}

		ScopedColor(ImGuiCol color, Colors::DefaultColor)
		{
			ImGui::PushStyleColor(color, Colors::GetDefaultColor(color));
		}

		~ScopedColor()
		{
			if constexpr (!TConditional)
				ImGui::PopStyleColor();
			else if (m_Pushed)
				ImGui::PopStyleColor();
		}

	private:
		bool m_Pushed = false;
	};

	ScopedColor(ImGuiCol, ImColor, bool)->ScopedColor<true>;

	class ScopedStyleStack
	{
	public:
		ScopedStyleStack() = default;

		ScopedStyleStack(ImGuiStyleVar style, const ImVec2& val)
		{
			ImGui::PushStyleVar(style, val);
			m_Count++;
		}

		ScopedStyleStack(ImGuiStyleVar style, const float& val)
		{
			ImGui::PushStyleVar(style, val);
			m_Count++;
		}

		template<typename... TArgs>
		ScopedStyleStack(ImGuiStyleVar style, const ImVec2& val, TArgs&&... args)
			: ScopedStyleStack(args...)
		{
			ImGui::PushStyleVar(style, val);
			m_Count++;
		}

		template<typename... TArgs>
		ScopedStyleStack(ImGuiStyleVar style, const float& val, TArgs&&... args)
			: ScopedStyleStack(args...)
		{
			ImGui::PushStyleVar(style, val);
			m_Count++;
		}

		~ScopedStyleStack()
		{
			ImGui::PopStyleVar(m_Count);
		}

	private:
		uint32_t m_Count = 0;
	};

	class ScopedColorStack
	{
	public:
		ScopedColorStack() = default;

		ScopedColorStack(ImGuiCol color, const ImColor& val)
		{
			ImGui::PushStyleColor(color, val.Value);
			m_Count++;
		}

		template<typename... TArgs>
		ScopedColorStack(ImGuiCol color, const ImColor& val, TArgs&&... args)
			: ScopedColorStack(args...)
		{
			ImGui::PushStyleColor(color, val.Value);
			m_Count++;
		}

		ScopedColorStack(ImGuiCol color, Colors::DefaultColor)
		{
			ImGui::PushStyleColor(color, Colors::GetDefaultColor(color));
			m_Count++;
		}

		template<typename... TArgs>
		ScopedColorStack(ImGuiCol color, Colors::DefaultColor, TArgs&&... args)
			: ScopedColorStack(args...)
		{
			ImGui::PushStyleColor(color, Colors::GetDefaultColor(color));
			m_Count++;
		}

		~ScopedColorStack()
		{
			ImGui::PopStyleColor(m_Count);
		}

	private:
		uint32_t m_Count = 0;
	};

	struct ScopedItemFlag
	{
		ScopedItemFlag(ImGuiItemFlags flag, bool enabled = true) { ImGui::PushItemFlag(flag, enabled); }
		~ScopedItemFlag() { ImGui::PopItemFlag(); }
	};

	struct ScopedDisabled
	{
		ScopedDisabled(bool disable = true) { ImGui::BeginDisabled(disable); }
		~ScopedDisabled() { ImGui::EndDisabled(); }
	};

	struct ScopedFont
	{
		ScopedFont(const char* name) { Fonts::Push(name); }
		~ScopedFont() { Fonts::Pop(); }
	};

	struct ScopedClipRect
	{
		ScopedClipRect(const ImVec2& clip_rect_min, const ImVec2& clip_rect_max, bool intersect_with_current_clip_rect = false) { ImGui::PushClipRect(clip_rect_min, clip_rect_max, intersect_with_current_clip_rect); }
		ScopedClipRect(const ImRect& clip_rect, bool intersect_with_current_clip_rect = false) { ImGui::PushClipRect(clip_rect.Min, clip_rect.Max, intersect_with_current_clip_rect); }
		~ScopedClipRect() { ImGui::PopClipRect(); }
	};

	struct ScopedIndent
	{
		ScopedIndent(float indent = 0) : Indent(indent) { ImGui::Indent(indent); }
		~ScopedIndent() { ImGui::Unindent(Indent); }

		float Indent;
	};

	//////////////////////////////////////////////////////////////////
	//// Utilities

	//////////////////////////////////////////////////////////////////
	//// Rectangle

	ImRect GetItemRect();
	ImRect RectExpand(const ImRect& rect, float x, float y);
	ImRect RectExpand(const ImRect& rect, const ImVec2& xy);
	ImRect RectOffset(const ImRect& rect, float x, float y);
	ImRect RectOffset(const ImRect& rect, const ImVec2& offset);
	ImRect RectFromSize(const ImVec2& topLeft, const ImVec2& size);

	//////////////////////////////////////////////////////////////////
	//// Draw

	namespace Draw {

		inline void ActivityOutline(ImRect itemRect, ImColor outlineColor = Colors::Theme::Highlight, float rounding = GImGui->Style.FrameRounding)
		{
			if (ImGui::GetItemFlags() & ImGuiItemFlags_Disabled)
				return;

			auto drawList = ImGui::GetWindowDrawList();
			const ImRect outlineRect = RectExpand(itemRect, 1.0f, 1.0f);
			if (ImGui::IsItemActive())
			{
				drawList->AddRect(outlineRect.Min, outlineRect.Max, outlineColor, rounding, 0, 1.5f);
			}
			else if (ImGui::IsItemHovered())
			{
				drawList->AddRect(outlineRect.Min, outlineRect.Max, ImColor(60, 60, 60), rounding, 0, 1.5f);
			}
		}

		inline void ItemActivityOutline(ImColor outlineColor = Colors::Theme::Highlight, float rounding = GImGui->Style.FrameRounding)
		{
			if (ImGui::GetItemFlags() & ImGuiItemFlags_Disabled)
				return;

			ActivityOutline(UI::GetItemRect(), outlineColor, rounding);
		}

	}

	//////////////////////////////////////////////////////////////////
	//// ImGui Controls

	bool DragScalar(const char* label, ImGuiDataType data_type, void* p_data, float v_speed, const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags);
	bool DragScalarN(const char* label, ImGuiDataType data_type, void* p_data, int components, float v_speed, const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags);
	bool SliderScalar(const char* label, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags);
	bool SliderScalarN(const char* label, ImGuiDataType data_type, void* p_data, int components, const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags);

	bool DragFloat(const char* label, float* v, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", ImGuiSliderFlags flags = 0);     // If v_min >= v_max we have no bound
	bool DragFloat2(const char* label, float v[2], float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
	bool DragFloat3(const char* label, float v[3], float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
	bool DragFloat4(const char* label, float v[4], float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
	bool DragFloatRange2(const char* label, float* v_current_min, float* v_current_max, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", const char* format_max = NULL, ImGuiSliderFlags flags = 0);

	bool SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0);     // adjust format to decorate the value with a prefix or a suffix for in-slider labels or unit display.
	bool SliderFloat2(const char* label, float v[2], float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
	bool SliderFloat3(const char* label, float v[3], float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
	bool SliderFloat4(const char* label, float v[4], float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
	bool SliderAngle(const char* label, float* v_rad, float v_degrees_min = -360.0f, float v_degrees_max = +360.0f, const char* format = "%.0f deg", ImGuiSliderFlags flags = 0);

	bool DragDouble(const char* label, double* v, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3", ImGuiSliderFlags flags = 0);
	bool SliderDouble(const char* label, double* v, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3", ImGuiSliderFlags flags = 0);

	bool DragInt8(const char* label, int8_t* v, float v_Speed = 1.0f, int8_t v_min = 0, int8_t v_max = 0, const char* format = nullptr, ImGuiSliderFlags flags = 0);
	bool DragInt16(const char* label, int16_t* v, float v_Speed = 1.0f, int16_t v_min = 0, int16_t v_max = 0, const char* format = nullptr, ImGuiSliderFlags flags = 0);
	bool DragInt32(const char* label, int32_t* v, float v_Speed = 1.0f, int32_t v_min = 0, int32_t v_max = 0, const char* format = nullptr, ImGuiSliderFlags flags = 0);
	bool DragInt64(const char* label, int64_t* v, float v_Speed = 1.0f, int64_t v_min = 0, int64_t v_max = 0, const char* format = nullptr, ImGuiSliderFlags flags = 0);

	bool DragUInt8(const char* label, uint8_t* v, float v_Speed = 1.0f, uint8_t v_min = 0, uint8_t v_max = 0, const char* format = nullptr, ImGuiSliderFlags flags = 0);
	bool DragUInt16(const char* label, uint16_t* v, float v_Speed = 1.0f, uint16_t v_min = 0, uint16_t v_max = 0, const char* format = nullptr, ImGuiSliderFlags flags = 0);
	bool DragUInt32(const char* label, uint32_t* v, float v_Speed = 1.0f, uint32_t v_min = 0, uint32_t v_max = 0, const char* format = nullptr, ImGuiSliderFlags flags = 0);
	bool DragUInt64(const char* label, uint64_t* v, float v_Speed = 1.0f, uint64_t v_min = 0, uint64_t v_max = 0, const char* format = nullptr, ImGuiSliderFlags flags = 0);

	bool SliderInt8(const char* label, int8_t* v, int8_t v_min = 0, int8_t v_max = 0, const char* format = nullptr, ImGuiSliderFlags flags = 0);
	bool SliderInt16(const char* label, int16_t* v, int16_t v_min = 0, int16_t v_max = 0, const char* format = nullptr, ImGuiSliderFlags flags = 0);
	bool SliderInt32(const char* label, int32_t* v, int32_t v_min = 0, int32_t v_max = 0, const char* format = nullptr, ImGuiSliderFlags flags = 0);
	bool SliderInt64(const char* label, int64_t* v, int64_t v_min = 0, int64_t v_max = 0, const char* format = nullptr, ImGuiSliderFlags flags = 0);

	bool SliderUInt8(const char* label, uint8_t* v, uint8_t v_min = 0, uint8_t v_max = 0, const char* format = nullptr, ImGuiSliderFlags flags = 0);
	bool SliderUInt16(const char* label, uint16_t* v, uint16_t v_min = 0, uint16_t v_max = 0, const char* format = nullptr, ImGuiSliderFlags flags = 0);
	bool SliderUInt32(const char* label, uint32_t* v, uint32_t v_min = 0, uint32_t v_max = 0, const char* format = nullptr, ImGuiSliderFlags flags = 0);
	bool SliderUInt64(const char* label, uint64_t* v, uint64_t v_min = 0, uint64_t v_max = 0, const char* format = nullptr, ImGuiSliderFlags flags = 0);

	bool ColorEdit3(const char* label, float col[3], ImGuiColorEditFlags flags = 0);
	bool ColorEdit4(const char* label, float col[4], ImGuiColorEditFlags flags = 0);

	bool Checkbox(const char* label, bool* v);

	bool InputText(const char* label, char* buf, size_t buf_size, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = NULL, void* user_data = NULL);
	bool InputText(const char* label, std::string* str, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr);
	bool InputTextWithHint(const char* label, const char* hint, char* buf, size_t buf_size, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = NULL, void* user_data = NULL);
	bool InputTextWithHint(const char* label, const char* hint, std::string* str, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr);
	bool InputTextMultiline(const char* label, char* buf, size_t buf_size, const ImVec2& size = ImVec2(0, 0), ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = NULL, void* user_data = NULL);
	bool InputTextMultiline(const char* label, std::string* str, const ImVec2& size = ImVec2(0, 0), ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr);

	bool BeginCombo(const char* label, const char* preview_value, ImGuiComboFlags flags = 0);
	void EndCombo();

}
