#include "skpch.h"
#include "Renderer2D.h"
#include "Shark/Render/RendererCommand.h"

#include "Shark/Render/Buffers.h"
#include "Shark/Render/Shaders.h"

#include <DirectXMath.h>

namespace Shark {

	struct QuadVertex
	{
		DirectX::XMFLOAT3 Pos;
		DirectX::XMFLOAT4 Color;
		DirectX::XMFLOAT2 Tex;
		int TextureIndex;
		float TilingFactor;
	};

	struct BatchRendererData
	{
		static constexpr uint32_t MaxQuads = 20000;
		static constexpr uint32_t MaxQuadVertices = MaxQuads * 4;
		static constexpr uint32_t MaxQuadIndices = MaxQuads * 6;
		static constexpr uint32_t MaxQuadVerticesSize = MaxQuadVertices * sizeof(QuadVertex);
		static constexpr uint32_t MaxTextureSlots = 16; // 16 is the max for samplers

		Ref<VertexBuffer> QuadVertexBuffer;
		Ref<IndexBuffer> QuadIndexBuffer;
		Ref<Shaders> Shader;

		QuadVertex* QuadVertexBasePtr = nullptr;
		QuadVertex* QuadVertexIndexPtr = nullptr;
		uint32_t QuadCount = 0;

		Ref<Texture2D> WitheTexture;
		std::array<Ref<Texture2D>, MaxTextureSlots> Textures;
		uint32_t TextureCount = 0;

		Renderer2D::Statistiks Stats;
	};
	static BatchRendererData s_BatchData;

	struct SceanData
	{
		DirectX::XMMATRIX ViewProjectionMatrix;
	};
	static SceanData s_SceanData;

	void Renderer2D::Init()
	{
		SK_CORE_ASSERT(!s_BatchData.QuadVertexBasePtr, "QuadVertexBasePtr allredy set!");

		s_BatchData.QuadVertexBasePtr = new QuadVertex[s_BatchData.MaxQuadVertices];
		s_BatchData.QuadVertexIndexPtr = s_BatchData.QuadVertexBasePtr;

		s_BatchData.Shader = Shaders::Create("assets/Shaders/MainShader.hlsl");
		s_BatchData.WitheTexture = Texture2D::Create({}, 1, 1, 0xFFFFFFFF);
		s_BatchData.Textures[0] = s_BatchData.WitheTexture;

		s_BatchData.QuadVertexBuffer = VertexBuffer::Create(s_BatchData.Shader->GetVertexLayout(), true);
		s_BatchData.QuadVertexBuffer->SetData(Buffer::Ref( s_BatchData.QuadVertexBasePtr, sizeof(QuadVertex) * s_BatchData.MaxQuadVertices ));

		uint32_t* indices = new uint32_t[s_BatchData.MaxQuadIndices];
		uint32_t offset = 0;
		for (uint32_t i = 0; i < s_BatchData.MaxQuadIndices; i += 6)
		{
			indices[i + 0] = offset + 0;
			indices[i + 1] = offset + 1;
			indices[i + 2] = offset + 2;

			indices[i + 3] = offset + 2;
			indices[i + 4] = offset + 3;
			indices[i + 5] = offset + 0;

			offset += 4;
		}

		s_BatchData.QuadIndexBuffer = IndexBuffer::Create(Buffer::Ref(indices, s_BatchData.MaxQuadIndices));
		delete[] indices;

#ifdef SK_DEBUG
		for (uint32_t i = 0; i < s_BatchData.MaxTextureSlots; ++i)
			s_BatchData.Textures[0]->Bind(i);
#endif
	}

	void Renderer2D::ShutDown()
	{
		delete[] s_BatchData.QuadVertexBasePtr;
		s_BatchData.QuadVertexBuffer.reset();
		s_BatchData.QuadIndexBuffer.reset();
		s_BatchData.Shader.reset();
		s_BatchData.Textures = {};
	}

	static void ReBind()
	{
		s_BatchData.Shader->Bind();
		s_BatchData.QuadVertexBuffer->Bind();
		s_BatchData.QuadIndexBuffer->Bind();
		for (uint32_t i = 0; i < s_BatchData.TextureCount; ++i)
			s_BatchData.Textures[i]->Bind();
	}

	static void DrawBatch()
	{
		ReBind();

		s_BatchData.Shader->SetBuffer("SceanData", Buffer::Ref(s_SceanData));
		s_BatchData.QuadVertexBuffer->UpdateData(Buffer::Ref( s_BatchData.QuadVertexBasePtr, s_BatchData.MaxQuadVerticesSize ));

		RendererCommand::DrawIndexed(s_BatchData.QuadCount * 6);

		s_BatchData.Stats.DrawCalls++;
		s_BatchData.Stats.QuadCount += s_BatchData.QuadCount;
		s_BatchData.Stats.TextureCount += s_BatchData.TextureCount;

		s_BatchData.QuadVertexIndexPtr = s_BatchData.QuadVertexBasePtr;
		s_BatchData.QuadCount = 0;

		s_BatchData.TextureCount = 1;
		s_BatchData.Textures = {};
		s_BatchData.Textures[0] = s_BatchData.WitheTexture;
	}

	Renderer2D::Statistiks Renderer2D::GetStats()
	{
		return s_BatchData.Stats;
	}

	void Renderer2D::ResetStats()
	{
		s_BatchData.Stats.DrawCalls = 0;
		s_BatchData.Stats.QuadCount = 0;
		s_BatchData.Stats.TextureCount = 0;
	}

	void Renderer2D::BeginScean(Camera& camera, const DirectX::XMMATRIX& view)
	{
		ResetStats();
		s_SceanData.ViewProjectionMatrix = view * camera.GetProjection();
	}

	void Renderer2D::BeginScean(EditorCamera& camera)
	{
		ResetStats();
		s_SceanData.ViewProjectionMatrix = camera.GetViewProjection();
	}

	void Renderer2D::EndScean()
	{
		DrawBatch();
	}

	void Renderer2D::DrawQuad(const DirectX::XMFLOAT2& pos, const DirectX::XMFLOAT2& scaling, const DirectX::XMFLOAT4& color)
	{
		DrawQuad({ pos.x, pos.y, 0.0f }, scaling, color);
	}

	void Renderer2D::DrawQuad(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT2& scaling, const DirectX::XMFLOAT4& color)
	{
		const auto translation = DirectX::XMMatrixScaling(scaling.x, scaling.y, 1.0f) * DirectX::XMMatrixTranslation(pos.x, pos.y, pos.z);
		DrawQuad(translation, color);
	}

	void Renderer2D::DrawQuad(const DirectX::XMFLOAT2& pos, const DirectX::XMFLOAT2& scaling, Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tint_color)
	{
		DrawQuad({ pos.x, pos.y, 0.0f }, scaling, texture, tilingfactor, tint_color);
	}

	void Renderer2D::DrawQuad(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT2& scaling, Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tint_color)
	{
		const auto translation = DirectX::XMMatrixScaling(scaling.x, scaling.y, 1.0f) * DirectX::XMMatrixTranslation(pos.x, pos.y, pos.z);
		DrawQuad(translation, texture, tilingfactor, tint_color);
	}

	void Renderer2D::DrawRotatedQuad(const DirectX::XMFLOAT2& pos, float rotation, const DirectX::XMFLOAT2& scaling, const DirectX::XMFLOAT4& color)
	{
		DrawRotatedQuad({ pos.x, pos.y, 0.0f }, rotation, scaling, color);
	}

	void Renderer2D::DrawRotatedQuad(const DirectX::XMFLOAT3& pos, float rotation, const DirectX::XMFLOAT2& scaling, const DirectX::XMFLOAT4& color)
	{
		const auto translation = DirectX::XMMatrixScaling(scaling.x, scaling.y, 1.0f) * DirectX::XMMatrixRotationZ(rotation) * DirectX::XMMatrixTranslation(pos.x, pos.y, pos.z);
		DrawQuad(translation, color);
	}

	void Renderer2D::DrawRotatedQuad(const DirectX::XMFLOAT2& pos, float rotation, const DirectX::XMFLOAT2& scaling, Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tint_color)
	{
		DrawRotatedQuad({ pos.x, pos.y, 0.0f }, rotation, scaling, texture, tilingfactor, tint_color);
	}

	void Renderer2D::DrawRotatedQuad(const DirectX::XMFLOAT3& pos, float rotation, const DirectX::XMFLOAT2& scaling, Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tint_color)
	{
		const auto translation = DirectX::XMMatrixScaling(scaling.x, scaling.y, 1.0f) * DirectX::XMMatrixRotationZ(rotation) * DirectX::XMMatrixTranslation(pos.x, pos.y, pos.z);
		DrawQuad(translation, texture, tilingfactor, tint_color);
	}

	void Renderer2D::DrawQuad(const DirectX::XMMATRIX& translation, const DirectX::XMFLOAT4& color)
	{
		if (s_BatchData.QuadCount >= s_BatchData.MaxQuads)
			DrawBatch();

		constexpr DirectX::XMFLOAT3 TopLeft = { -0.5f,  0.5f, 0.0f };
		DirectX::XMStoreFloat3(&s_BatchData.QuadVertexIndexPtr->Pos, DirectX::XMVector3Transform(DirectX::XMLoadFloat3(&TopLeft), translation));
		s_BatchData.QuadVertexIndexPtr->Color = color;
		s_BatchData.QuadVertexIndexPtr->Tex = { 0.0f, 0.0f };
		s_BatchData.QuadVertexIndexPtr->TextureIndex = 0;
		s_BatchData.QuadVertexIndexPtr->TilingFactor = 1.0f;
		s_BatchData.QuadVertexIndexPtr++;

		constexpr DirectX::XMFLOAT3 TopRight = { 0.5f,  0.5f, 0.0f };
		DirectX::XMStoreFloat3(&s_BatchData.QuadVertexIndexPtr->Pos, DirectX::XMVector3Transform(DirectX::XMLoadFloat3(&TopRight), translation));
		s_BatchData.QuadVertexIndexPtr->Color = color;
		s_BatchData.QuadVertexIndexPtr->Tex = { 1.0f, 0.0f };
		s_BatchData.QuadVertexIndexPtr->TextureIndex = 0;
		s_BatchData.QuadVertexIndexPtr->TilingFactor = 1.0f;
		s_BatchData.QuadVertexIndexPtr++;

		constexpr DirectX::XMFLOAT3 BottemRight = { 0.5f, -0.5f, 0.0f };
		DirectX::XMStoreFloat3(&s_BatchData.QuadVertexIndexPtr->Pos, DirectX::XMVector3Transform(DirectX::XMLoadFloat3(&BottemRight), translation));
		s_BatchData.QuadVertexIndexPtr->Color = color;
		s_BatchData.QuadVertexIndexPtr->Tex = { 1.0f, 1.0f };
		s_BatchData.QuadVertexIndexPtr->TextureIndex = 0;
		s_BatchData.QuadVertexIndexPtr->TilingFactor = 1.0f;
		s_BatchData.QuadVertexIndexPtr++;

		constexpr DirectX::XMFLOAT3 BottemLeft = { -0.5f, -0.5f, 0.0f };
		DirectX::XMStoreFloat3(&s_BatchData.QuadVertexIndexPtr->Pos, DirectX::XMVector3Transform(DirectX::XMLoadFloat3(&BottemLeft), translation));
		s_BatchData.QuadVertexIndexPtr->Color = color;
		s_BatchData.QuadVertexIndexPtr->Tex = { 0.0f, 1.0f };
		s_BatchData.QuadVertexIndexPtr->TextureIndex = 0;
		s_BatchData.QuadVertexIndexPtr->TilingFactor = 1.0f;
		s_BatchData.QuadVertexIndexPtr++;


		s_BatchData.QuadCount++;
	}

	void Renderer2D::DrawQuad(const DirectX::XMMATRIX& translation, Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tint_color)
	{
		if (s_BatchData.QuadCount >= s_BatchData.MaxQuads)
			DrawBatch();

		uint32_t slot = 0;
		for (uint32_t i = 0; i < s_BatchData.TextureCount; ++i)
		{
			if (s_BatchData.Textures[i] == texture)
			{
				slot = i;
				break;
			}
		}

		if (slot == 0)
		{
			if (s_BatchData.TextureCount >= s_BatchData.MaxTextureSlots)
				DrawBatch();

			s_BatchData.Textures[s_BatchData.TextureCount] = texture;
			slot = s_BatchData.TextureCount++;
			texture->SetSlot(slot);
		}

		constexpr DirectX::XMFLOAT3 TopLeft = { -0.5f,  0.5f, 0.0f };
		DirectX::XMStoreFloat3(&s_BatchData.QuadVertexIndexPtr->Pos, DirectX::XMVector3Transform(DirectX::XMLoadFloat3(&TopLeft), translation));
		s_BatchData.QuadVertexIndexPtr->Color = tint_color;
		s_BatchData.QuadVertexIndexPtr->Tex = { 0.0f, 0.0f };
		s_BatchData.QuadVertexIndexPtr->TextureIndex = slot;
		s_BatchData.QuadVertexIndexPtr->TilingFactor = tilingfactor;
		s_BatchData.QuadVertexIndexPtr++;

		constexpr DirectX::XMFLOAT3 TopRight = { 0.5f,  0.5f, 0.0f };
		DirectX::XMStoreFloat3(&s_BatchData.QuadVertexIndexPtr->Pos, DirectX::XMVector3Transform(DirectX::XMLoadFloat3(&TopRight), translation));
		s_BatchData.QuadVertexIndexPtr->Color = tint_color;
		s_BatchData.QuadVertexIndexPtr->Tex = { 1.0f, 0.0f };
		s_BatchData.QuadVertexIndexPtr->TextureIndex = slot;
		s_BatchData.QuadVertexIndexPtr->TilingFactor = tilingfactor;
		s_BatchData.QuadVertexIndexPtr++;

		constexpr DirectX::XMFLOAT3 BottemRight = { 0.5f, -0.5f, 0.0f };
		DirectX::XMStoreFloat3(&s_BatchData.QuadVertexIndexPtr->Pos, DirectX::XMVector3Transform(DirectX::XMLoadFloat3(&BottemRight), translation));
		s_BatchData.QuadVertexIndexPtr->Color = tint_color;
		s_BatchData.QuadVertexIndexPtr->Tex = { 1.0f, 1.0f };
		s_BatchData.QuadVertexIndexPtr->TextureIndex = slot;
		s_BatchData.QuadVertexIndexPtr->TilingFactor = tilingfactor;
		s_BatchData.QuadVertexIndexPtr++;

		constexpr DirectX::XMFLOAT3 BottemLeft = { -0.5f, -0.5f, 0.0f };
		DirectX::XMStoreFloat3(&s_BatchData.QuadVertexIndexPtr->Pos, DirectX::XMVector3Transform(DirectX::XMLoadFloat3(&BottemLeft), translation));
		s_BatchData.QuadVertexIndexPtr->Color = tint_color;
		s_BatchData.QuadVertexIndexPtr->Tex = { 0.0f, 1.0f };
		s_BatchData.QuadVertexIndexPtr->TextureIndex = slot;
		s_BatchData.QuadVertexIndexPtr->TilingFactor = tilingfactor;
		s_BatchData.QuadVertexIndexPtr++;


		s_BatchData.QuadCount++;
	}

}
