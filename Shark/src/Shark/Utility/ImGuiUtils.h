#pragma once

#include "Shark/Render/FrameBuffer.h"

#include <imgui.h>

#include <DirectXMath.h>

namespace Shark {

	enum class AssetType
	{
		None = 0,
		Texture, Scene
	};

	struct AssetPayload
	{
		static constexpr const char* ID = "Asset";
		char FilePath[256];
		AssetType Type;
	};

	namespace UI {

		void DrawFloatShow(const char* lable, float val, const char* fmt = "%.2f", float textWidth = 100.0f, const char* buttoncharacter = "X");

		void DrawVec2Show(const char* lable, DirectX::XMFLOAT2 vec, const char* fmt = "%.2f", float textWidth = 100.0f, const char* buttoncharacters = "X\0Y");

		void DrawVec3Show(const char* lable, DirectX::XMFLOAT3 vec, const char* fmt = "%.2f", float textWidth = 100.0f, const char* buttoncharacters = "X\0Y\0Z");

		bool DrawFloatControl(const char* lable, float& val, float resetVal = 0.0f, const char* fmt = "%.2f", float textWidth = 100.0f, const char* buttoncharacter = "X");

		bool DrawVec2Control(const char* lable, DirectX::XMFLOAT2& vec, float resetVal = 0.0f, const char* fmt = "%.2f", float textWidth = 100.0f, const char* buttoncharacters = "X\0Y");

		bool DrawVec3Control(const char* lable, DirectX::XMFLOAT3& vec, float resetVal = 0.0f, const char* fmt = "%.2f", float textWidth = 100.0f, const char* buttoncharacters = "X\0Y\0Z");

		bool DrawFloatXControl(const char* lable, float* data, uint32_t count, float resetVal = 0.0f, const char* fmt = "%.2f", float textWidth = 100.0f);

		void NoAlpaImage(const Ref<FrameBuffer>& framebuffer, ImTextureID textureID, const ImVec2& size, const ImVec2& uv0 = { 0, 0 }, const ImVec2& uv1 = { 1, 1 }, const ImVec4& tintcolor = { 1, 1, 1, 1 }, const ImVec4& bordercolor = { 0, 0, 0, 0 });

		bool SelectTextureImageButton(Ref<Texture2D>& texture, const ImVec2& size, const ImVec2& uv0 = { 0, 0 }, const ImVec2& uv1 = { 1, 1 }, float framepadding = -1, const ImVec4& bgColor = { 0, 0, 0, 0 }, const ImVec4& tintcolor = { 1, 1, 1, 1 });

	}

}
