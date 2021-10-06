#include "skpch.h"
#include "Renderer2D.h"
#include "Shark/Render/RendererCommand.h"

#include "Shark/Render/Renderer.h"

#include "Shark/Render/Buffers.h"
#include "Shark/Render/Shaders.h"
#include "Shark/Render/ConstantBuffer.h"

#include "Shark/Scene/Components/TransformComponent.h"
#include "Shark/Scene/Components/SpriteRendererComponent.h"
#include "Shark/Scene/Components/TagComponent.h"

#include <DirectXMath.h>

#include "Shark/Utility/Math.h"

namespace Shark {

	using Index = IndexBuffer::IndexType;

	struct QuadVertex
	{
		DirectX::XMFLOAT3 Pos;
		DirectX::XMFLOAT4 Color;
		DirectX::XMFLOAT2 Tex;
		int TextureIndex;
		float TilingFactor;
		int ID;
	};

	struct CircleVertex
	{
		DirectX::XMFLOAT3 Pos;
		DirectX::XMFLOAT2 LocalPosition;
		DirectX::XMFLOAT4 Color;
		float Thickness;
		DirectX::XMFLOAT2 TexCoord;
		float TilingFactor;
		int TextureIndex;
		int ID;
	};

	struct LineVertex
	{
		DirectX::XMFLOAT3 Pos;
		DirectX::XMFLOAT4 Color;
		int ID;
	};

	struct BatchData
	{
		static constexpr uint32_t MaxTextures = 16;
		static constexpr uint32_t MaxGeometry = 20000;
		static constexpr uint32_t MaxQuadVertices = MaxGeometry * 4;
		static constexpr uint32_t MaxLineVertices = MaxGeometry * 2;
		static constexpr uint32_t MaxQuadIndices = MaxGeometry * 6;
		static constexpr uint32_t QuadBatchTextureBasemx = MaxTextures * 0;
		static constexpr uint32_t CircleBatchTextureBaseIndex = MaxTextures * 1;

		struct QuadBatch
		{
			QuadVertex* VertexBasePtr = nullptr;
			QuadVertex* VertexIndexPtr = nullptr;
			std::array<Ref<Texture2D>, BatchData::MaxTextures> Textures;
			uint32_t TextureCount = 0;
			uint32_t Count = 0;
		};
		struct CircleBatch
		{
			CircleVertex* VertexBasePtr = nullptr;
			CircleVertex* VertexIndexPtr = nullptr;
			std::array<Ref<Texture2D>, BatchData::MaxTextures> Textures;
			uint32_t TextureCount = 0;
			uint32_t Count = 0;
		};
		struct LineBatch
		{
			LineVertex* VertexBasePtr = nullptr;
			LineVertex* VertexIndexPtr = nullptr;
			uint32_t Count = 0;
		};

		Ref<Texture2D> WhiteTexture;
		Ref<ConstantBuffer> ViewProjection;
		Ref<VertexBuffer> QuadVertexBuffer;
		Ref<VertexBuffer> CircleVertexBuffer;
		Ref<VertexBuffer> LineVertexBuffer;
		Ref<IndexBuffer> QuadIndexBuffer;

		Ref<Shaders> QuadShader;
		Ref<Shaders> CircleShader;
		Ref<Shaders> LineShader;

		QuadBatch QuadBatch;
		CircleBatch CircleBatch;
		LineBatch LineBatch;

		Renderer2D::Statistics Stats;
	};

	static BatchData* s_Data;

	namespace Internal {

		static void ResetStates()
		{
			memset(&s_Data->Stats, 0, sizeof(s_Data->Stats));
		}

		static void FlushQuad()
		{
			auto& batch = s_Data->QuadBatch;

			if (batch.Count == 0)
				return;

			s_Data->QuadVertexBuffer->SetData(batch.VertexBasePtr, BatchData::MaxQuadVertices);

			s_Data->ViewProjection->Bind();
			s_Data->QuadVertexBuffer->Bind();
			s_Data->QuadIndexBuffer->Bind();
			s_Data->QuadShader->Bind();

			for (uint32_t i = 0; i < batch.TextureCount; i++)
				batch.Textures[i]->Bind(i);

			RendererCommand::DrawIndexed(batch.Count * 6, PrimitveTopology::Triangle);

			s_Data->Stats.DrawCalls++;
			s_Data->Stats.TextureCount += batch.TextureCount;

			batch.VertexIndexPtr = batch.VertexBasePtr;
			batch.TextureCount = 1;
			batch.Textures.fill(nullptr);
			batch.Textures[0] = s_Data->WhiteTexture;
			batch.Count = 0;
		}

		static void FlushCircle()
		{
			auto& batch = s_Data->CircleBatch;

			if (batch.Count == 0)
				return;

			s_Data->CircleVertexBuffer->SetData(batch.VertexBasePtr, BatchData::MaxQuadVertices);

			s_Data->ViewProjection->Bind();
			s_Data->CircleVertexBuffer->Bind();
			s_Data->QuadIndexBuffer->Bind();
			s_Data->CircleShader->Bind();

			for (uint32_t i = 0; i < batch.TextureCount; i++)
				batch.Textures[i]->Bind(i);

			RendererCommand::DrawIndexed(batch.Count * 6, PrimitveTopology::Triangle);

			s_Data->Stats.DrawCalls++;
			s_Data->Stats.TextureCount += batch.TextureCount;

			batch.VertexIndexPtr = batch.VertexBasePtr;
			batch.TextureCount = 1;
			batch.Textures.fill(nullptr);
			batch.Textures[0] = s_Data->WhiteTexture;
			batch.Count = 0;
		}

		static void FlushLine()
		{
			auto& batch = s_Data->LineBatch;

			if (batch.Count == 0)
				return;

			s_Data->LineVertexBuffer->SetData(batch.VertexBasePtr, BatchData::MaxLineVertices);

			s_Data->ViewProjection->Bind();
			s_Data->LineVertexBuffer->Bind();
			s_Data->LineShader->Bind();

			RendererCommand::Draw(batch.Count * 2, PrimitveTopology::Line);

			s_Data->Stats.DrawCalls++;

			batch.VertexIndexPtr = batch.VertexBasePtr;
			batch.Count = 0;
		}

		static void AddQuad(const DirectX::XMMATRIX& translation, const Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tintcolor, int id)
		{
			constexpr DirectX::XMFLOAT3 Vertices[4] = { { -0.5f, 0.5f, 0.0f }, { 0.5f, 0.5f, 0.0f }, { 0.5f, -0.5f, 0.0f }, { -0.5f, -0.5f, 0.0f } };
			constexpr DirectX::XMFLOAT2 TexCoords[4] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

			auto& batch = s_Data->QuadBatch;

			if (batch.Count >= BatchData::MaxGeometry || batch.TextureCount >= BatchData::MaxTextures)
				FlushQuad();

			uint32_t index = 0;
			if (texture != s_Data->WhiteTexture)
			{
				for (uint32_t i = 1; i < batch.TextureCount; i++)
					if (batch.Textures[i] == texture)
						index = i;

				if (index == 0)
				{
					index = batch.TextureCount;
					batch.Textures[batch.TextureCount++] = texture;
				}
			}

			for (uint32_t i = 0; i < 4; i++)
			{
				QuadVertex* vtx = batch.VertexIndexPtr++;
				DirectX::XMStoreFloat3(&vtx->Pos, DirectX::XMVector3Transform(DirectX::XMLoadFloat3(&Vertices[i]), translation));
				vtx->Color = tintcolor;
				vtx->Tex = TexCoords[i];
				vtx->TextureIndex = index;
				vtx->TilingFactor = tilingfactor;
				vtx->ID = id;
			}

			batch.Count++;

			s_Data->Stats.ElementCount++;
			s_Data->Stats.QuadCount++;
			s_Data->Stats.VertexCount += 4;
			s_Data->Stats.IndexCount += 6;
		}

		static void AddCircle(const DirectX::XMMATRIX& translation, float thickness, const Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tintcolor, int id)
		{
			constexpr DirectX::XMFLOAT3 Vertices[4] = { { -0.5f, 0.5f, 0.0f }, { 0.5f, 0.5f, 0.0f }, { 0.5f, -0.5f, 0.0f }, { -0.5f, -0.5f, 0.0f } };
			constexpr DirectX::XMFLOAT2 TexCoords[4] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

			auto& batch = s_Data->CircleBatch;

			if (batch.Count >= BatchData::MaxGeometry || batch.TextureCount >= BatchData::MaxTextures)
				FlushCircle();

			uint32_t index = 0;
			if (texture != s_Data->WhiteTexture)
			{
				for (uint32_t i = 1; i < batch.TextureCount; i++)
					if (batch.Textures[i] == texture)
						index = i;

				if (index == 0)
				{
					index = batch.TextureCount;
					batch.Textures[batch.TextureCount++] = texture;
				}
			}

			for (uint32_t i = 0; i < 4; i++)
			{
				CircleVertex* vtx = batch.VertexIndexPtr++;
				vtx->Pos = DXMath::Store3(DirectX::XMVector3Transform(DXMath::Load(Vertices[i]), translation));
				vtx->LocalPosition = { Vertices[i].x, Vertices[i].y };
				vtx->Thickness = thickness;
				vtx->Color = tintcolor;
				vtx->TexCoord = TexCoords[i];
				vtx->TextureIndex = index;
				vtx->TilingFactor = tilingfactor;
				vtx->ID = id;
			}

			batch.Count++;

			s_Data->Stats.ElementCount++;
			s_Data->Stats.CircleCount++;
			s_Data->Stats.VertexCount += 4;
			s_Data->Stats.IndexCount += 6;
		}

		static void AddLine(const DirectX::XMFLOAT3& pos0, const DirectX::XMFLOAT3& pos1, const DirectX::XMFLOAT4& color, int id)
		{
			auto& batch = s_Data->LineBatch;

			if (batch.Count >= BatchData::MaxGeometry)
				FlushLine();  

			LineVertex* vtx = batch.VertexIndexPtr;
			vtx->Pos = pos0;
			vtx->Color = color;
			vtx->ID = id;

			vtx++;
			vtx->Pos = pos1;
			vtx->Color = color;
			vtx->ID = id;

			batch.Count++;

			s_Data->Stats.ElementCount++;
			s_Data->Stats.LineCount++;
			s_Data->Stats.VertexCount += 2;

		}

	}

	void Renderer2D::Init()
	{
		s_Data = new BatchData;
		s_Data->ViewProjection = ConstantBuffer::Create(64, 0);

		s_Data->QuadBatch.VertexBasePtr = new QuadVertex[BatchData::MaxQuadVertices];
		s_Data->QuadBatch.VertexIndexPtr = s_Data->QuadBatch.VertexBasePtr;
		s_Data->CircleBatch.VertexBasePtr = new CircleVertex[BatchData::MaxQuadVertices];
		s_Data->CircleBatch.VertexIndexPtr = s_Data->CircleBatch.VertexBasePtr;
		s_Data->LineBatch.VertexBasePtr = new LineVertex[BatchData::MaxLineVertices];
		s_Data->LineBatch.VertexIndexPtr = s_Data->LineBatch.VertexBasePtr;

		s_Data->QuadShader = Renderer::GetShaderLib().Get("BatchShader2D_Quad");
		s_Data->CircleShader = Renderer::GetShaderLib().Get("BatchShader2D_Circle");
		s_Data->LineShader = Renderer::GetShaderLib().Get("BatchShader2D_Line");

		s_Data->QuadVertexBuffer = VertexBuffer::Create(s_Data->QuadShader->GetVertexLayout(), s_Data->QuadBatch.VertexBasePtr, BatchData::MaxQuadVertices, true);
		s_Data->CircleVertexBuffer = VertexBuffer::Create(s_Data->CircleShader->GetVertexLayout(), s_Data->CircleBatch.VertexBasePtr, BatchData::MaxQuadVertices, true);
		s_Data->LineVertexBuffer = VertexBuffer::Create(s_Data->LineShader->GetVertexLayout(), s_Data->LineBatch.VertexBasePtr, BatchData::MaxLineVertices, true);

		Index* quadIndices = new Index[BatchData::MaxQuadIndices];
		for (uint32_t i = 0, j = 0; i < BatchData::MaxQuadIndices; i += 6, j += 4)
		{
			quadIndices[i + 0] = j + 0;
			quadIndices[i + 1] = j + 1;
			quadIndices[i + 2] = j + 2;

			quadIndices[i + 3] = j + 2;
			quadIndices[i + 4] = j + 3;
			quadIndices[i + 5] = j + 0;
		}
		s_Data->QuadIndexBuffer = IndexBuffer::Create(quadIndices, BatchData::MaxQuadIndices);
		delete[] quadIndices;

		s_Data->WhiteTexture = Renderer::GetWhiteTexture();
		s_Data->QuadBatch.TextureCount = 1;
		s_Data->QuadBatch.Textures[0] = s_Data->WhiteTexture;
		s_Data->CircleBatch.TextureCount = 1;
		s_Data->CircleBatch.Textures[0] = s_Data->WhiteTexture;

#ifdef SK_DEBUG
		// Bind the withe texture to all texture slots to avoid the annoying directx warnings
		for (uint32_t i = 0; i < BatchData::MaxTextures; i++)
			s_Data->WhiteTexture->Bind(i);
#endif
	}

	void Renderer2D::ShutDown()
	{
		delete s_Data->QuadBatch.VertexBasePtr;
		delete s_Data->CircleBatch.VertexBasePtr;
		delete s_Data->LineBatch.VertexBasePtr;

		delete s_Data;
	}

	void Renderer2D::BeginScene(Camera& camera, const DirectX::XMMATRIX& view)
	{
		Internal::ResetStates();
		auto mat = view * camera.GetProjection();
		s_Data->ViewProjection->Set(&mat);
	}

	void Renderer2D::BeginScene(EditorCamera& camera)
	{
		Internal::ResetStates();
		auto mat = camera.GetViewProjection();
		s_Data->ViewProjection->Set(&mat);
	}

	void Renderer2D::EndScene()
	{
		Internal::FlushQuad();
		Internal::FlushCircle();
		Internal::FlushLine();
	}


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Quad ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void Renderer2D::DrawQuad(const DirectX::XMFLOAT2& position, const DirectX::XMFLOAT2& scaling, const DirectX::XMFLOAT4& color, int id)
	{
		DrawQuad({ position.x, position.y, 0.0f }, { scaling.x, scaling.y, 1.0f }, color, id);
	}

	void Renderer2D::DrawQuad(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& scaling, const DirectX::XMFLOAT4& color, int id /*= -1*/)
	{
		const auto translation = DirectX::XMMatrixScaling(scaling.x, scaling.y, scaling.x) * DirectX::XMMatrixTranslation(position.x, position.y, position.z);
		Internal::AddQuad(translation, s_Data->WhiteTexture, 1.0f, color, id);
	}

	void Renderer2D::DrawQuad(const DirectX::XMFLOAT2& position, const DirectX::XMFLOAT2& scaling, const Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tintcolor, int id)
	{
		DrawQuad({ position.x, position.y, 0.0f }, { scaling.x, scaling.y, 1.0f }, texture, tilingfactor, tintcolor, id);
	}

	void Renderer2D::DrawQuad(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& scaling, const Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tintcolor, int id /*= -1*/)
	{
		const auto translation = DirectX::XMMatrixScaling(scaling.x, scaling.y, scaling.x) * DirectX::XMMatrixTranslation(position.x, position.y, position.z);
		Internal::AddQuad(translation, texture, tilingfactor, tintcolor, id);
	}

	void Renderer2D::DrawRotatedQuad(const DirectX::XMFLOAT2& position, float rotation, const DirectX::XMFLOAT2& scaling, const DirectX::XMFLOAT4& color, int id)
	{
		DrawRotatedQuad({ position.x, position.y, 0.0f }, { 0.0f, 0.0f, rotation }, { scaling.x, scaling.y, 1.0f }, color, id);
	}

	void Renderer2D::DrawRotatedQuad(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& rotation, const DirectX::XMFLOAT3& scaling, const DirectX::XMFLOAT4& color, int id /*= -1*/)
	{
		const auto translation = DirectX::XMMatrixScaling(scaling.x, scaling.y, 1.0f) * DirectX::XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z) * DirectX::XMMatrixTranslation(position.x, position.y, position.z);
		Internal::AddQuad(translation, s_Data->WhiteTexture, 1.0f, color, id);
	}

	void Renderer2D::DrawRotatedQuad(const DirectX::XMFLOAT2& position, float rotation, const DirectX::XMFLOAT2& scaling, const Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tintcolor, int id)
	{
		DrawRotatedQuad({ position.x, position.y, 0.0f }, { 0.0f, 0.0f, rotation }, { scaling.x, scaling.y, 1.0f }, texture, tilingfactor, tintcolor, id);
	}

	void Renderer2D::DrawRotatedQuad(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& rotation, const DirectX::XMFLOAT3& scaling, const Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tintcolor, int id /*= -1*/)
	{
		const auto translation = DirectX::XMMatrixScaling(scaling.x, scaling.y, 1.0f) * DirectX::XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z) * DirectX::XMMatrixTranslation(position.x, position.y, position.z);
		Internal::AddQuad(translation, texture, tilingfactor, tintcolor, id);
	}


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Circle /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void Renderer2D::DrawCircle(const DirectX::XMFLOAT2& position, const DirectX::XMFLOAT2& scaling, float thickness, const DirectX::XMFLOAT4& color, int id)
	{
		DrawCircle({ position.x, position.y, 0.0f }, { scaling.x, scaling.y, 1.0f }, thickness, color, id);
	}

	void Renderer2D::DrawCircle(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& scaling, float thickness, const DirectX::XMFLOAT4& color, int id /*= -1*/)
	{
		const auto translation = DirectX::XMMatrixScaling(scaling.x, scaling.y, scaling.x) * DirectX::XMMatrixTranslation(position.x, position.y, position.z);
		Internal::AddCircle(translation, thickness, s_Data->WhiteTexture, 1.0f, color, id);
	}

	void Renderer2D::DrawCircle(const DirectX::XMFLOAT2& position, const DirectX::XMFLOAT2& scaling, float thickness, const Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tintcolor, int id)
	{
		DrawCircle({ position.x, position.y, 0.0f }, { scaling.x, scaling.y, 1.0f }, thickness, texture, tilingfactor, tintcolor, id);
	}

	void Renderer2D::DrawCircle(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& scaling, float thickness, const Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tintcolor, int id /*= -1*/)
	{
		const auto translation = DirectX::XMMatrixScaling(scaling.x, scaling.y, scaling.x) * DirectX::XMMatrixTranslation(position.x, position.y, position.z);
		Internal::AddCircle(translation, thickness, texture, tilingfactor, tintcolor, id);
	}

	void Renderer2D::DrawRotatedCircle(const DirectX::XMFLOAT2& position, float rotation, const DirectX::XMFLOAT2& scaling, float thickness, const DirectX::XMFLOAT4& color, int id)
	{
		DrawRotatedCircle({ position.x, position.y, 0.0f }, { 0.0f, 0.0f, rotation }, { scaling.x, scaling.y, 1.0f }, thickness, color, id);
	}

	void Renderer2D::DrawRotatedCircle(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& rotation, const DirectX::XMFLOAT3& scaling, float thickness, const DirectX::XMFLOAT4& color, int id /*= -1*/)
	{
		const auto translation = DirectX::XMMatrixScaling(scaling.x, scaling.y, 1.0f) * DirectX::XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z) * DirectX::XMMatrixTranslation(position.x, position.y, position.z);
		Internal::AddCircle(translation, thickness, s_Data->WhiteTexture, 1.0f, color, id);
	}

	void Renderer2D::DrawRotatedCircle(const DirectX::XMFLOAT2& position, float rotation, const DirectX::XMFLOAT2& scaling, float thickness, const Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tintcolor, int id)
	{
		DrawRotatedCircle({ position.x, position.y, 0.0f }, { 0.0f, 0.0f, rotation }, { scaling.x, scaling.y, 1.0f }, thickness, texture, tilingfactor, tintcolor, id);
	}

	void Renderer2D::DrawRotatedCircle(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& rotation, const DirectX::XMFLOAT3& scaling, float thickness, const Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tintcolor, int id /*= -1*/)
	{
		const auto translation = DirectX::XMMatrixScaling(scaling.x, scaling.y, 1.0f) * DirectX::XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z) * DirectX::XMMatrixTranslation(position.x, position.y, position.z);
		Internal::AddCircle(translation, thickness, texture, tilingfactor, tintcolor, id);
	}


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Line ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void Renderer2D::DrawLine(const DirectX::XMFLOAT2& pos0, const DirectX::XMFLOAT2& pos1, const DirectX::XMFLOAT4& color, int id)
	{
		DrawLine({ pos0.x, pos0.y, 0.0f }, { pos1.x, pos1.y, 0.0f }, color, id);
	}

	void Renderer2D::DrawLine(const DirectX::XMFLOAT3& pos0, const DirectX::XMFLOAT3& pos1, const DirectX::XMFLOAT4& color, int id /*= -1*/)
	{
		Internal::AddLine(pos0, pos1, color, id);
	}


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Entity /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void Renderer2D::DrawEntity(Entity entity)
	{
		SK_CORE_ASSERT(entity.HasComponent<TransformComponent>(), "Tried to Draw Entity without Transform Component");
		SK_CORE_ASSERT(entity.HasComponent<SpriteRendererComponent>(), "Tried to Draw Entity without Sprite Renderer Component");

		auto& tf = entity.GetComponent<TransformComponent>();
		auto& sr = entity.GetComponent<SpriteRendererComponent>();

		Ref<Texture2D> texture = sr.Texture ? sr.Texture : s_Data->WhiteTexture;
		switch (sr.Geometry)
		{
			case SpriteRendererComponent::GeometryType::Quad:     Internal::AddQuad(tf.GetTranform(), texture, sr.TilingFactor, sr.Color, (int)(uint32_t)entity);                    break;
			case SpriteRendererComponent::GeometryType::Circle:   Internal::AddCircle(tf.GetTranform(), sr.Thickness, texture, sr.TilingFactor, sr.Color, (int)(uint32_t)(entity));  break;
		}
	}

	Renderer2D::Statistics Renderer2D::GetStatistics()
	{
		return s_Data->Stats;
	}

}
