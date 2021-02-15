#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/OrtographicCamera.h"
#include "Shark/Render/Texture.h"

namespace Shark {

	class Renderer2D
	{
	public:
		static void Init();
		static void ShutDown();

		static void BeginScean(OrtographicCamera& camera);
		static void EndScean();

		static void DrawQuad(const DirectX::XMFLOAT2& pos, const DirectX::XMFLOAT2& scaling, const DirectX::XMFLOAT4& color);
		static void DrawQuad(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT2& scaling, const DirectX::XMFLOAT4& color);
		static void DrawQuad(const DirectX::XMFLOAT2& pos, const DirectX::XMFLOAT2& scaling, Ref<Texture2D>& texture, const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 1.0f, 1.0f }, float tilingfactor = 1);
		static void DrawQuad(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT2& scaling, Ref<Texture2D>& texture, const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 1.0f, 1.0f }, float tilingfactor = 1);

		static void DrawRotatedQuad(const DirectX::XMFLOAT2& pos, float rotation, const DirectX::XMFLOAT2& scaling, const DirectX::XMFLOAT4& color);
		static void DrawRotatedQuad(const DirectX::XMFLOAT3& pos, float rotation, const DirectX::XMFLOAT2& scaling, const DirectX::XMFLOAT4& color);
		static void DrawRotatedQuad(const DirectX::XMFLOAT2& pos, float rotation, const DirectX::XMFLOAT2& scaling, Ref<Texture2D>& texture, const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 1.0f, 1.0f }, float tilingfactor = 1);
		static void DrawRotatedQuad(const DirectX::XMFLOAT3& pos, float rotation, const DirectX::XMFLOAT2& scaling, Ref<Texture2D>& texture, const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 1.0f, 1.0f }, float tilingfactor = 1);

		static void DrawQuad(const DirectX::XMMATRIX& translation, const DirectX::XMFLOAT4& color);
		static void DrawQuad(const DirectX::XMMATRIX& translation, Ref<Texture2D>& texture, const DirectX::XMFLOAT4& color, float tilingfactor = 1);
		
		struct Statistiks
		{
			uint32_t DrawCalls = 0;
			uint32_t QuadCount = 0;
			uint32_t TextureCount = 0;

			uint32_t VertexCount() const { return QuadCount * 4; }
			uint32_t IndexCount() const { return QuadCount * 6; }
		};
		static Statistiks GetStats();
	private:
		static void ResetStats();
	};

}