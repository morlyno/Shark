#pragma once

#include "Shark/Core/Base.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <DirectXMath.h>

namespace Shark::UI {
	inline namespace Deprecated {

		enum class SK_DEPRECATED("ContentType is deprecated, probaly a replacement very soon") ContentType
		{
			None = 0,
			Unkown = -1,
			Directory,
			Texture, Scene
		};

		struct SK_DEPRECATED("ContentType is deprecated, probaly a replacement very soon") ContentPayload
		{
			static constexpr const char* ID = "Content";
			char Path[260]; // 260 is max Path on Windows // Maby change to global heap alocated buffer;
			ContentType Type;
		};

		SK_DEPRECATED("MoveCurserPos is deprecated, probaly a replacement very soon")
		inline void MoveCurserPos(ImVec2 delta)
		{
			ImGuiWindow* window = ImGui::GetCurrentWindow();
			window->DC.CursorPos += delta;
		}

		SK_DEPRECATED("MoveCurserPosX is deprecated, probaly a replacement very soon")
		inline void MoveCurserPosX(float deltaX)
		{
			ImGuiWindow* window = ImGui::GetCurrentWindow();
			window->DC.CursorPos.x += deltaX;

		}

		SK_DEPRECATED("MoveCurserPosY is deprecated, probaly a replacement very soon")
		inline void MoveCurserPosY(float deltaY)
		{
			ImGuiWindow* window = ImGui::GetCurrentWindow();
			window->DC.CursorPos.y += deltaY;
		}

		SK_DEPRECATED("ImageButton is deprecated, probaly a replacement very soon")
		inline bool ImageButton(ImGuiID id, ImTextureID textureID, const ImVec2& size, const ImVec2& uv0 = { 0, 0 }, const ImVec2& uv1 = { 1, 1 }, int frame_padding = -1, const ImVec4& bg_col = { 0, 0, 0, 0 }, const ImVec4& tint_col = { 1, 1, 1, 1 })
		{
			ImGuiContext& g = *ImGui::GetCurrentContext();
			ImGuiWindow* window = g.CurrentWindow;
			if (window->SkipItems)
				return false;

			const ImVec2 padding = (frame_padding >= 0) ? ImVec2((float)frame_padding, (float)frame_padding) : g.Style.FramePadding;
			return ImGui::ImageButtonEx(id, textureID, size, uv0, uv1, padding, bg_col, tint_col);
		}

		SK_DEPRECATED("ImageButton is deprecated, probaly a replacement very soon")
		inline bool ImageButton(const std::string& strID, ImTextureID textureID, const ImVec2& size, const ImVec2& uv0 = { 0, 0 }, const ImVec2& uv1 = { 1, 1 }, int frame_padding = -1, const ImVec4& bg_col = { 0, 0, 0, 0 }, const ImVec4& tint_col = { 1, 1, 1, 1 })
		{
			ImGuiID id = ImGui::GetID(strID.c_str());
			return ImageButton(id, textureID, size, uv0, uv1, frame_padding, bg_col, tint_col);
		}

		SK_DEPRECATED("GetContentPayload is deprecated, probaly a replacement very soon")
		inline bool GetContentPayload(std::string& out_Path, ContentType type)
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

		SK_DEPRECATED("GetContentPayload is deprecated, probaly a replacement very soon")
		inline bool GetContentPayload(std::filesystem::path& out_Path, ContentType type)
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

		static inline ImVec4 ImVec4Max(const ImVec4& a, const ImVec4& b)
		{
			return ImVec4(
				a.x > b.x ? a.x : b.x,
				a.y > b.y ? a.y : b.y,
				a.z > b.z ? a.z : b.z,
				a.w > b.w ? a.w : b.w
			);
		}

		SK_DEPRECATED("ButtonDisabled is deprecated, probaly a replacement very soon")
		inline bool ButtonDisabled(const std::string& label_arg, const ImVec2& size_arg = ImVec2(0, 0), ImGuiButtonFlags flags = 0)
		{
			ImGuiWindow* window = ImGui::GetCurrentWindow();
			if (window->SkipItems)
				return false;

			constexpr bool active = false;
			const ImGuiID id = ImGui::GetID(label_arg.c_str());

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
	}
}

namespace Shark::UI {

	void NewFrame();

	//////////////////////////////////////////////////////////////////////////////
	/// Helpers //////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	struct ScopedID
	{
		const ImGuiID ID;
		ScopedID(ImGuiID id);
		~ScopedID();
	};

	ImGuiID GetID(int intID);
	ImGuiID GetID(void* ptrID);
	ImGuiID GetID(const std::string& strID);

	void PushID(ImGuiID id);
	ImGuiID PopID();

	ImGuiID GetCurrentID();

	void SetBlend(bool blend);

	//////////////////////////////////////////////////////////////////////////////
	/// Text /////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	void Text(const std::string& str);
	void Text(const std::filesystem::path& path);
	void TextAligned(const std::string& str);

	void TextWithBackGround(const std::string& text);
	void TextWithBackGround(const std::string& text, const ImVec4& bgColor);
	void TextWithBackGround(const std::filesystem::path& text);
	void TextWithBackGround(const std::filesystem::path& text, const ImVec4& bgColor);

	bool InputText(const std::string& tag, std::string& buffer);
	bool InputText(const std::string& tag, std::filesystem::path& buffer);

	//////////////////////////////////////////////////////////////////////////////
	/// Controls /////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	bool BeginControls();
	void EndControls();

	bool BeginControlsGrid();
	void EndControlsGrid();

	bool DragFloat(const std::string& tag, float& val,             float resetVal = 0.0f, float min = 0.0f, float max = 0.0f, float speed = 1.0f, const char* fmt = "%.2f", ImGuiSliderFlags flags = ImGuiSliderFlags_None);
	bool DragFloat(const std::string& tag, DirectX::XMFLOAT2& val, float resetVal = 0.0f, float min = 0.0f, float max = 0.0f, float speed = 1.0f, const char* fmt = "%.2f", ImGuiSliderFlags flags = ImGuiSliderFlags_None);
	bool DragFloat(const std::string& tag, DirectX::XMFLOAT3& val, float resetVal = 0.0f, float min = 0.0f, float max = 0.0f, float speed = 1.0f, const char* fmt = "%.2f", ImGuiSliderFlags flags = ImGuiSliderFlags_None);
	bool DragFloat(const std::string& tag, DirectX::XMFLOAT4& val, float resetVal = 0.0f, float min = 0.0f, float max = 0.0f, float speed = 1.0f, const char* fmt = "%.2f", ImGuiSliderFlags flags = ImGuiSliderFlags_None);
	
	bool SliderFloat(const std::string& tag, float& val,             float resetVal = 0.0f, float min = 0.0f, float max = 0.0f, const char* fmt = "%.2f", ImGuiSliderFlags flags = ImGuiSliderFlags_None);
	bool SliderFloat(const std::string& tag, DirectX::XMFLOAT2& val, float resetVal = 0.0f, float min = 0.0f, float max = 0.0f, const char* fmt = "%.2f", ImGuiSliderFlags flags = ImGuiSliderFlags_None);
	bool SliderFloat(const std::string& tag, DirectX::XMFLOAT3& val, float resetVal = 0.0f, float min = 0.0f, float max = 0.0f, const char* fmt = "%.2f", ImGuiSliderFlags flags = ImGuiSliderFlags_None);
	bool SliderFloat(const std::string& tag, DirectX::XMFLOAT4& val, float resetVal = 0.0f, float min = 0.0f, float max = 0.0f, const char* fmt = "%.2f", ImGuiSliderFlags flags = ImGuiSliderFlags_None);
	
	bool DragInt(const std::string& tag, int& val,             int resetVal = 0.0f, int min = 0.0f, int max = 0.0f, float speed = 1.0f, const char* fmt = "%d", ImGuiSliderFlags flags = ImGuiSliderFlags_None);
	bool DragInt(const std::string& tag, DirectX::XMINT2& val, int resetVal = 0.0f, int min = 0.0f, int max = 0.0f, float speed = 1.0f, const char* fmt = "%d", ImGuiSliderFlags flags = ImGuiSliderFlags_None);
	bool DragInt(const std::string& tag, DirectX::XMINT3& val, int resetVal = 0.0f, int min = 0.0f, int max = 0.0f, float speed = 1.0f, const char* fmt = "%d", ImGuiSliderFlags flags = ImGuiSliderFlags_None);
	bool DragInt(const std::string& tag, DirectX::XMINT4& val, int resetVal = 0.0f, int min = 0.0f, int max = 0.0f, float speed = 1.0f, const char* fmt = "%d", ImGuiSliderFlags flags = ImGuiSliderFlags_None);
	
	bool SliderInt(const std::string& tag, int& val,             int resetVal = 0.0f, int min = 0.0f, int max = 0.0f, const char* fmt = "%d", ImGuiSliderFlags flags = ImGuiSliderFlags_None);
	bool SliderInt(const std::string& tag, DirectX::XMINT2& val, int resetVal = 0.0f, int min = 0.0f, int max = 0.0f, const char* fmt = "%d", ImGuiSliderFlags flags = ImGuiSliderFlags_None);
	bool SliderInt(const std::string& tag, DirectX::XMINT3& val, int resetVal = 0.0f, int min = 0.0f, int max = 0.0f, const char* fmt = "%d", ImGuiSliderFlags flags = ImGuiSliderFlags_None);
	bool SliderInt(const std::string& tag, DirectX::XMINT4& val, int resetVal = 0.0f, int min = 0.0f, int max = 0.0f, const char* fmt = "%d", ImGuiSliderFlags flags = ImGuiSliderFlags_None);
	
	bool DragAngle(const std::string& tag, float& radians,             float resetVal = 0.0f, float min = 0.0f, float max = 0.0f, float speed = 1.0f, const char* fmt = "%.2f", ImGuiSliderFlags flags = ImGuiSliderFlags_None);
	bool DragAngle(const std::string& tag, DirectX::XMFLOAT2& radians, float resetVal = 0.0f, float min = 0.0f, float max = 0.0f, float speed = 1.0f, const char* fmt = "%.2f", ImGuiSliderFlags flags = ImGuiSliderFlags_None);
	bool DragAngle(const std::string& tag, DirectX::XMFLOAT3& radians, float resetVal = 0.0f, float min = 0.0f, float max = 0.0f, float speed = 1.0f, const char* fmt = "%.2f", ImGuiSliderFlags flags = ImGuiSliderFlags_None);
	bool DragAngle(const std::string& tag, DirectX::XMFLOAT4& radians, float resetVal = 0.0f, float min = 0.0f, float max = 0.0f, float speed = 1.0f, const char* fmt = "%.2f", ImGuiSliderFlags flags = ImGuiSliderFlags_None);

	bool ColorEdit(const std::string& tag, DirectX::XMFLOAT4& color, ImGuiColorEditFlags flags = ImGuiColorEditFlags_None);

	bool Checkbox(const std::string& tag, bool& v);
	bool Checkbox(const std::string& tag, const bool& v);

	bool ButtonRightAligned(const std::string& tag, const ImVec2& size = ImVec2(0, 0), ImGuiButtonFlags flags = ImGuiButtonFlags_None);

	bool BeginCustomControl(ImGuiID id);
	void EndCustomControl();

}