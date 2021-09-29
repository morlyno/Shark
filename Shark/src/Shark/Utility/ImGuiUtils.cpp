#include "skpch.h"
#include "ImGuiUtils.h"

#include <imgui_internal.h>
#include "Shark/Render/RendererCommand.h"

#include "Shark/Utility/PlatformUtils.h"

#include <misc/cpp/imgui_stdlib.h>

namespace Shark::Utility {

	ImVec4 ToImVec4(const DirectX::XMFLOAT4& color)
	{
		return ImVec4{ color.x, color.y, color.z, color.w };
	}

}

namespace Shark::UI {

	static ImVec4 ImVec4Max(const ImVec4& a, const ImVec4& b)
	{
		return ImVec4(
			a.x > b.x ? a.x : b.x,
			a.y > b.y ? a.y : b.y,
			a.z > b.z ? a.z : b.z,
			a.w > b.w ? a.w : b.w
		);
	}

	ImGuiID GetID(const std::string& str)
	{
		return ImGui::GetID(str.c_str());
	}

	void DrawFloatShow(const std::string& label, float val, const char* fmt, float textWidth, const char* buttoncharacter)
	{
		ImGui::PushID(label.c_str());

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, textWidth);

		ImGui::AlignTextToFramePadding();
		ImGui::Text(label.c_str());
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

	void DrawVec2Show(const std::string& label, DirectX::XMFLOAT2 vec, const char* fmt, float textWidth, const char* buttoncharacters)
	{
		ImGui::PushID(label.c_str());

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, textWidth);

		ImGui::AlignTextToFramePadding();
		ImGui::Text(label.c_str());
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

	void DrawVec3Show(const std::string& label, DirectX::XMFLOAT3 vec, const char* fmt, float textWidth, const char* buttoncharacters)
	{
		ImGui::PushID(label.c_str());

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, textWidth);

		ImGui::AlignTextToFramePadding();
		ImGui::Text(label.c_str());
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

	bool DrawFloatControl(const std::string& label, float& val, float resetVal, const char* fmt, float textWidth, const char* buttoncharacter)
	{
		ImGui::PushID(label.c_str());

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, textWidth);

		ImGui::AlignTextToFramePadding();
		ImGui::Text(label.c_str());
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

	bool DrawVec2Control(const std::string& label, DirectX::XMFLOAT2& vec, float resetVal, const char* fmt, float textWidth, const char* buttoncharacters)
	{
		ImGui::PushID(label.c_str());

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, textWidth);

		ImGui::AlignTextToFramePadding();
		ImGui::Text(label.c_str());
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

	bool DrawVec3Control(const std::string& label, DirectX::XMFLOAT3& vec, float resetVal, const char* fmt, float textWidth, const char* buttoncharacters)
	{
		ImGui::PushID(label.c_str());

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, textWidth);

		ImGui::AlignTextToFramePadding();
		ImGui::Text(label.c_str());
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

	bool DrawFloatXControl(const std::string& label, float* data, uint32_t count, float resetVal, const char* fmt, float textWidth)
	{
		switch (count)
		{
		case 1:
		{
			float& val = *data;
			return DrawFloatControl(label, val, resetVal, fmt, textWidth);
		}
		case 2:
		{
			DirectX::XMFLOAT2& val = *(DirectX::XMFLOAT2*)data;
			return DrawVec2Control(label, val, resetVal, fmt, textWidth);
		}
		case 3:
		{
			DirectX::XMFLOAT3& val = *(DirectX::XMFLOAT3*)data;
			return DrawVec3Control(label, val, resetVal, fmt, textWidth);
		}
		case 4:
		{
			return ImGui::ColorEdit4("label", data);
		}
		}
		SK_CORE_ASSERT(false);
		return false;
	}

	static void ImGuiCallbackFunctionBlend(const ImDrawList* parent_list, const ImDrawCmd* cmd)
	{
		RendererCommand::MainFrameBufferSetBlend((bool)cmd->UserCallbackData);
	}

	void NoAlpaImage(ImTextureID textureID, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tintcolor, const ImVec4& bordercolor)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return;

		window->DrawList->AddCallback(ImGuiCallbackFunctionBlend, (void*)false);
		ImGui::Image(textureID, size, uv0, uv1, tintcolor, bordercolor);
		window->DrawList->AddCallback(ImGuiCallbackFunctionBlend, (void*)true);

	}

	bool SelectTextureImageButton(Ref<Texture2D>& texture, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, int framepadding, const ImVec4& bgColor, const ImVec4& tintcolor)
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
			const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(UI::ContentPayload::ID);
			if (payload)
			{
				auto* data = (UI::ContentPayload*)payload->Data;
				if (data->Type == UI::ContentType::Texture)
				{
					texture = Texture2D::Create(data->Path);
					changed = true;
				}
			}
			ImGui::EndDragDropTarget();
		}

		return changed;
	}

	bool ImageButton(const std::string& strID, ImTextureID textureID, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, int frame_padding, const ImVec4& bg_col, const ImVec4& tint_col)
	{
		ImGuiID id = ImGui::GetID(strID.c_str());
		return ImageButton(id, textureID, size, uv0, uv1, frame_padding, bg_col, tint_col);
	}

	bool ImageButton(ImGuiID id, ImTextureID textureID, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, int frame_padding, const ImVec4& bg_col, const ImVec4& tint_col)
	{
		ImGuiContext& g = *ImGui::GetCurrentContext();
		ImGuiWindow* window = g.CurrentWindow;
		if (window->SkipItems)
			return false;

		const ImVec2 padding = (frame_padding >= 0) ? ImVec2((float)frame_padding, (float)frame_padding) : g.Style.FramePadding;
		return ImGui::ImageButtonEx(id, textureID, size, uv0, uv1, padding, bg_col, tint_col);
	}

	void Image(ImTextureID textureID, const ImVec2& size, const ImVec2& uv0 /*= { 0, 0 }*/, const ImVec2& uv1 /*= { 1, 1 }*/, int frame_padding /*= -1*/, const ImVec4& bg_col /*= { 0, 0, 0, 0 }*/, const ImVec4& tint_col /*= { 1, 1, 1, 1 }*/)
	{
		ImGui::Image(textureID, size, uv0, uv1, tint_col, bg_col);
	}

	ImVec2 CalcItemSize(const std::string& label)
	{
		const ImGuiStyle& style = ImGui::GetStyle();

		const ImVec2 label_size = ImGui::CalcTextSize(label.c_str(), NULL, true);
		return ImGui::CalcItemSize({ 0, 0 }, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);
	}

	ImVec2 GetFramePadding()
	{
		const ImGuiStyle& style = ImGui::GetStyle();
		return style.FramePadding;
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

		ImGui::CollapsingHeader(text.c_str(), ImGuiTreeNodeFlags_Leaf);

		ImGui::PopStyleColor(3);

		ImGui::IsItemClicked();
	}

	void TextWithBackGround(const std::filesystem::path& filePath)
	{
		TextWithBackGround(filePath, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
	}

	void TextWithBackGround(const std::filesystem::path& filePath, const ImVec4& bgColor)
	{
		TextWithBackGround(filePath.string(), bgColor);
	}

	bool InputText(const char* label, std::string& str, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
	{
		auto& g = *GImGui;
		if (!(g.NextItemData.Flags & ImGuiNextItemDataFlags_HasWidth))
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		return ImGui::InputText(label, &str, flags, callback, user_data);
	}

	bool InputText(const char* label, std::filesystem::path& path, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
	{
		auto&& str = path.string();
		bool retval = InputText(label, str, flags, callback, user_data);
		path = std::move(str);
		return retval;

	}

	void Text(const char* str)
	{
		ImGui::Text(str);
	}

	void Text(const std::string& str)
	{
		Text(str.c_str());
	}

	void Text(const std::filesystem::path& path)
	{
		Text(path.string());
	}

	void MoveCurserPos(ImVec2 delta)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		window->DC.CursorPos += delta;
	}

	void MoveCurserPosX(float deltaX)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		window->DC.CursorPos.x += deltaX;

	}

	void MoveCurserPosY(float deltaY)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		window->DC.CursorPos.y += deltaY;
	}

	bool ButtonRightAligned(const std::string& label, const ImVec2& size, ImGuiButtonFlags flags)
	{
		ImVec2 itemSize;
		itemSize = GetItemSize(label);
		if (size.x) itemSize.x = size.x;
		if (size.y) itemSize.y = size.y;
		ImGui::SameLine();
		MoveCurserPosX(ImGui::GetContentRegionAvail().x - itemSize.x);
		return ImGui::ButtonEx(label.c_str(), itemSize, flags);
	}
	
	bool BeginPopup(const std::string& label, ImGuiWindowFlags flags)
	{
		return BeginPopup(ImGui::GetID(label.c_str()), flags);
	}

	bool BeginPopup(ImGuiID id, ImGuiWindowFlags flags)
	{
		ImGuiContext& g = *GImGui;
		if (g.OpenPopupStack.Size <= g.BeginPopupStack.Size) // Early out for performance
		{
			g.NextWindowData.ClearFlags(); // We behave like Begin() and need to consume those values
			return false;
		}
		flags |= ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings;
		return ImGui::BeginPopupEx(id, flags);
	}

	bool GetContentPayload(std::string& out_Path, ContentType type)
	{
		bool accepted = false;
		if (ImGui::BeginDragDropTarget())
		{
			const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(ContentPayload::ID);
			if (payload)
			{
				auto* content = (ContentPayload*)payload->Data;
				if (content->Type == type)
				{
					out_Path = content->Path;
					accepted = true;
				}
			}
			ImGui::EndDragDropTarget();
		}
		return accepted;
	}

	bool GetContentPayload(std::filesystem::path& out_Path, ContentType type)
	{
		bool accepted = false;
		if (ImGui::BeginDragDropTarget())
		{
			const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(ContentPayload::ID);
			if (payload)
			{
				auto* content = (ContentPayload*)payload->Data;
				if (content->Type == type)
				{
					out_Path = content->Path;
					accepted = true;
				}
			}
			ImGui::EndDragDropTarget();
		}
		return accepted;
	}

	const ImVec4& GetColor(ImGuiCol color)
	{
		return ImGui::GetStyleColorVec4(color);
	}

	bool Button(ImGuiID id, const std::string& label_arg, const ImVec2& size_arg, bool active, ImGuiButtonFlags flags)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;

		const char* label = label_arg.c_str();
		const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

		ImVec2 pos = window->DC.CursorPos;
		if ((flags & ImGuiButtonFlags_AlignTextBaseLine) && style.FramePadding.y < window->DC.CurrLineTextBaseOffset) // Try to vertically align buttons that are smaller/have no padding so that text baseline matches (bit hacky, since it shouldn't be a flag)
			pos.y += window->DC.CurrLineTextBaseOffset - style.FramePadding.y;
		ImVec2 size = ImGui::CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

		const ImRect bb(pos, pos + size);
		ImGui::ItemSize(size, style.FramePadding.y);
		if (!ImGui::ItemAdd(bb, id))
			return false;

		if (g.CurrentItemFlags & ImGuiItemFlags_ButtonRepeat)
			flags |= ImGuiButtonFlags_Repeat;
		bool hovered, held;
		bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, flags);

		// Render
		ImVec4 c = style.Colors[active ? (held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button : ImGuiCol_Button];
		c.w *= style.Alpha;
		if (!active)
			c = ImVec4Max(c - ImVec4(0.1f, 0.1f, 0.1f, 0.0f), ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

		const ImU32 col = ImGui::ColorConvertFloat4ToU32(c);
		//const ImU32 col = ImGui::GetColorU32(active ? (held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button : ImGuiCol_Button);
		ImGui::RenderNavHighlight(bb, id);
		ImGui::RenderFrame(bb.Min, bb.Max, col, true, style.FrameRounding);

		if (g.LogEnabled)
			ImGui::LogSetNextTextDecoration("[", "]");

		if (active)
		{
			ImGui::RenderTextClipped(bb.Min + style.FramePadding, bb.Max - style.FramePadding, label, NULL, &label_size, style.ButtonTextAlign, &bb);
		}
		else
		{
			ImGui::PushStyleColor(ImGuiCol_Text, style.Colors[ImGuiCol_TextDisabled]);
			ImGui::RenderTextClipped(bb.Min + style.FramePadding, bb.Max - style.FramePadding, label, NULL, &label_size, style.ButtonTextAlign, &bb);
			ImGui::PopStyleColor();
		}

		// Automatically close popups
		//if (pressed && !(flags & ImGuiButtonFlags_DontClosePopups) && (window->Flags & ImGuiWindowFlags_Popup))
		//    CloseCurrentPopup();

		IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.LastItemStatusFlags);
		return pressed;
	}

	bool Button(const std::string& label, const ImVec2& size /*= ImVec2(0, 0)*/)
	{
		return ImGui::Button(label.c_str(), size);
	}

	bool ButtonDisabled(const std::string& label, const ImVec2& size /*= ImVec2(0, 0)*/)
	{
		return UI::Button(UI::GetID(label), label, size, false, ImGuiButtonFlags_None);
	}

}
