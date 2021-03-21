#include "skpch.h"
#include "ImGuiUtils.h"

#include <imgui_internal.h>
#include "Shark/Render/RendererCommand.h"

namespace Shark::UI {

	void DrawFloatShow(const char* lable, float val, const char* fmt, float textWidth)
	{
		ImGui::PushID(lable);

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, textWidth);

		ImGui::Text(lable);
		ImGui::NextColumn();

		const ImVec2 buttonSize = { 19, 19 };
		const float itemWidth = ImGui::GetColumnWidth() - buttonSize.x - 8.0f;
		constexpr ImGuiSliderFlags sliderFalgs = ImGuiSliderFlags_ReadOnly | ImGuiSliderFlags_NoInput;

		ImGui::PushItemWidth(itemWidth);

		ImGui::Button("X", buttonSize);
		ImGui::SameLine(0.0f, 0.0f);
		ImGui::DragFloat("##X", &val, 1.0f, 0.0f, 0.0f, fmt, sliderFalgs);

		ImGui::PopItemWidth();

		ImGui::Columns();

		ImGui::PopID();
	}

	void DrawVec2Show(const char* lable, DirectX::XMFLOAT2 vec, const char* fmt, float textWidth)
	{
		ImGui::PushID(lable);

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, textWidth);

		ImGui::Text(lable);
		ImGui::NextColumn();

		const ImVec2 buttonSize = { 19, 19 };
		const float itemWidth = ImGui::GetColumnWidth() / 2.0f - buttonSize.x - 8.0f;
		constexpr ImGuiSliderFlags sliderFalgs = ImGuiSliderFlags_ReadOnly | ImGuiSliderFlags_NoInput;

		ImGui::PushItemWidth(itemWidth);

		ImGui::Button("X", buttonSize);
		ImGui::SameLine(0.0f, 0.0f);
		ImGui::DragFloat("##X", &vec.x, 1.0f, 0.0f, 0.0f, fmt, sliderFalgs);

		ImGui::SameLine();
		ImGui::Button("Y", buttonSize);
		ImGui::SameLine(0.0f, 0.0f);
		ImGui::DragFloat("##Y", &vec.y, 1.0f, 0.0f, 0.0f, fmt, sliderFalgs);

		ImGui::PopItemWidth();

		ImGui::Columns();

		ImGui::PopID();
	}

	void DrawVec3Show(const char* lable, DirectX::XMFLOAT3 vec, const char* fmt, float textWidth)
	{
		ImGui::PushID(lable);

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, textWidth);

		ImGui::Text(lable);
		ImGui::NextColumn();

		const ImVec2 buttonSize = { 19, 19 };
		const float itemWidth = ImGui::GetColumnWidth() / 3.0f - buttonSize.x - 8.0f;
		constexpr ImGuiSliderFlags sliderFalgs = ImGuiSliderFlags_ReadOnly | ImGuiSliderFlags_NoInput;

		ImGui::PushItemWidth(itemWidth);

		ImGui::Button("X", buttonSize);
		ImGui::SameLine(0.0f, 0.0f);
		ImGui::DragFloat("##X", &vec.x, 1.0f, 0.0f, 0.0f, fmt, sliderFalgs);

		ImGui::SameLine();
		ImGui::Button("Y", buttonSize);
		ImGui::SameLine(0.0f, 0.0f);
		ImGui::DragFloat("##Y", &vec.y, 1.0f, 0.0f, 0.0f, fmt, sliderFalgs);

		ImGui::SameLine();
		ImGui::Button("Z", buttonSize);
		ImGui::SameLine(0.0f, 0.0f);
		ImGui::DragFloat("##Z", &vec.z, 1.0f, 0.0f, 0.0f, fmt, sliderFalgs);

		ImGui::PopItemWidth();

		ImGui::Columns();

		ImGui::PopID();
	}

	bool DrawFloatControl(const char* lable, float& val, float resetVal, const char* fmt, float textWidth)
	{
		ImGui::PushID(lable);

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, textWidth);

		ImGui::Text(lable);
		ImGui::NextColumn();

		const ImVec2 buttonSize = { 19, 19 };
		const float itemWidth = ImGui::GetColumnWidth() - buttonSize.x - 8.0f;
		constexpr ImGuiSliderFlags sliderFalgs = ImGuiSliderFlags_NoRoundToFormat;

		bool valchanged = false;

		ImGui::PushItemWidth(itemWidth);

		if (ImGui::Button("X", buttonSize))
		{
			val = resetVal;
			valchanged = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		valchanged |= ImGui::DragFloat("##X", &val, 1.0f, 0.0f, 0.0f, fmt, sliderFalgs);

		ImGui::PopItemWidth();

		ImGui::Columns();

		ImGui::PopID();

		return valchanged;
	}

	bool DrawVec2Control(const char* lable, DirectX::XMFLOAT2& vec, float resetVal, const char* fmt, float textWidth)
	{
		ImGui::PushID(lable);

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, textWidth);

		ImGui::Text(lable);
		ImGui::NextColumn();

		const ImVec2 buttonSize = { 19, 19 };
		const float itemWidth = ImGui::GetColumnWidth() / 2.0f - buttonSize.x - 8.0f;
		constexpr ImGuiSliderFlags sliderFalgs = ImGuiSliderFlags_NoRoundToFormat;

		bool valchanged = false;

		ImGui::PushItemWidth(itemWidth);

		if (ImGui::Button("X", buttonSize))
		{
			vec.x = resetVal;
			valchanged = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		valchanged |= ImGui::DragFloat("##X", &vec.x, 1.0f, 0.0f, 0.0f, fmt, sliderFalgs);

		ImGui::SameLine();
		if (ImGui::Button("Y", buttonSize))
		{
			vec.y = resetVal;
			valchanged = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		valchanged |= ImGui::DragFloat("##Y", &vec.y, 1.0f, 0.0f, 0.0f, fmt, sliderFalgs);

		ImGui::PopItemWidth();

		ImGui::Columns();

		ImGui::PopID();

		return valchanged;
	}

	bool DrawVec3Control(const char* lable, DirectX::XMFLOAT3& vec, float resetVal, const char* fmt, float textWidth)
	{
		ImGui::PushID(lable);

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, textWidth);

		ImGui::Text(lable);
		ImGui::NextColumn();

		const ImVec2 buttonSize = { 19, 19 };
		const float itemWidth = ImGui::GetColumnWidth() / 3.0f - buttonSize.x - 8.0f;
		constexpr ImGuiSliderFlags sliderFalgs = ImGuiSliderFlags_NoRoundToFormat;

		bool valchanged = false;

		ImGui::PushItemWidth(itemWidth);

		if (ImGui::Button("X", buttonSize))
		{
			vec.x = resetVal;
			valchanged = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		valchanged |= ImGui::DragFloat("##X", &vec.x, 1.0f, 0.0f, 0.0f, fmt, sliderFalgs);

		ImGui::SameLine();
		if (ImGui::Button("Y", buttonSize))
		{
			vec.y = resetVal;
			valchanged = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		valchanged |= ImGui::DragFloat("##Y", &vec.y, 1.0f, 0.0f, 0.0f, fmt, sliderFalgs);

		ImGui::SameLine();
		if (ImGui::Button("Z", buttonSize))
		{
			vec.z = resetVal;
			valchanged = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		valchanged |= ImGui::DragFloat("##Z", &vec.z, 1.0f, 0.0f, 0.0f, fmt, sliderFalgs);

		ImGui::PopItemWidth();

		ImGui::Columns();

		ImGui::PopID();

		return valchanged;
	}

	static void ImGuiCallbackFunctionBlend(const ImDrawList* parent_list, const ImDrawCmd* cmd)
	{
		::Shark::RendererCommand::SetBlendState((bool)cmd->UserCallbackData);
	}

	void NoAlpaImage(ImTextureID textureID, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tintcolor, const ImVec4& bordercolor)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return;

		window->DrawList->AddCallback(ImGuiCallbackFunctionBlend, (void*)0);
		ImGui::Image(textureID, size, uv0, uv1, tintcolor, bordercolor);
		window->DrawList->AddCallback(ImGuiCallbackFunctionBlend, (void*)1);

	}

}
