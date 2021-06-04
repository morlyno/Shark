#include "skpch.h"
#include "ImGuiUtils.h"

#include <imgui_internal.h>
#include "Shark/Render/RendererCommand.h"

#include "Shark/Utility/PlatformUtils.h"

namespace Shark::UI {

	void DrawFloatShow(const char* lable, float val, const char* fmt, float textWidth, const char* buttoncharacter)
	{
		ImGui::PushID(lable);

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, textWidth);

		ImGui::AlignTextToFramePadding();
		ImGui::Text(lable);
		ImGui::NextColumn();

		auto& style = ImGui::GetStyle();
		const float buttonHeight = ImGui::GetFontSize() + style.FramePadding.y * 2.0f;
		const ImVec2 buttonSize = { buttonHeight, buttonHeight };
		const float itemWidth = ImGui::GetColumnWidth() - buttonSize.x - style.FramePadding.x * 2.0f;
		constexpr ImGuiSliderFlags sliderFalgs = ImGuiSliderFlags_ReadOnly | ImGuiSliderFlags_NoInput;

		ImGui::PushItemWidth(itemWidth);

		ImGui::Button(buttoncharacter, buttonSize);
		ImGui::SameLine(0.0f, 0.0f);
		ImGui::DragFloat("##X", &val, 1.0f, 0.0f, 0.0f, fmt, sliderFalgs);

		ImGui::PopItemWidth();

		ImGui::Columns();

		ImGui::PopID();
	}

	void DrawVec2Show(const char* lable, DirectX::XMFLOAT2 vec, const char* fmt, float textWidth, const char* buttoncharacters)
	{
		ImGui::PushID(lable);

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, textWidth);

		ImGui::AlignTextToFramePadding();
		ImGui::Text(lable);
		ImGui::NextColumn();

		auto& style = ImGui::GetStyle();
		const float buttonHeight = ImGui::GetFontSize() + style.FramePadding.y * 2.0f;
		const ImVec2 buttonSize = { buttonHeight, buttonHeight };
		const float itemWidth = ImGui::GetColumnWidth() / 2.0f - buttonSize.x - style.FramePadding.x * 2.0f;
		constexpr ImGuiSliderFlags sliderFalgs = ImGuiSliderFlags_ReadOnly | ImGuiSliderFlags_NoInput;

		ImGui::PushItemWidth(itemWidth);

		ImGui::Button(&buttoncharacters[0], buttonSize);
		ImGui::SameLine(0.0f, 0.0f);
		ImGui::DragFloat("##X", &vec.x, 1.0f, 0.0f, 0.0f, fmt, sliderFalgs);

		ImGui::SameLine();
		ImGui::Button(&buttoncharacters[2], buttonSize);
		ImGui::SameLine(0.0f, 0.0f);
		ImGui::DragFloat("##Y", &vec.y, 1.0f, 0.0f, 0.0f, fmt, sliderFalgs);

		ImGui::PopItemWidth();

		ImGui::Columns();

		ImGui::PopID();
	}

	void DrawVec3Show(const char* lable, DirectX::XMFLOAT3 vec, const char* fmt, float textWidth, const char* buttoncharacters)
	{
		ImGui::PushID(lable);

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, textWidth);

		ImGui::AlignTextToFramePadding();
		ImGui::Text(lable);
		ImGui::NextColumn();

		auto& style = ImGui::GetStyle();
		const float buttonHeight = ImGui::GetFontSize() + style.FramePadding.y * 2.0f;
		const ImVec2 buttonSize = { buttonHeight, buttonHeight };
		const float itemWidth = ImGui::GetColumnWidth() / 3.0f - buttonSize.x - style.FramePadding.x * 2.0f;
		constexpr ImGuiSliderFlags sliderFalgs = ImGuiSliderFlags_ReadOnly | ImGuiSliderFlags_NoInput;

		ImGui::PushItemWidth(itemWidth);

		ImGui::Button(&buttoncharacters[0], buttonSize);
		ImGui::SameLine(0.0f, 0.0f);
		ImGui::DragFloat("##X", &vec.x, 1.0f, 0.0f, 0.0f, fmt, sliderFalgs);

		ImGui::SameLine();
		ImGui::Button(&buttoncharacters[2], buttonSize);
		ImGui::SameLine(0.0f, 0.0f);
		ImGui::DragFloat("##Y", &vec.y, 1.0f, 0.0f, 0.0f, fmt, sliderFalgs);

		ImGui::SameLine();
		ImGui::Button(&buttoncharacters[4], buttonSize);
		ImGui::SameLine(0.0f, 0.0f);
		ImGui::DragFloat("##Z", &vec.z, 1.0f, 0.0f, 0.0f, fmt, sliderFalgs);

		ImGui::PopItemWidth();

		ImGui::Columns();

		ImGui::PopID();
	}

	bool DrawFloatControl(const char* lable, float& val, float resetVal, const char* fmt, float textWidth, const char* buttoncharacter)
	{
		ImGui::PushID(lable);

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, textWidth);

		ImGui::AlignTextToFramePadding();
		ImGui::Text(lable);
		ImGui::NextColumn();

		auto& style = ImGui::GetStyle();
		const float buttonHeight = ImGui::GetFontSize() + style.FramePadding.y * 2.0f;
		const ImVec2 buttonSize = { buttonHeight, buttonHeight };
		const float itemWidth = ImGui::GetColumnWidth() - buttonSize.x - style.FramePadding.x * 2.0f;
		constexpr ImGuiSliderFlags sliderFalgs = ImGuiSliderFlags_NoRoundToFormat;

		bool valchanged = false;

		ImGui::PushItemWidth(itemWidth);

		if (ImGui::Button(buttoncharacter, buttonSize))
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

	bool DrawVec2Control(const char* lable, DirectX::XMFLOAT2& vec, float resetVal, const char* fmt, float textWidth, const char* buttoncharacters)
	{
		ImGui::PushID(lable);

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, textWidth);

		ImGui::AlignTextToFramePadding();
		ImGui::Text(lable);
		ImGui::NextColumn();

		auto& style = ImGui::GetStyle();
		const float buttonHeight = ImGui::GetFontSize() + style.FramePadding.y * 2.0f;
		const ImVec2 buttonSize = { buttonHeight, buttonHeight };
		const float itemWidth = ImGui::GetColumnWidth() / 2.0f - buttonSize.x - style.FramePadding.x * 2.0f;
		constexpr ImGuiSliderFlags sliderFalgs = ImGuiSliderFlags_NoRoundToFormat;

		bool valchanged = false;

		ImGui::PushItemWidth(itemWidth);

		if (ImGui::Button(&buttoncharacters[0], buttonSize))
		{
			vec.x = resetVal;
			valchanged = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		valchanged |= ImGui::DragFloat("##X", &vec.x, 1.0f, 0.0f, 0.0f, fmt, sliderFalgs);

		ImGui::SameLine();
		if (ImGui::Button(&buttoncharacters[2], buttonSize))
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

	bool DrawVec3Control(const char* lable, DirectX::XMFLOAT3& vec, float resetVal, const char* fmt, float textWidth, const char* buttoncharacters)
	{
		ImGui::PushID(lable);

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, textWidth);

		ImGui::AlignTextToFramePadding();
		ImGui::Text(lable);
		ImGui::NextColumn();

		auto& style = ImGui::GetStyle();
		const float buttonHeight = ImGui::GetFontSize() + style.FramePadding.y * 2.0f;
		const ImVec2 buttonSize = { buttonHeight, buttonHeight };
		const float itemWidth = ImGui::GetColumnWidth() / 3.0f - buttonSize.x - style.FramePadding.x * 2.0f;
		constexpr ImGuiSliderFlags sliderFalgs = ImGuiSliderFlags_NoRoundToFormat;

		bool valchanged = false;

		ImGui::PushItemWidth(itemWidth);

		if (ImGui::Button(&buttoncharacters[0], buttonSize))
		{
			vec.x = resetVal;
			valchanged = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		valchanged |= ImGui::DragFloat("##X", &vec.x, 1.0f, 0.0f, 0.0f, fmt, sliderFalgs);

		ImGui::SameLine();
		if (ImGui::Button(&buttoncharacters[2], buttonSize))
		{
			vec.y = resetVal;
			valchanged = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		valchanged |= ImGui::DragFloat("##Y", &vec.y, 1.0f, 0.0f, 0.0f, fmt, sliderFalgs);

		ImGui::SameLine();
		if (ImGui::Button(&buttoncharacters[4], buttonSize))
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

	bool DrawFloatXControl(const char* lable, float* data, uint32_t count, float resetVal, const char* fmt, float textWidth)
	{
		switch (count)
		{
			case 1:
			{
				float& val = *data;
				return DrawFloatControl(lable, val, resetVal, fmt, textWidth);
			}
			case 2:
			{
				DirectX::XMFLOAT2& val = *(DirectX::XMFLOAT2*)data;
				return DrawVec2Control(lable, val, resetVal, fmt, textWidth);
			}
			case 3:
			{
				DirectX::XMFLOAT3& val = *(DirectX::XMFLOAT3*)data;
				return DrawVec3Control(lable, val, resetVal, fmt, textWidth);
			}
			case 4:
			{
				return ImGui::ColorEdit4("lable", data);
			}
		}
		SK_CORE_ASSERT(false);
		return false;
	}

	static void ImGuiCallbackFunctionBlend(const ImDrawList* parent_list, const ImDrawCmd* cmd)
	{
		RendererCommand::MainFrameBufferSetBlend((bool)cmd->UserCallbackData);
	}

	void NoAlpaImage(const Ref<FrameBuffer>& framebuffer, ImTextureID textureID, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tintcolor, const ImVec4& bordercolor)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return;

		window->DrawList->AddCallback(ImGuiCallbackFunctionBlend, (void*)false);
		ImGui::Image(textureID, size, uv0, uv1, tintcolor, bordercolor);
		window->DrawList->AddCallback(ImGuiCallbackFunctionBlend, (void*)true);

	}

	bool SelectTextureImageButton(Ref<Texture2D>& texture, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, float framepadding, const ImVec4& bgColor, const ImVec4& tintcolor)
	{
		bool changed = false;

		RenderID textureID = texture ? texture->GetRenderID() : nullptr;
		if (ImGui::ImageButton(textureID, size, uv0, uv1, framepadding, bgColor, tintcolor))
		{
			auto path = FileDialogs::OpenFile("Texture (*.*)\0*.*\0");
			if (!path.empty())
			{
				texture = Texture2D::Create(path);
				changed = true;
			}
		}

		if (ImGui::BeginDragDropTarget())
		{
			const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(AssetPayload::ID);
			if (payload)
			{
				auto* data = (AssetPayload*)payload->Data;
				if (data->Type == AssetType::Texture)
				{
					texture = Texture2D::Create(data->FilePath);
					changed = true;
				}
			}
			ImGui::EndDragDropTarget();
		}

		return changed;
	}

}
