#include "skpch.h"
#include "UIUtilities.h"

#include "Shark/UI/UICore.h"

#include <misc/cpp/imgui_stdlib.h>

namespace Shark {

	glm::vec4 UI::Convert(ImVec4 vec)
	{
		return { vec.x, vec.y, vec.z, vec.w };
	}

	glm::vec4 UI::Convert(ImColor color)
	{
		return Convert(color.Value);
	}

	ImVec4 UI::Convert(glm::vec4 vec4)
	{
		return { vec4.x, vec4.y, vec4.z, vec4.w };
	}

	ImRect UI::GetItemRect()
	{
		return ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
	}

	ImRect UI::RectExpand(const ImRect& rect, float x, float y)
	{
		ImRect result = rect;
		result.Min.x -= x;
		result.Min.y -= y;
		result.Max.x += x;
		result.Max.y += y;
		return result;
	}

	ImRect UI::RectExpand(const ImRect& rect, const ImVec2& xy)
	{
		return RectExpand(rect, xy.x, xy.y);
	}

	ImRect UI::RectExpand(const ImRect& rect, float left, float right, float top, float bottom)
	{
		ImRect result = rect;
		result.Min.x -= left;
		result.Min.y -= top;
		result.Max.x += right;
		result.Max.y += bottom;
		return result;
	}

	ImRect UI::RectOffset(const ImRect& rect, float x, float y)
	{
		ImRect result = rect;
		result.Min.x += x;
		result.Min.y += y;
		result.Max.x += x;
		result.Max.y += y;
		return result;
	}

	ImRect UI::RectOffset(const ImRect& rect, const ImVec2& offset)
	{
		return RectOffset(rect, offset.x, offset.y);
	}

	ImRect UI::RectFromSize(const ImVec2& topLeft, const ImVec2& size)
	{
		return ImRect(topLeft, topLeft + size);
	}

	//////////////////////////////////////////////////////////////////
	//// ImGui Controls

	bool UI::DragScalar(const char* label, ImGuiDataType data_type, void* p_data, float v_speed, const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags)
	{
		const bool modified = ImGui::DragScalar(label, data_type, p_data, v_speed, p_min, p_max, format, flags);
		Draw::ItemActivityOutline();
		return modified;
	}

	bool UI::DragScalarN(const char* label, ImGuiDataType data_type, void* p_data, int components, float v_speed, const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags)
	{
		const bool modified = ImGui::DragScalarN(label, data_type, p_data, components, v_speed, p_min, p_max, format, flags);
		Draw::ItemActivityOutline();
		return modified;
	}

	bool UI::SliderScalar(const char* label, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags)
	{
		const bool modified = ImGui::SliderScalar(label, data_type, p_data, p_min, p_max, format, flags);
		Draw::ItemActivityOutline();
		return modified;
	}

	bool UI::SliderScalarN(const char* label, ImGuiDataType data_type, void* p_data, int components, const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags)
	{
		const bool modified = ImGui::SliderScalarN(label, data_type, p_data, components, p_min, p_max, format, flags);
		Draw::ItemActivityOutline();
		return modified;
	}

	bool UI::DragFloat(const char* label, float* v, float v_speed, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
	{
		const bool modified = ImGui::DragFloat(label, v, v_speed, v_min, v_max, format, flags);
		Draw::ItemActivityOutline();
		return modified;
	}

	bool UI::DragFloat2(const char* label, float v[2], float v_speed, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
	{
		const bool modified = ImGui::DragFloat2(label, v, v_speed, v_min, v_max, format, flags);
		Draw::ItemActivityOutline();
		return modified;
	}

	bool UI::DragFloat3(const char* label, float v[3], float v_speed, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
	{
		const bool modified = ImGui::DragFloat3(label, v, v_speed, v_min, v_max, format, flags);
		Draw::ItemActivityOutline();
		return modified;
	}

	bool UI::DragFloat4(const char* label, float v[4], float v_speed, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
	{
		const bool modified = ImGui::DragFloat4(label, v, v_speed, v_min, v_max, format, flags);
		Draw::ItemActivityOutline();
		return modified;
	}

	bool UI::DragFloatRange2(const char* label, float* v_current_min, float* v_current_max, float v_speed, float v_min, float v_max, const char* format, const char* format_max, ImGuiSliderFlags flags)
	{
		const bool modified = ImGui::DragFloatRange2(label, v_current_min, v_current_max, v_speed, v_min, v_max, format, format_max, flags);
		Draw::ItemActivityOutline();
		return modified;
	}

	bool UI::SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
	{
		const bool modified = ImGui::SliderFloat(label, v, v_min, v_max, format, flags);
		Draw::ItemActivityOutline();
		return modified;
	}

	bool UI::SliderFloat2(const char* label, float v[2], float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
	{
		const bool modified = ImGui::SliderFloat2(label, v, v_min, v_max, format, flags);
		Draw::ItemActivityOutline();
		return modified;
	}

	bool UI::SliderFloat3(const char* label, float v[3], float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
	{
		const bool modified = ImGui::SliderFloat3(label, v, v_min, v_max, format, flags);
		Draw::ItemActivityOutline();
		return modified;
	}

	bool UI::SliderFloat4(const char* label, float v[4], float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
	{
		const bool modified = ImGui::SliderFloat4(label, v, v_min, v_max, format, flags);
		Draw::ItemActivityOutline();
		return modified;
	}

	bool UI::SliderAngle(const char* label, float* v_rad, float v_degrees_min, float v_degrees_max, const char* format, ImGuiSliderFlags flags)
	{
		const bool modified = ImGui::SliderAngle(label, v_rad, v_degrees_min, v_degrees_max, format, flags);
		Draw::ItemActivityOutline();
		return modified;
	}

	bool UI::DragDouble(const char* label, double* v, float v_speed, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
	{
		const bool modified = DragScalar(label, ImGuiDataType_Double, v, v_speed, &v_min, &v_max, format, flags);
		Draw::ItemActivityOutline();
		return modified;
	}

	bool UI::SliderDouble(const char* label, double* v, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
	{
		const bool modified = SliderScalar(label, ImGuiDataType_Double, v, &v_min, &v_max, format, flags);
		Draw::ItemActivityOutline();
		return modified;
	}

	bool UI::DragInt8(const char* label, int8_t* v, float v_Speed, int8_t v_min, int8_t v_max, const char* format, ImGuiSliderFlags flags)
	{
		return DragScalar(label, ImGuiDataType_S8, v, v_Speed, &v_min, &v_max, format, flags);
	}

	bool UI::DragInt16(const char* label, int16_t* v, float v_Speed, int16_t v_min, int16_t v_max, const char* format, ImGuiSliderFlags flags)
	{
		return DragScalar(label, ImGuiDataType_S16, v, v_Speed, &v_min, &v_max, format, flags);
	}

	bool UI::DragInt32(const char* label, int32_t* v, float v_Speed, int32_t v_min, int32_t v_max, const char* format, ImGuiSliderFlags flags)
	{
		return DragScalar(label, ImGuiDataType_S32, v, v_Speed, &v_min, &v_max, format, flags);
	}

	bool UI::DragInt64(const char* label, int64_t* v, float v_Speed, int64_t v_min, int64_t v_max, const char* format, ImGuiSliderFlags flags)
	{
		return DragScalar(label, ImGuiDataType_S64, v, v_Speed, &v_min, &v_max, format, flags);
	}

	bool UI::DragUInt8(const char* label, uint8_t* v, float v_Speed, uint8_t v_min, uint8_t v_max, const char* format, ImGuiSliderFlags flags)
	{
		return DragScalar(label, ImGuiDataType_U8, v, v_Speed, &v_min, &v_max, format, flags);
	}

	bool UI::DragUInt16(const char* label, uint16_t* v, float v_Speed, uint16_t v_min, uint16_t v_max, const char* format, ImGuiSliderFlags flags)
	{
		return DragScalar(label, ImGuiDataType_U16, v, v_Speed, &v_min, &v_max, format, flags);
	}

	bool UI::DragUInt32(const char* label, uint32_t* v, float v_Speed, uint32_t v_min, uint32_t v_max, const char* format, ImGuiSliderFlags flags)
	{
		return DragScalar(label, ImGuiDataType_U32, v, v_Speed, &v_min, &v_max, format, flags);
	}

	bool UI::DragUInt64(const char* label, uint64_t* v, float v_Speed, uint64_t v_min, uint64_t v_max, const char* format, ImGuiSliderFlags flags)
	{
		return DragScalar(label, ImGuiDataType_U64, v, v_Speed, &v_min, &v_max, format, flags);
	}

	bool UI::SliderInt8(const char* label, int8_t* v, int8_t v_min, int8_t v_max, const char* format, ImGuiSliderFlags flags)
	{
		return SliderScalar(label, ImGuiDataType_S8, v, &v_min, &v_max, format, flags);
	}

	bool UI::SliderInt16(const char* label, int16_t* v, int16_t v_min, int16_t v_max, const char* format, ImGuiSliderFlags flags)
	{
		return SliderScalar(label, ImGuiDataType_S16, v, &v_min, &v_max, format, flags);
	}

	bool UI::SliderInt32(const char* label, int32_t* v, int32_t v_min, int32_t v_max, const char* format, ImGuiSliderFlags flags)
	{
		return SliderScalar(label, ImGuiDataType_S32, v, &v_min, &v_max, format, flags);
	}

	bool UI::SliderInt64(const char* label, int64_t* v, int64_t v_min, int64_t v_max, const char* format, ImGuiSliderFlags flags)
	{
		return SliderScalar(label, ImGuiDataType_S64, v, &v_min, &v_max, format, flags);
	}

	bool UI::SliderUInt8(const char* label, uint8_t* v, uint8_t v_min, uint8_t v_max, const char* format, ImGuiSliderFlags flags)
	{
		return SliderScalar(label, ImGuiDataType_U8, v, &v_min, &v_max, format, flags);
	}

	bool UI::SliderUInt16(const char* label, uint16_t* v, uint16_t v_min, uint16_t v_max, const char* format, ImGuiSliderFlags flags)
	{
		return SliderScalar(label, ImGuiDataType_U16, v, &v_min, &v_max, format, flags);
	}

	bool UI::SliderUInt32(const char* label, uint32_t* v, uint32_t v_min, uint32_t v_max, const char* format, ImGuiSliderFlags flags)
	{
		return SliderScalar(label, ImGuiDataType_U32, v, &v_min, &v_max, format, flags);
	}

	bool UI::SliderUInt64(const char* label, uint64_t* v, uint64_t v_min, uint64_t v_max, const char* format, ImGuiSliderFlags flags)
	{
		return SliderScalar(label, ImGuiDataType_U64, v, &v_min, &v_max, format, flags);
	}

	bool UI::ColorEdit3(const char* label, float col[3], ImGuiColorEditFlags flags)
	{
		const bool modified = ImGui::ColorEdit3(label, col, flags);
		Draw::ItemActivityOutline();
		return modified;
	}

	bool UI::ColorEdit4(const char* label, float col[4], ImGuiColorEditFlags flags)
	{
		const bool modified = ImGui::ColorEdit4(label, col, flags);
		Draw::ItemActivityOutline();
		return modified;
	}

	bool UI::Checkbox(const char* label, bool* v)
	{
		const bool modified = ImGui::Checkbox(label, v);
		Draw::ItemActivityOutline();
		return modified;
	}

	bool UI::CheckboxButton(const char* label, bool* v)
	{
		auto& style = ImGui::GetStyle();
		auto window = ImGui::GetCurrentWindow();

		auto textSize = ImGui::CalcTextSize(label);
		auto checkmarkSize = ImGui::GetFontSize();

		const ImVec2 size = ImVec2(checkmarkSize + style.ItemInnerSpacing.x + textSize.x, textSize.y);
		const ImVec2 frameSize = ImVec2(size.x + style.FramePadding.x * 2.0f,
										size.y + style.FramePadding.y * 2.0f);

		bool modified = false;
		if (ImGui::InvisibleButton(label, frameSize))
		{
			*v = !*v;
			modified = true;
		}

		auto rect = GetItemRect();
		DrawButton(rect);

		auto checkmarkBB = RectFromSize(rect.Min + style.FramePadding, { checkmarkSize, checkmarkSize });
		auto textBB = RectFromSize(ImVec2(checkmarkBB.Max.x + style.ItemInnerSpacing.x, checkmarkBB.Min.y),
								   textSize);

		if (*v)
		{
			const ImVec2 pos = rect.Min + style.FramePadding;
			const ImRect check_bb(pos, pos + ImVec2(checkmarkSize, checkmarkSize));
			const float pad = glm::max(1.0f, glm::trunc(checkmarkSize / 6.0f));
			ImGui::RenderCheckMark(window->DrawList, check_bb.Min + ImVec2(pad, pad), ImGui::GetColorU32(ImGuiCol_CheckMark), checkmarkSize - pad * 2.0f);
		}
		else
		{
			const float pad = glm::max(1.0f, glm::trunc(checkmarkSize / 6.0f));
			ImVec2 pos = checkmarkBB.Min + ImVec2(pad, pad);
			float sz = checkmarkSize - pad * 2.0f;

			float thickness = glm::max(sz / 5.0f, 1.0f);
			sz -= thickness * 0.5f;
			pos += ImVec2(thickness * 0.25f, thickness * 0.25f);

			auto center = ImVec2(pos.x + sz * 0.5f, pos.y + sz * 0.5f);
			auto offset = center.x - pos.x;

			window->DrawList->AddLine(center + ImVec2(-offset, -offset), center + ImVec2(+offset, +offset), ImGui::GetColorU32(ImGuiCol_CheckMark), thickness);
			window->DrawList->AddLine(center + ImVec2(-offset, +offset), center + ImVec2(+offset, -offset), ImGui::GetColorU32(ImGuiCol_CheckMark), thickness);
		}

		ImGui::RenderText(textBB.Min, label);
		return modified;
	}

	bool UI::InputText(const char* label, char* buf, size_t buf_size, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
	{
		const bool modified = ImGui::InputText(label, buf, buf_size, flags, callback, user_data);
		Draw::ItemActivityOutline();
		return modified;
	}

	bool UI::InputText(const char* label, std::string* str, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
	{
		const bool modified = ImGui::InputText(label, str, flags, callback, user_data);
		Draw::ItemActivityOutline();
		return modified;
	}

	bool UI::InputTextWithHint(const char* label, const char* hint, char* buf, size_t buf_size, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
	{
		const bool modified = ImGui::InputTextWithHint(label, hint, buf, buf_size, flags, callback, user_data);
		Draw::ItemActivityOutline();
		return modified;
	}

	bool UI::InputTextWithHint(const char* label, const char* hint, std::string* str, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
	{
		const bool modified = ImGui::InputTextWithHint(label, hint, str, flags, callback, user_data);
		Draw::ItemActivityOutline();
		return modified;
	}

	bool UI::InputTextMultiline(const char* label, char* buf, size_t buf_size, const ImVec2& size, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
	{
		const bool modified = ImGui::InputTextMultiline(label, buf, buf_size, size, flags, callback, user_data);
		Draw::ItemActivityOutline();
		return modified;
	}

	bool UI::InputTextMultiline(const char* label, std::string* str, const ImVec2& size, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
	{
		const bool modified = ImGui::InputTextMultiline(label, str, size, flags, callback, user_data);
		Draw::ItemActivityOutline();
		return modified;
	}

	bool UI::BeginCombo(const char* label, const char* preview_value, ImGuiComboFlags flags)
	{
		const bool isOpen = ImGui::BeginCombo(label, preview_value, flags);
		Draw::ItemActivityOutline();
		return isOpen;
	}

	void UI::EndCombo()
	{
		ImGui::EndCombo();
	}

}
