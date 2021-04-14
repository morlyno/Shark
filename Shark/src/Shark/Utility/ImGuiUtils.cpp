#include "skpch.h"
#include "ImGuiUtils.h"

#include <imgui_internal.h>
#include "Shark/Render/RendererCommand.h"

namespace Shark::UI {

	void DrawFloatShow(const char* lable, float val, const char* fmt, float textWidth, const std::string& buttoncharacter)
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

		ImGui::Button(buttoncharacter.c_str(), buttonSize);
		ImGui::SameLine(0.0f, 0.0f);
		ImGui::DragFloat("##X", &val, 1.0f, 0.0f, 0.0f, fmt, sliderFalgs);

		ImGui::PopItemWidth();

		ImGui::Columns();

		ImGui::PopID();
	}

	void DrawVec2Show(const char* lable, DirectX::XMFLOAT2 vec, const char* fmt, float textWidth, const std::string& buttoncharacters)
	{
		ImGui::PushID(lable);

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, textWidth);

		ImGui::Text(lable);
		ImGui::NextColumn();

		const ImVec2 buttonSize = { 19, 19 };
		const float itemWidth = ImGui::GetColumnWidth() / 2.0f - buttonSize.x - 8.0f;
		constexpr ImGuiSliderFlags sliderFalgs = ImGuiSliderFlags_ReadOnly | ImGuiSliderFlags_NoInput;
		auto splitstr = Utils::StringSplit(buttoncharacters);

		ImGui::PushItemWidth(itemWidth);

		ImGui::Button(splitstr[0].c_str(), buttonSize);
		ImGui::SameLine(0.0f, 0.0f);
		ImGui::DragFloat("##X", &vec.x, 1.0f, 0.0f, 0.0f, fmt, sliderFalgs);

		ImGui::SameLine();
		ImGui::Button(splitstr[1].c_str(), buttonSize);
		ImGui::SameLine(0.0f, 0.0f);
		ImGui::DragFloat("##Y", &vec.y, 1.0f, 0.0f, 0.0f, fmt, sliderFalgs);

		ImGui::PopItemWidth();

		ImGui::Columns();

		ImGui::PopID();
	}

	void DrawVec3Show(const char* lable, DirectX::XMFLOAT3 vec, const char* fmt, float textWidth, const std::string& buttoncharacters)
	{
		ImGui::PushID(lable);

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, textWidth);

		ImGui::Text(lable);
		ImGui::NextColumn();

		const ImVec2 buttonSize = { 19, 19 };
		const float itemWidth = ImGui::GetColumnWidth() / 3.0f - buttonSize.x - 8.0f;
		constexpr ImGuiSliderFlags sliderFalgs = ImGuiSliderFlags_ReadOnly | ImGuiSliderFlags_NoInput;
		auto splitstr = Utils::StringSplit(buttoncharacters);

		ImGui::PushItemWidth(itemWidth);

		ImGui::Button(splitstr[0].c_str(), buttonSize);
		ImGui::SameLine(0.0f, 0.0f);
		ImGui::DragFloat("##X", &vec.x, 1.0f, 0.0f, 0.0f, fmt, sliderFalgs);

		ImGui::SameLine();
		ImGui::Button(splitstr[1].c_str(), buttonSize);
		ImGui::SameLine(0.0f, 0.0f);
		ImGui::DragFloat("##Y", &vec.y, 1.0f, 0.0f, 0.0f, fmt, sliderFalgs);

		ImGui::SameLine();
		ImGui::Button(splitstr[2].c_str(), buttonSize);
		ImGui::SameLine(0.0f, 0.0f);
		ImGui::DragFloat("##Z", &vec.z, 1.0f, 0.0f, 0.0f, fmt, sliderFalgs);

		ImGui::PopItemWidth();

		ImGui::Columns();

		ImGui::PopID();
	}

	bool DrawFloatControl(const char* lable, float& val, float resetVal, const char* fmt, float textWidth, const std::string& buttoncharacter)
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

		if (ImGui::Button(buttoncharacter.c_str(), buttonSize))
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

	bool DrawVec2Control(const char* lable, DirectX::XMFLOAT2& vec, float resetVal, const char* fmt, float textWidth, const std::string& buttoncharacters)
	{
		ImGui::PushID(lable);

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, textWidth);

		ImGui::Text(lable);
		ImGui::NextColumn();

		const ImVec2 buttonSize = { 19, 19 };
		const float itemWidth = ImGui::GetColumnWidth() / 2.0f - buttonSize.x - 8.0f;
		constexpr ImGuiSliderFlags sliderFalgs = ImGuiSliderFlags_NoRoundToFormat;
		auto splitstr = Utils::StringSplit(buttoncharacters);

		bool valchanged = false;

		ImGui::PushItemWidth(itemWidth);

		if (ImGui::Button(splitstr[0].c_str(), buttonSize))
		{
			vec.x = resetVal;
			valchanged = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		valchanged |= ImGui::DragFloat("##X", &vec.x, 1.0f, 0.0f, 0.0f, fmt, sliderFalgs);

		ImGui::SameLine();
		if (ImGui::Button(splitstr[1].c_str(), buttonSize))
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

	bool DrawVec3Control(const char* lable, DirectX::XMFLOAT3& vec, float resetVal, const char* fmt, float textWidth, const std::string& buttoncharacters)
	{
		ImGui::PushID(lable);

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, textWidth);

		ImGui::Text(lable);
		ImGui::NextColumn();

		const ImVec2 buttonSize = { 19, 19 };
		const float itemWidth = ImGui::GetColumnWidth() / 3.0f - buttonSize.x - 8.0f;
		constexpr ImGuiSliderFlags sliderFalgs = ImGuiSliderFlags_NoRoundToFormat;
		auto splitstr = Utils::StringSplit(buttoncharacters);

		bool valchanged = false;

		ImGui::PushItemWidth(itemWidth);

		if (ImGui::Button(splitstr[0].c_str(), buttonSize))
		{
			vec.x = resetVal;
			valchanged = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		valchanged |= ImGui::DragFloat("##X", &vec.x, 1.0f, 0.0f, 0.0f, fmt, sliderFalgs);

		ImGui::SameLine();
		if (ImGui::Button(splitstr[1].c_str(), buttonSize))
		{
			vec.y = resetVal;
			valchanged = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		valchanged |= ImGui::DragFloat("##Y", &vec.y, 1.0f, 0.0f, 0.0f, fmt, sliderFalgs);

		ImGui::SameLine();
		if (ImGui::Button(splitstr[2].c_str(), buttonSize))
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

	static void ImGuiCallbackFunctionBlendOff(const ImDrawList* parent_list, const ImDrawCmd* cmd)
	{
		(*(Ref<FrameBuffer>*)cmd->UserCallbackData)->SetBlend(0, false);
	}

	static void ImGUiCallbackFunctionBlendOn(const ImDrawList* parent_list, const ImDrawCmd* cmd)
	{
		(*(Ref<FrameBuffer>*)cmd->UserCallbackData)->SetBlend(0, true);
	}

	void NoAlpaImage(const Ref<FrameBuffer>& framebuffer, ImTextureID textureID, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tintcolor, const ImVec4& bordercolor)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return;

		window->DrawList->AddCallback(ImGuiCallbackFunctionBlendOff, (void*)&framebuffer);
		ImGui::Image(textureID, size, uv0, uv1, tintcolor, bordercolor);
		window->DrawList->AddCallback(ImGUiCallbackFunctionBlendOn, (void*)&framebuffer);

	}

}
