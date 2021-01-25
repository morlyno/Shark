#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/RendererCommand.h"
#include "Shark/Render/OrtographicCamera.h"
#include "Shark/Render/Texture.h"

namespace Shark {

	class Renderer2D
	{
	public:
		static void Init(const class Window& window);
		static void ShutDown();

		static void BeginScean(OrtographicCamera& camera);
		static void EndScean();

		static void DrawQuad(const DirectX::XMFLOAT2& pos, const DirectX::XMFLOAT2& scaling, const DirectX::XMFLOAT4& color);
		static void DrawQuad(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT2& scaling, const DirectX::XMFLOAT4& color);
		static void DrawQuad(const DirectX::XMFLOAT2& pos, const DirectX::XMFLOAT2& scaling, Ref<Texture> texture, const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 1.0f, 1.0f });
		static void DrawQuad(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT2& scaling, Ref<Texture> texture, const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

		static void DrawRotatedQuad(const DirectX::XMFLOAT2& pos, float rotation, const DirectX::XMFLOAT2& scaling, const DirectX::XMFLOAT4& color);
		static void DrawRotatedQuad(const DirectX::XMFLOAT3& pos, float rotation, const DirectX::XMFLOAT2& scaling, const DirectX::XMFLOAT4& color);
		static void DrawRotatedQuad(const DirectX::XMFLOAT2& pos, float rotation, const DirectX::XMFLOAT2& scaling, Ref<Texture> texture, const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 1.0f, 1.0f });
		static void DrawRotatedQuad(const DirectX::XMFLOAT3& pos, float rotation, const DirectX::XMFLOAT2& scaling, Ref<Texture> texture, const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 1.0f, 1.0f });
	private:
		struct SceanData
		{
			DirectX::XMMATRIX ViewProjectionMatrix;
		};
		static SceanData* s_SceanData;
	};

}