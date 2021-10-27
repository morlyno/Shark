#include "skpch.h"
#include "UI.h"

#include "Shark/Utility/Math.h"
#include "Shark/Render/Renderer.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <misc/cpp/imgui_stdlib.h>


namespace Shark::UI {
	inline namespace Deprecated {

		static inline void ImGuiCallbackFunctionBlend(const ImDrawList* parent_list, const ImDrawCmd* cmd)
		{
			Renderer::GetRendererAPI()->SetBlendForImgui((bool)cmd->UserCallbackData);
		}

		SK_DEPRECATED("NoAlpaImage is deprecated, probaly a replacement very soon")
		void NoAlpaImage(ImTextureID textureID, const ImVec2 & size, const ImVec2 & uv0, const ImVec2 & uv1, const ImVec4 & tintcolor, const ImVec4 & bordercolor)
		{
			ImGuiWindow* window = ImGui::GetCurrentWindow();
			if (window->SkipItems)
				return;

			//window->DrawList->AddCallback(ImGuiCallbackFunctionBlend, (void*)false);
			ImGui::Image(textureID, size, uv0, uv1, tintcolor, bordercolor);
			//window->DrawList->AddCallback(ImGuiCallbackFunctionBlend, (void*)true);
		}
	}
}

namespace ImGuiEx {

	using namespace ImGui;

	static bool Checkbox(ImGuiID id, const char* label, bool* v)
	{
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImVec2 label_size = CalcTextSize(label, NULL, true);

		const float square_sz = GetFrameHeight();
		const ImVec2 pos = window->DC.CursorPos;
		const ImRect total_bb(pos, pos + ImVec2(square_sz + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f), label_size.y + style.FramePadding.y * 2.0f));
		ItemSize(total_bb, style.FramePadding.y);
		if (!ItemAdd(total_bb, id))
		{
			IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.LastItemStatusFlags | ImGuiItemStatusFlags_Checkable | (*v ? ImGuiItemStatusFlags_Checked : 0));
			return false;
		}

		bool hovered, held;
		bool pressed = ButtonBehavior(total_bb, id, &hovered, &held);
		if (pressed)
		{
			*v = !(*v);
			MarkItemEdited(id);
		}

		const ImRect check_bb(pos, pos + ImVec2(square_sz, square_sz));
		RenderNavHighlight(total_bb, id);
		RenderFrame(check_bb.Min, check_bb.Max, GetColorU32((held && hovered) ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg), true, style.FrameRounding);
		ImU32 check_col = GetColorU32(ImGuiCol_CheckMark);
		bool mixed_value = (g.CurrentItemFlags & ImGuiItemFlags_MixedValue) != 0;
		if (mixed_value)
		{
			// Undocumented tristate/mixed/indeterminate checkbox (#2644)
			// This may seem awkwardly designed because the aim is to make ImGuiItemFlags_MixedValue supported by all widgets (not just checkbox)
			ImVec2 pad(ImMax(1.0f, IM_FLOOR(square_sz / 3.6f)), ImMax(1.0f, IM_FLOOR(square_sz / 3.6f)));
			window->DrawList->AddRectFilled(check_bb.Min + pad, check_bb.Max - pad, check_col, style.FrameRounding);
		}
		else if (*v)
		{
			const float pad = ImMax(1.0f, IM_FLOOR(square_sz / 6.0f));
			RenderCheckMark(window->DrawList, check_bb.Min + ImVec2(pad, pad), check_col, square_sz - pad * 2.0f);
		}

		ImVec2 label_pos = ImVec2(check_bb.Max.x + style.ItemInnerSpacing.x, check_bb.Min.y + style.FramePadding.y);
		if (g.LogEnabled)
			LogRenderedText(&label_pos, mixed_value ? "[~]" : *v ? "[x]" : "[ ]");
		if (label_size.x > 0.0f)
			RenderText(label_pos, label);

		IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.LastItemStatusFlags | ImGuiItemStatusFlags_Checkable | (*v ? ImGuiItemStatusFlags_Checked : 0));
		return pressed;
	}

}

namespace Shark::UI {

	//////////////////////////////////////////////////////////////////////////////
	/// Global Data //////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	static uint32_t s_ControlsCount = 0;

	void NewFrame()
	{
		// Sanity Checks
		SK_CORE_ASSERT(s_ControlsCount == 0);

		// Reset Global Data
		s_ControlsCount = 0;
	}


	//////////////////////////////////////////////////////////////////////////////
	/// Helpers //////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	ImGuiID GetID(int intID)
	{
		return ImGui::GetCurrentWindowRead()->GetID(intID);
	}

	ImGuiID GetID(void* ptrID)
	{
		return ImGui::GetCurrentWindowRead()->GetID(ptrID);
	}

	ImGuiID GetID(const std::string& strID)
	{
		return ImGui::GetCurrentWindowRead()->GetID(strID.c_str());
	}

	void PushID(ImGuiID id)
	{
		ImGui::PushID(id);
	}

	ImGuiID PopID()
	{
		const ImGuiID id = ImGui::GetCurrentWindowRead()->IDStack.back();
		ImGui::PopID();
		return id;
	}

	ImGuiID GetCurrentID()
	{
		return ImGui::GetCurrentWindowRead()->IDStack.back();
	}


	//////////////////////////////////////////////////////////////////////////////
	/// Text /////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	void Text(const std::string& str)
	{
		ImGui::Text(str.c_str());
	}

	void Text(const std::filesystem::path& path)
	{
		Text(path.string());
	}

	void TextAligned(const std::string& str)
	{
		ImGui::AlignTextToFramePadding();
		Text(str);
	}

	void TextWithBackGround(const std::string& text)
	{
		TextWithBackGround(text, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
	}

	void TextWithBackGround(const std::string& text, const ImVec4& bgColor)
	{
		ImGui::PushStyleColor(ImGuiCol_Header, bgColor);
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, bgColor);
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, bgColor);

		const ImGuiID id = GetID(text);
		ImGui::TreeNodeBehavior(id, ImGuiTreeNodeFlags_CollapsingHeader, text.c_str());

		ImGui::PopStyleColor(3);
	}

	void TextWithBackGround(const std::filesystem::path& text)
	{
		TextWithBackGround(text, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
	}

	void TextWithBackGround(const std::filesystem::path& text, const ImVec4& bgColor)
	{
		TextWithBackGround(text.string(), bgColor);
	}

	bool InputText(const std::string& tag, std::string& buffer)
	{
		return ImGui::InputText(tag.c_str(), &buffer);
	}

	bool InputText(const std::string& tag, std::filesystem::path& buffer)
	{
		std::string str = buffer.string();
		if (InputText(tag, str))
		{
			buffer = str;
			return true;
		}
		return false;
	}

	//////////////////////////////////////////////////////////////////////////////
	/// Controls /////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	bool BeginControls()
	{
		const ImGuiID tableID = GetID(s_ControlsCount);
		constexpr ImGuiTableFlags flags = ImGuiTableFlags_None | ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV;
		if (ImGui::BeginTableEx("ControlsTable", tableID, 2, flags))
		{
			s_ControlsCount++;
			ImGuiStyle& style = ImGui::GetStyle();
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { style.ItemSpacing.x * 0.5f, style.ItemSpacing.y });
			return true;
		}
		return false;
	}

	void EndControls()
	{
		if (ImGui::GetCurrentTable())
		{
			ImGui::PopStyleVar();

			ImGui::EndTable();
			s_ControlsCount--;
		}
	}


	bool DragFloat(const std::string& tag, float& val, float resetVal, float min, float max, float speed, const char* fmt, ImGuiSliderFlags flags)
	{
		// ImGui's Table API currently crashes when BeginTable return false but Talbe functions get called
		if (!ImGui::GetCurrentTable())
			return false;

		ImGui::TableNextRow();

		PushID(GetID(tag));

		ImGui::TableSetColumnIndex(0);
		Text(tag);
		ImGui::TableSetColumnIndex(1);

		ImGuiStyle& style = ImGui::GetStyle();
		const float buttonSize = ImGui::GetFrameHeight();
		const float widthAvail = ImGui::GetContentRegionAvail().x;
		const float width = widthAvail - buttonSize;
		ImGui::PushItemWidth(width);

		bool changed = false;

		if (ImGui::Button("X", { buttonSize, buttonSize }))
		{
			val = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::DragFloat("##X", &val, speed, min, max, fmt, flags);

		ImGui::PopItemWidth();

		PopID();
		return changed;
	}

	bool DragFloat(const std::string& tag, DirectX::XMFLOAT2& val, float resetVal, float min, float max, float speed, const char* fmt, ImGuiSliderFlags flags)
	{
		// ImGui's Table API currently crashes when BeginTable return false but Talbe functions get called
		if (!ImGui::GetCurrentTable())
			return false;

		ImGui::TableNextRow();

		PushID(GetID(tag));

		ImGui::TableSetColumnIndex(0);
		Text(tag);
		ImGui::TableSetColumnIndex(1);

		ImGuiStyle& style = ImGui::GetStyle();
		constexpr float components = 2.0f;
		const float buttonSize = ImGui::GetFrameHeight();
		const float widthAvail = ImGui::GetContentRegionAvail().x;
		const float width = (widthAvail - style.ItemSpacing.x * (components - 1)) / components - buttonSize;

		ImGui::PushItemWidth(width);

		bool changed = false;

		if (ImGui::Button("X", { buttonSize, buttonSize }))
		{
			val.x = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::DragFloat("##X", &val.x, speed, min, max, fmt, flags);

		ImGui::SameLine();

		if (ImGui::Button("Y", { buttonSize, buttonSize }))
		{
			val.y = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::DragFloat("##Y", &val.y, speed, min, max, fmt, flags);

		ImGui::PopItemWidth();

		PopID();
		return changed;
	}

	bool DragFloat(const std::string& tag, DirectX::XMFLOAT3& val, float resetVal, float min, float max, float speed, const char* fmt, ImGuiSliderFlags flags)
	{
		// ImGui's Table API currently crashes when BeginTable return false but Talbe functions get called
		if (!ImGui::GetCurrentTable())
			return false;

		ImGui::TableNextRow();

		PushID(GetID(tag));

		ImGui::TableSetColumnIndex(0);
		Text(tag);
		ImGui::TableSetColumnIndex(1);

		ImGuiStyle& style = ImGui::GetStyle();
		constexpr float components = 3.0f;
		const float buttonSize = ImGui::GetFrameHeight();
		const float widthAvail = ImGui::GetContentRegionAvail().x;
		const float width = (widthAvail - style.ItemSpacing.x * (components - 1)) / components - buttonSize;

		ImGui::PushItemWidth(width);

		bool changed = false;

		if (ImGui::Button("X", { buttonSize, buttonSize }))
		{
			val.x = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::DragFloat("##X", &val.x, speed, min, max, fmt, flags);

		ImGui::SameLine();

		if (ImGui::Button("Y", { buttonSize, buttonSize }))
		{
			val.y = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::DragFloat("##Y", &val.y, speed, min, max, fmt, flags);

		ImGui::SameLine();

		if (ImGui::Button("Z", { buttonSize, buttonSize }))
		{
			val.z = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::DragFloat("##Z", &val.z, speed, min, max, fmt, flags);

		ImGui::PopItemWidth();

		PopID();
		return changed;
	}

	bool DragFloat(const std::string& tag, DirectX::XMFLOAT4& val, float resetVal, float min, float max, float speed, const char* fmt, ImGuiSliderFlags flags)
	{
		// ImGui's Table API currently crashes when BeginTable return false but Talbe functions get called
		if (!ImGui::GetCurrentTable())
			return false;

		ImGui::TableNextRow();

		PushID(GetID(tag));

		ImGui::TableSetColumnIndex(0);
		Text(tag);
		ImGui::TableSetColumnIndex(1);

		ImGuiStyle& style = ImGui::GetStyle();
		constexpr float components = 4.0f;
		const float buttonSize = ImGui::GetFrameHeight();
		const float widthAvail = ImGui::GetContentRegionAvail().x;
		const float width = (widthAvail - style.ItemSpacing.x * (components - 1)) / components - buttonSize;

		ImGui::PushItemWidth(width);

		bool changed = false;

		if (ImGui::Button("X", { buttonSize, buttonSize }))
		{
			val.x = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::DragFloat("##X", &val.x, speed, min, max, fmt, flags);

		ImGui::SameLine();

		if (ImGui::Button("Y", { buttonSize, buttonSize }))
		{
			val.y = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::DragFloat("##Y", &val.y, speed, min, max, fmt, flags);

		ImGui::SameLine();

		if (ImGui::Button("Z", { buttonSize, buttonSize }))
		{
			val.z = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::DragFloat("##Z", &val.z, speed, min, max, fmt, flags);

		ImGui::SameLine();

		if (ImGui::Button("W", { buttonSize, buttonSize }))
		{
			val.w = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::DragFloat("##W", &val.w, speed, min, max, fmt, flags);

		ImGui::PopItemWidth();

		PopID();
		return changed;
	}


	bool SliderFloat(const std::string& tag, float& val, float resetVal, float min, float max, const char* fmt, ImGuiSliderFlags flags)
	{
		// ImGui's Table API currently crashes when BeginTable return false but Talbe functions get called
		if (!ImGui::GetCurrentTable())
			return false;

		ImGui::TableNextRow();

		PushID(GetID(tag));

		ImGui::TableSetColumnIndex(0);
		Text(tag);
		ImGui::TableSetColumnIndex(1);

		ImGuiStyle& style = ImGui::GetStyle();
		const float buttonSize = ImGui::GetFrameHeight();
		const float widthAvail = ImGui::GetContentRegionAvail().x;
		const float width = widthAvail - buttonSize;
		ImGui::PushItemWidth(width);

		bool changed = false;

		if (ImGui::Button("X", { buttonSize, buttonSize }))
		{
			val = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::SliderFloat("##X", &val, min, max, fmt, flags);

		ImGui::PopItemWidth();

		PopID();
		return changed;
	}

	bool SliderFloat(const std::string& tag, DirectX::XMFLOAT2& val, float resetVal, float min, float max, const char* fmt, ImGuiSliderFlags flags)
	{
		// ImGui's Table API currently crashes when BeginTable return false but Talbe functions get called
		if (!ImGui::GetCurrentTable())
			return false;

		ImGui::TableNextRow();

		PushID(GetID(tag));

		ImGui::TableSetColumnIndex(0);
		Text(tag);
		ImGui::TableSetColumnIndex(1);

		ImGuiStyle& style = ImGui::GetStyle();
		constexpr float components = 2.0f;
		const float buttonSize = ImGui::GetFrameHeight();
		const float widthAvail = ImGui::GetContentRegionAvail().x;
		const float width = (widthAvail - style.ItemSpacing.x * (components - 1)) / components - buttonSize;

		ImGui::PushItemWidth(width);

		bool changed = false;

		if (ImGui::Button("X", { buttonSize, buttonSize }))
		{
			val.x = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::SliderFloat("##X", &val.x, min, max, fmt, flags);

		ImGui::SameLine();

		if (ImGui::Button("Y", { buttonSize, buttonSize }))
		{
			val.y = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::SliderFloat("##Y", &val.y, min, max, fmt, flags);

		PopID();
		return changed;
	}

	bool SliderFloat(const std::string& tag, DirectX::XMFLOAT3& val, float resetVal, float min, float max, const char* fmt, ImGuiSliderFlags flags)
	{
		// ImGui's Table API currently crashes when BeginTable return false but Talbe functions get called
		if (!ImGui::GetCurrentTable())
			return false;

		ImGui::TableNextRow();

		PushID(GetID(tag));

		ImGui::TableSetColumnIndex(0);
		Text(tag);
		ImGui::TableSetColumnIndex(1);

		ImGuiStyle& style = ImGui::GetStyle();
		constexpr float components = 3.0f;
		const float buttonSize = ImGui::GetFrameHeight();
		const float widthAvail = ImGui::GetContentRegionAvail().x;
		const float width = (widthAvail - style.ItemSpacing.x * (components - 1)) / components - buttonSize;

		ImGui::PushItemWidth(width);

		bool changed = false;

		if (ImGui::Button("X", { buttonSize, buttonSize }))
		{
			val.x = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::SliderFloat("##X", &val.x, min, max, fmt, flags);

		ImGui::SameLine();

		if (ImGui::Button("Y", { buttonSize, buttonSize }))
		{
			val.y = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::SliderFloat("##Y", &val.y, min, max, fmt, flags);

		ImGui::SameLine();

		if (ImGui::Button("Z", { buttonSize, buttonSize }))
		{
			val.z = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::SliderFloat("##Z", &val.z, min, max, fmt, flags);

		PopID();
		return changed;
	}

	bool SliderFloat(const std::string& tag, DirectX::XMFLOAT4& val, float resetVal, float min, float max, const char* fmt, ImGuiSliderFlags flags)
	{
		// ImGui's Table API currently crashes when BeginTable return false but Talbe functions get called
		if (!ImGui::GetCurrentTable())
			return false;

		ImGui::TableNextRow();

		PushID(GetID(tag));

		ImGui::TableSetColumnIndex(0);
		Text(tag);
		ImGui::TableSetColumnIndex(1);

		ImGuiStyle& style = ImGui::GetStyle();
		constexpr float components = 4.0f;
		const float buttonSize = ImGui::GetFrameHeight();
		const float widthAvail = ImGui::GetContentRegionAvail().x;
		const float width = (widthAvail - style.ItemSpacing.x * (components - 1)) / components - buttonSize;

		ImGui::PushItemWidth(width);

		bool changed = false;

		if (ImGui::Button("X", { buttonSize, buttonSize }))
		{
			val.x = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::SliderFloat("##X", &val.x, min, max, fmt, flags);

		ImGui::SameLine();

		if (ImGui::Button("Y", { buttonSize, buttonSize }))
		{
			val.y = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::SliderFloat("##Y", &val.y, min, max, fmt, flags);

		ImGui::SameLine();

		if (ImGui::Button("Z", { buttonSize, buttonSize }))
		{
			val.z = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::SliderFloat("##Z", &val.z, min, max, fmt, flags);

		ImGui::SameLine();

		if (ImGui::Button("W", { buttonSize, buttonSize }))
		{
			val.w = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::SliderFloat("##W", &val.w, min, max, fmt, flags);

		PopID();
		return changed;
	}

	bool DragInt(const std::string& tag, int& val, int resetVal, int min, int max, float speed, const char* fmt, ImGuiSliderFlags flags)
	{
		// ImGui's Table API currently crashes when BeginTable return false but Talbe functions get called
		if (!ImGui::GetCurrentTable())
			return false;

		ImGui::TableNextRow();

		PushID(GetID(tag));

		ImGui::TableSetColumnIndex(0);
		Text(tag);
		ImGui::TableSetColumnIndex(1);

		ImGuiStyle& style = ImGui::GetStyle();
		const float buttonSize = ImGui::GetFrameHeight();
		const float widthAvail = ImGui::GetContentRegionAvail().x;
		const float width = widthAvail - buttonSize;
		ImGui::PushItemWidth(width);

		bool changed = false;

		if (ImGui::Button("X", { buttonSize, buttonSize }))
		{
			val = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::DragInt("##X", &val, speed, min, max, fmt, flags);

		ImGui::PopItemWidth();

		PopID();
		return changed;
	}

	bool DragInt(const std::string& tag, DirectX::XMINT2& val, int resetVal, int min, int max, float speed, const char* fmt, ImGuiSliderFlags flags)
	{
		// ImGui's Table API currently crashes when BeginTable return false but Talbe functions get called
		if (!ImGui::GetCurrentTable())
			return false;

		ImGui::TableNextRow();

		PushID(GetID(tag));

		ImGui::TableSetColumnIndex(0);
		Text(tag);
		ImGui::TableSetColumnIndex(1);

		ImGuiStyle& style = ImGui::GetStyle();
		constexpr float components = 2.0f;
		const float buttonSize = ImGui::GetFrameHeight();
		const float widthAvail = ImGui::GetContentRegionAvail().x;
		const float width = (widthAvail - style.ItemSpacing.x * (components - 1)) / components - buttonSize;

		ImGui::PushItemWidth(width);

		bool changed = false;

		if (ImGui::Button("X", { buttonSize, buttonSize }))
		{
			val.x = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::DragInt("##X", &val.x, speed, min, max, fmt, flags);

		ImGui::SameLine();

		if (ImGui::Button("Y", { buttonSize, buttonSize }))
		{
			val.y = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::DragInt("##Y", &val.y, speed, min, max, fmt, flags);

		ImGui::PopItemWidth();

		PopID();
		return changed;
	}

	bool DragInt(const std::string& tag, DirectX::XMINT3& val, int resetVal, int min, int max, float speed, const char* fmt, ImGuiSliderFlags flags)
	{
		// ImGui's Table API currently crashes when BeginTable return false but Talbe functions get called
		if (!ImGui::GetCurrentTable())
			return false;

		ImGui::TableNextRow();

		PushID(GetID(tag));

		ImGui::TableSetColumnIndex(0);
		Text(tag);
		ImGui::TableSetColumnIndex(1);

		ImGuiStyle& style = ImGui::GetStyle();
		constexpr float components = 3.0f;
		const float buttonSize = ImGui::GetFrameHeight();
		const float widthAvail = ImGui::GetContentRegionAvail().x;
		const float width = (widthAvail - style.ItemSpacing.x * (components - 1)) / components - buttonSize;

		ImGui::PushItemWidth(width);

		bool changed = false;

		if (ImGui::Button("X", { buttonSize, buttonSize }))
		{
			val.x = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::DragInt("##X", &val.x, speed, min, max, fmt, flags);

		ImGui::SameLine();

		if (ImGui::Button("Y", { buttonSize, buttonSize }))
		{
			val.y = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::DragInt("##Y", &val.y, speed, min, max, fmt, flags);

		ImGui::SameLine();

		if (ImGui::Button("Z", { buttonSize, buttonSize }))
		{
			val.z = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::DragInt("##Z", &val.z, speed, min, max, fmt, flags);

		ImGui::PopItemWidth();

		PopID();
		return changed;
	}

	bool DragInt(const std::string& tag, DirectX::XMINT4& val, int resetVal, int min, int max, float speed, const char* fmt, ImGuiSliderFlags flags)
	{
		// ImGui's Table API currently crashes when BeginTable return false but Talbe functions get called
		if (!ImGui::GetCurrentTable())
			return false;

		ImGui::TableNextRow();

		PushID(GetID(tag));

		ImGui::TableSetColumnIndex(0);
		Text(tag);
		ImGui::TableSetColumnIndex(1);

		ImGuiStyle& style = ImGui::GetStyle();
		constexpr float components = 4.0f;
		const float buttonSize = ImGui::GetFrameHeight();
		const float widthAvail = ImGui::GetContentRegionAvail().x;
		const float width = (widthAvail - style.ItemSpacing.x * (components - 1)) / components - buttonSize;

		ImGui::PushItemWidth(width);

		bool changed = false;

		if (ImGui::Button("X", { buttonSize, buttonSize }))
		{
			val.x = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::DragInt("##X", &val.x, speed, min, max, fmt, flags);

		ImGui::SameLine();

		if (ImGui::Button("Y", { buttonSize, buttonSize }))
		{
			val.y = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::DragInt("##Y", &val.y, speed, min, max, fmt, flags);

		ImGui::SameLine();

		if (ImGui::Button("Z", { buttonSize, buttonSize }))
		{
			val.z = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::DragInt("##Z", &val.z, speed, min, max, fmt, flags);

		ImGui::SameLine();

		if (ImGui::Button("W", { buttonSize, buttonSize }))
		{
			val.w = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::DragInt("##W", &val.w, speed, min, max, fmt, flags);

		ImGui::PopItemWidth();

		PopID();
		return changed;
	}


	bool SliderInt(const std::string& tag, int& val, int resetVal, int min, int max, const char* fmt, ImGuiSliderFlags flags)
	{
		// ImGui's Table API currently crashes when BeginTable return false but Talbe functions get called
		if (!ImGui::GetCurrentTable())
			return false;

		ImGui::TableNextRow();

		PushID(GetID(tag));

		ImGui::TableSetColumnIndex(0);
		Text(tag);
		ImGui::TableSetColumnIndex(1);

		ImGuiStyle& style = ImGui::GetStyle();
		const float buttonSize = ImGui::GetFrameHeight();
		const float widthAvail = ImGui::GetContentRegionAvail().x;
		const float width = widthAvail - buttonSize;
		ImGui::PushItemWidth(width);

		bool changed = false;

		if (ImGui::Button("X", { buttonSize, buttonSize }))
		{
			val = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::SliderInt("##X", &val, min, max, fmt, flags);

		ImGui::PopItemWidth();

		PopID();
		return changed;
	}

	bool SliderInt(const std::string& tag, DirectX::XMINT2& val, int resetVal, int min, int max, const char* fmt, ImGuiSliderFlags flags)
	{
		// ImGui's Table API currently crashes when BeginTable return false but Talbe functions get called
		if (!ImGui::GetCurrentTable())
			return false;

		ImGui::TableNextRow();

		PushID(GetID(tag));

		ImGui::TableSetColumnIndex(0);
		Text(tag);
		ImGui::TableSetColumnIndex(1);

		ImGuiStyle& style = ImGui::GetStyle();
		constexpr float components = 2.0f;
		const float buttonSize = ImGui::GetFrameHeight();
		const float widthAvail = ImGui::GetContentRegionAvail().x;
		const float width = (widthAvail - style.ItemSpacing.x * (components - 1)) / components - buttonSize;

		ImGui::PushItemWidth(width);

		bool changed = false;

		if (ImGui::Button("X", { buttonSize, buttonSize }))
		{
			val.x = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::SliderInt("##X", &val.x, min, max, fmt, flags);

		ImGui::SameLine();

		if (ImGui::Button("Y", { buttonSize, buttonSize }))
		{
			val.y = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::SliderInt("##Y", &val.y, min, max, fmt, flags);

		PopID();
		return changed;
	}

	bool SliderInt(const std::string& tag, DirectX::XMINT3& val, int resetVal, int min, int max, const char* fmt, ImGuiSliderFlags flags)
	{
		// ImGui's Table API currently crashes when BeginTable return false but Talbe functions get called
		if (!ImGui::GetCurrentTable())
			return false;

		ImGui::TableNextRow();

		PushID(GetID(tag));

		ImGui::TableSetColumnIndex(0);
		Text(tag);
		ImGui::TableSetColumnIndex(1);

		ImGuiStyle& style = ImGui::GetStyle();
		constexpr float components = 3.0f;
		const float buttonSize = ImGui::GetFrameHeight();
		const float widthAvail = ImGui::GetContentRegionAvail().x;
		const float width = (widthAvail - style.ItemSpacing.x * (components - 1)) / components - buttonSize;

		ImGui::PushItemWidth(width);

		bool changed = false;

		if (ImGui::Button("X", { buttonSize, buttonSize }))
		{
			val.x = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::SliderInt("##X", &val.x, min, max, fmt, flags);

		ImGui::SameLine();

		if (ImGui::Button("Y", { buttonSize, buttonSize }))
		{
			val.y = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::SliderInt("##Y", &val.y, min, max, fmt, flags);

		ImGui::SameLine();

		if (ImGui::Button("Z", { buttonSize, buttonSize }))
		{
			val.z = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::SliderInt("##Z", &val.z, min, max, fmt, flags);

		PopID();
		return changed;
	}

	bool SliderInt(const std::string& tag, DirectX::XMINT4& val, int resetVal, int min, int max, const char* fmt, ImGuiSliderFlags flags)
	{
		// ImGui's Table API currently crashes when BeginTable return false but Talbe functions get called
		if (!ImGui::GetCurrentTable())
			return false;

		ImGui::TableNextRow();

		PushID(GetID(tag));

		ImGui::TableSetColumnIndex(0);
		Text(tag);
		ImGui::TableSetColumnIndex(1);

		ImGuiStyle& style = ImGui::GetStyle();
		constexpr float components = 4.0f;
		const float buttonSize = ImGui::GetFrameHeight();
		const float widthAvail = ImGui::GetContentRegionAvail().x;
		const float width = (widthAvail - style.ItemSpacing.x * (components - 1)) / components - buttonSize;

		ImGui::PushItemWidth(width);

		bool changed = false;

		if (ImGui::Button("X", { buttonSize, buttonSize }))
		{
			val.x = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::SliderInt("##X", &val.x, min, max, fmt, flags);

		ImGui::SameLine();

		if (ImGui::Button("Y", { buttonSize, buttonSize }))
		{
			val.y = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::SliderInt("##Y", &val.y, min, max, fmt, flags);

		ImGui::SameLine();

		if (ImGui::Button("Z", { buttonSize, buttonSize }))
		{
			val.z = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::SliderInt("##Z", &val.z, min, max, fmt, flags);

		ImGui::SameLine();

		if (ImGui::Button("W", { buttonSize, buttonSize }))
		{
			val.w = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::SliderInt("##W", &val.w, min, max, fmt, flags);

		PopID();
		return changed;
	}


	bool DragAngle(const std::string& tag, float& radians, float resetVal, float min, float max, float speed, const char* fmt, ImGuiSliderFlags flags)
	{
		auto degrees = Math::ToDegrees(radians);
		if (DragFloat(tag, degrees, resetVal, speed, min, max, fmt, flags))
		{
			radians = Math::ToRadians(degrees);
			return true;
		}
		return false;
	}

	bool DragAngle(const std::string& tag, DirectX::XMFLOAT2& radians, float resetVal, float min, float max, float speed, const char* fmt, ImGuiSliderFlags flags)
	{
		auto degrees = Math::ToDegrees(radians);
		if (DragFloat(tag, degrees, resetVal, speed, min, max, fmt, flags))
		{
			radians = Math::ToRadians(degrees);
			return true;
		}
		return false;
	}

	bool DragAngle(const std::string& tag, DirectX::XMFLOAT3& radians, float resetVal, float min, float max, float speed, const char* fmt, ImGuiSliderFlags flags)
	{
		auto degrees = Math::ToDegrees(radians);
		if (DragFloat(tag, degrees, resetVal, speed, min, max, fmt, flags))
		{
			radians = Math::ToRadians(degrees);
			return true;
		}
		return false;
	}

	bool DragAngle(const std::string& tag, DirectX::XMFLOAT4& radians, float resetVal, float min, float max, float speed, const char* fmt, ImGuiSliderFlags flags)
	{
		auto degrees = Math::ToDegrees(radians);
		if (DragFloat(tag, degrees, resetVal, speed, min, max, fmt, flags))
		{
			radians = Math::ToRadians(degrees);
			return true;
		}
		return false;
	}


	bool Checkbox(const std::string& tag, bool& v)
	{
		// ImGui's Table API currently crashes when BeginTable return false but Talbe functions get called
		if (!ImGui::GetCurrentTable())
			return false;

		ImGui::TableNextRow();

		PushID(GetID(tag));

		ImGui::TableSetColumnIndex(0);
		Text(tag);
		ImGui::TableSetColumnIndex(1);

		const bool changed = ImGui::Checkbox("##X", &v);

		PopID();

		return changed;
	}

	bool ButtonRightAligned(const std::string& tag, const ImVec2& size, ImGuiButtonFlags flags)
	{
		ImGuiContext* ctx = GImGui;
		ImGuiWindow* window = ctx->CurrentWindow;
		if (window->SkipItems)
			return false;

		ImGuiStyle& style = ImGui::GetStyle();

		ImVec2 textSize = ImGui::CalcTextSize(tag.c_str(), NULL, true);
		ImVec2 itemSize = ImGui::CalcItemSize(size, textSize.x + style.FramePadding.x * 2.0f, textSize.y + style.FramePadding.y * 2.0f);
		window->DC.CursorPos.x += ImGui::GetContentRegionAvail().x - itemSize.x;
		return ImGui::ButtonEx(tag.c_str(), itemSize, flags);
	}

}

