#pragma once

#include "Shark/Render/FrameBuffer.h"

#include <imgui.h>

#include <DirectXMath.h>

namespace Shark {

	namespace UI {

		enum class ContentType
		{
			None = 0,
			Unkown = -1,
			Directory,
			Texture, Scene
		};

		struct ContentPayload
		{
			static constexpr const char* ID = "Content";
			char Path[260]; // 260 is max Path on Windows // Maby change to global heap alocated buffer;
			ContentType Type;
		};

	}

	inline ImVec2 operator-(const ImVec2& v2) { return { -v2.x, -v2.y }; }
	inline ImVec2 operator+(const ImVec2& v2) { return v2; }


	namespace UI {

		void DrawFloatShow(const std::string& label, float val, const char* fmt = "%.2f", float textWidth = 100.0f, const char* buttoncharacter = "X");
		void DrawVec2Show(const std::string& label, DirectX::XMFLOAT2 vec, const char* fmt = "%.2f", float textWidth = 100.0f, const char* buttoncharacters = "X\0Y");
		void DrawVec3Show(const std::string& label, DirectX::XMFLOAT3 vec, const char* fmt = "%.2f", float textWidth = 100.0f, const char* buttoncharacters = "X\0Y\0Z");

		bool DrawFloatControl(const std::string& label, float& val, float resetVal = 0.0f, const char* fmt = "%.2f", float textWidth = 100.0f, const char* buttoncharacter = "X");
		bool DrawVec2Control(const std::string& label, DirectX::XMFLOAT2& vec, float resetVal = 0.0f, const char* fmt = "%.2f", float textWidth = 100.0f, const char* buttoncharacters = "X\0Y");
		bool DrawVec3Control(const std::string& label, DirectX::XMFLOAT3& vec, float resetVal = 0.0f, const char* fmt = "%.2f", float textWidth = 100.0f, const char* buttoncharacters = "X\0Y\0Z");

		bool DrawFloatXControl(const std::string& label, float* data, uint32_t count, float resetVal = 0.0f, const char* fmt = "%.2f", float textWidth = 100.0f);

		void NoAlpaImage(ImTextureID textureID, const ImVec2& size, const ImVec2& uv0 = { 0, 0 }, const ImVec2& uv1 = { 1, 1 }, const ImVec4& tintcolor = { 1, 1, 1, 1 }, const ImVec4& bordercolor = { 0, 0, 0, 0 });
		bool SelectTextureImageButton(Ref<Texture2D>& texture, const ImVec2& size, const ImVec2& uv0 = { 0, 0 }, const ImVec2& uv1 = { 1, 1 }, float framepadding = -1, const ImVec4& bgColor = { 0, 0, 0, 0 }, const ImVec4& tintcolor = { 1, 1, 1, 1 });

		bool ImageButton(const std::string& strID, ImTextureID textureID, const ImVec2& size, const ImVec2& uv0 = { 0, 0 }, const ImVec2& uv1 = { 1, 1 }, int frame_padding = -1, const ImVec4& bg_col = { 0, 0, 0, 0 }, const ImVec4& tint_col = { 1, 1, 1, 1 });
		bool ImageButton(ImGuiID id, ImTextureID textureID, const ImVec2& size, const ImVec2& uv0 = { 0, 0 }, const ImVec2& uv1 = { 1, 1 }, int frame_padding = -1, const ImVec4& bg_col = { 0, 0, 0, 0 }, const ImVec4& tint_col = { 1, 1, 1, 1 });

		ImVec2 GetItemSize(const std::string& lable);
		ImVec2 GetFramePadding();

		void TextWithBackGround(const std::string& text);
		void TextWithBackGround(const std::string& text, const ImVec4& bgColor);

		bool InputText(const char* label, std::string& str, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = NULL, void* user_data = NULL);
		bool InputText(const char* label, std::filesystem::path& path, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = NULL, void* user_data = NULL);

		void Text(const char* str);
		void Text(const std::string& str);
		void Text(const std::filesystem::path& path);

		void MoveCurserPos(ImVec2 delta);
		void MoveCurserPosX(float deltaX);
		void MoveCurserPosY(float deltaY);

		bool ButtonRightAligned(const std::string& label, const ImVec2& size = ImVec2(0, 0), ImGuiButtonFlags flags = ImGuiButtonFlags_None);

		bool BeginPopup(const std::string& label, ImGuiWindowFlags flags = ImGuiWindowFlags_None);
		bool BeginPopup(ImGuiID id, ImGuiWindowFlags flags = ImGuiWindowFlags_None);

		bool GetContentPayload(std::string& out_Path, ContentType type);
		bool GetContentPayload(std::filesystem::path& out_Path, ContentType type);

	}

}
