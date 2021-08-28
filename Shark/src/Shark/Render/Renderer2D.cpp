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
		DirectX::XMFLOAT3 Center;
		float Radius;
		DirectX::XMFLOAT4 Color;
		DirectX::XMFLOAT2 Tex;
		int TextureIndex;
		float TilingFactor;
		int ID;
	};

	struct BatchData
	{
		static constexpr uint32_t MaxTextures = 16;
		static constexpr uint32_t MaxGeometry = 20000;
		static constexpr uint32_t MaxVertices = MaxGeometry * 4;
		static constexpr uint32_t MaxIndices = MaxGeometry * 6;
		static constexpr uint32_t QuadBatchTextureBaseIndex = MaxTextures * 0;
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

		Ref<Texture2D> WhiteTexture;
		Ref<ConstantBuffer> ViewProjection;
		Ref<VertexBuffer> QuadVertexBuffer;
		Ref<VertexBuffer> CircleVertexBuffer;
		Ref<IndexBuffer> IndexBuffer;

		Ref<Material> QuadMaterial;
		Ref<Material> CircleMaterial;

		QuadBatch QuadBatch;
		CircleBatch CircleBatch;

		Renderer2D::Statistics Stats;
	};

	static BatchData* s_Data;

	namespace Internal {

		static void ResetStates()
		{
			auto& s = s_Data->Stats;
			s.DrawCalls = 0;
			s.ElementCount = 0;
			s.VertexCount = 0;
			s.IndexCount = 0;
			s.TextureCount = 0;
		}

		static void FlushQuad()
		{
			auto& batch = s_Data->QuadBatch;

			if (batch.Count == 0)
				return;

			s_Data->QuadVertexBuffer->SetData(batch.VertexBasePtr, BatchData::MaxVertices);

			s_Data->ViewProjection->Bind();
			s_Data->QuadVertexBuffer->Bind();
			s_Data->IndexBuffer->Bind();
			s_Data->QuadMaterial->GetShaders()->Bind();

			for (uint32_t i = 0; i < batch.TextureCount; i++)
				batch.Textures[i]->Bind(i);

			RendererCommand::DrawIndexed(batch.Count * 6);

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

			s_Data->CircleVertexBuffer->SetData(batch.VertexBasePtr, BatchData::MaxVertices);

			s_Data->ViewProjection->Bind();
			s_Data->CircleVertexBuffer->Bind();
			s_Data->IndexBuffer->Bind();
			s_Data->CircleMaterial->GetShaders()->Bind();

			for (uint32_t i = 0; i < batch.TextureCount; i++)
				batch.Textures[i]->Bind(i);

			RendererCommand::DrawIndexed(batch.Count * 6);

			s_Data->Stats.DrawCalls++;
			s_Data->Stats.TextureCount += batch.TextureCount;

			batch.VertexIndexPtr = batch.VertexBasePtr;
			batch.TextureCount = 1;
			batch.Textures.fill(nullptr);
			batch.Textures[0] = s_Data->WhiteTexture;
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
			s_Data->Stats.VertexCount += 4;
			s_Data->Stats.IndexCount += 6;
		}

		static void AddCircle(const DirectX::XMMATRIX& translation, float radius, const Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tintcolor, int id)
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
				vtx->Center = DXMath::Store3(DirectX::XMVector3Transform(DirectX::XMVectorZero(), translation));
				vtx->Radius = radius;

				vtx->Color = tintcolor;
				vtx->Tex = TexCoords[i];
				vtx->TextureIndex = index;
				vtx->TilingFactor = tilingfactor;
				vtx->ID = id;
			}

			batch.Count++;

			s_Data->Stats.ElementCount++;
			s_Data->Stats.VertexCount += 4;
			s_Data->Stats.IndexCount += 6;
		}

	}

	void Renderer2D::Init()
	{
		s_Data = new BatchData;
		s_Data->ViewProjection = ConstantBuffer::Create(64, 0);

		s_Data->QuadBatch.VertexBasePtr = new QuadVertex[BatchData::MaxVertices];
		s_Data->QuadBatch.VertexIndexPtr = s_Data->QuadBatch.VertexBasePtr;
		s_Data->CircleBatch.VertexBasePtr = new CircleVertex[BatchData::MaxVertices];
		s_Data->CircleBatch.VertexIndexPtr = s_Data->CircleBatch.VertexBasePtr;

		auto quadShader = Renderer::GetShaderLib().Get("QuadShader");
		auto circleShader = Renderer::GetShaderLib().Get("CircleShader");

		s_Data->QuadVertexBuffer = VertexBuffer::Create(quadShader->GetVertexLayout(), s_Data->QuadBatch.VertexBasePtr, BatchData::MaxVertices, true);
		s_Data->CircleVertexBuffer = VertexBuffer::Create(circleShader->GetVertexLayout(), s_Data->CircleBatch.VertexBasePtr, BatchData::MaxVertices, true);
		
		s_Data->QuadMaterial = Material::Create(quadShader, std::string{});
		s_Data->CircleMaterial = Material::Create(circleShader, std::string{});

		Index* indices = new Index[BatchData::MaxIndices];
		for (uint32_t i = 0, j = 0; i < BatchData::MaxIndices; i += 6, j += 4)
		{
			indices[i + 0] = j + 0;
			indices[i + 1] = j + 1;
			indices[i + 2] = j + 2;

			indices[i + 3] = j + 2;
			indices[i + 4] = j + 3;
			indices[i + 5] = j + 0;
		}
		s_Data->IndexBuffer = IndexBuffer::Create(indices, BatchData::MaxIndices);
		delete[] indices;

		s_Data->WhiteTexture = Renderer::GetWhiteTexture();
		s_Data->QuadBatch.TextureCount = 1;
		s_Data->QuadBatch.Textures[0] = s_Data->WhiteTexture;
		s_Data->CircleBatch.TextureCount = 1;
		s_Data->CircleBatch.Textures[0] = s_Data->WhiteTexture;

#ifdef SK_DEBUG
		for (uint32_t i = 0; i < BatchData::MaxTextures; i++)
			s_Data->WhiteTexture->Bind(i);
#endif
	}

	void Renderer2D::ShutDown()
	{
		delete s_Data->QuadBatch.VertexBasePtr;
		delete s_Data->CircleBatch.VertexBasePtr;

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
	}

	void Renderer2D::DrawQuad(const DirectX::XMFLOAT2& position, const DirectX::XMFLOAT2& scaling, const DirectX::XMFLOAT4& color, int id)
	{
		const auto translation = DirectX::XMMatrixScaling(scaling.x, scaling.y, 1.0f) * DirectX::XMMatrixTranslation(position.x, position.y, 0.0f);
		Internal::AddQuad(translation, s_Data->WhiteTexture, 1.0f, color, id);
	}

	void Renderer2D::DrawQuad(const DirectX::XMFLOAT2& position, const DirectX::XMFLOAT2& scaling, const Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tintcolor, int id)
	{
		const auto translation = DirectX::XMMatrixScaling(scaling.x, scaling.y, 1.0f) * DirectX::XMMatrixTranslation(position.x, position.y, 0.0f);
		Internal::AddQuad(translation, texture, tilingfactor, tintcolor, id);
	}

	void Renderer2D::DrawRotatedQuad(const DirectX::XMFLOAT2& position, float rotation, const DirectX::XMFLOAT2& scaling, const DirectX::XMFLOAT4& color, int id)
	{
		const auto translation = DirectX::XMMatrixScaling(scaling.x, scaling.y, 1.0f) * DirectX::XMMatrixRotationZ(rotation) * DirectX::XMMatrixTranslation(position.x, position.y, 0.0f);
		Internal::AddQuad(translation, s_Data->WhiteTexture, 1.0f, color, id);
	}

	void Renderer2D::DrawRotatedQuad(const DirectX::XMFLOAT2& position, float rotation, const DirectX::XMFLOAT2& scaling, const Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tintcolor, int id)
	{
		const auto translation = DirectX::XMMatrixScaling(scaling.x, scaling.y, 1.0f) * DirectX::XMMatrixRotationZ(rotation) * DirectX::XMMatrixTranslation(position.x, position.y, 0.0f);
		Internal::AddQuad(translation, texture, tilingfactor, tintcolor, id);
	}

	void Renderer2D::DrawCircle(const DirectX::XMFLOAT2& position, float radius, const DirectX::XMFLOAT4& color, int id)
	{
		const auto translation = DirectX::XMMatrixScaling(radius, radius, 1.0f) * DirectX::XMMatrixTranslation(position.x, position.y, 0.0f);
		Internal::AddCircle(translation, radius, s_Data->WhiteTexture, 1.0f, color, id);
	}

	void Renderer2D::DrawCircle(const DirectX::XMFLOAT2& position, float radius, const Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tintcolor, int id)
	{
		const auto translation = DirectX::XMMatrixScaling(radius, radius, 1.0f) * DirectX::XMMatrixTranslation(position.x, position.y, 0.0f);
		Internal::AddCircle(translation, radius, texture, tilingfactor, tintcolor, id);
	}

	void Renderer2D::DrawRotatedCircle(const DirectX::XMFLOAT2& position, float rotation, float radius, const DirectX::XMFLOAT4& color, int id)
	{
		const auto translation = DirectX::XMMatrixScaling(radius, radius, 1.0f) * DirectX::XMMatrixRotationZ(rotation) * DirectX::XMMatrixTranslation(position.x, position.y, 0.0f);
		Internal::AddCircle(translation, radius, s_Data->WhiteTexture, 1.0f, color, id);
	}

	void Renderer2D::DrawRotatedCircle(const DirectX::XMFLOAT2& position, float rotation, float radius, const Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tintcolor, int id)
	{
		const auto translation = DirectX::XMMatrixScaling(radius, radius, 1.0f) * DirectX::XMMatrixRotationZ(rotation) * DirectX::XMMatrixTranslation(position.x, position.y, 0.0f);
		Internal::AddCircle(translation, radius, texture, tilingfactor, tintcolor, id);
	}

	void Renderer2D::DrawEntity(Entity entity)
	{
		SK_CORE_ASSERT(entity.HasComponent<TransformComponent>(), "Tried to Draw Entity without Transform Component");
		SK_CORE_ASSERT(entity.HasComponent<SpriteRendererComponent>(), "Tried to Draw Entity without Sprite Renderer Component");

		auto& tf = entity.GetComponent<TransformComponent>();
		auto& sr = entity.GetComponent<SpriteRendererComponent>();

		Ref<Texture2D> texture = sr.Texture ? sr.Texture : s_Data->WhiteTexture;
		switch (sr.Geometry)
		{
			case Geometry::Quad:                                   Internal::AddQuad(tf.GetTranform(), texture, sr.TilingFactor, sr.Color, (int)(uint32_t)entity);                           break;
			case Geometry::Circle:   tf.Scaling.y = tf.Scaling.x;  Internal::AddCircle(tf.GetTranform(), tf.Scaling.x * 0.5f, texture, sr.TilingFactor, sr.Color, (int)(uint32_t)(entity));  break;
		}
	}

	Renderer2D::Statistics Renderer2D::GetStatistics()
	{
		return s_Data->Stats;
	}

}
