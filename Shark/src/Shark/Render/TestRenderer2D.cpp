#include "skpch.h"
#include "TestRenderer2D.h"

#include "Shark/Render/Renderer.h"

#include "Shark/Render/Buffers.h"
#include "Shark/Render/Shaders.h"
#include "Shark/Render/ConstantBuffer.h"

#include "Shark/Scean/Components/SpriteRendererComponent.h"
#include "Shark/Scean/Components/TransformComponent.h"

#include "Shark/Render/RendererCommand.h"

namespace Shark {

	Ref<Texture2D> TestRenderer::BaseTexture = nullptr;

	struct RendererData
	{
		Ref<VertexBuffer> VertexBuffer;
		Ref<IndexBuffer> IndexBuffer;
		Ref<Shaders> Shaders;

		Ref<ConstantBuffer> ViewProjection;
		Ref<ConstantBuffer> Material;
	};

	static RendererData s_Data;

	struct Vertex
	{
		DirectX::XMFLOAT3 Position;
		DirectX::XMFLOAT2 TexCoord;
	};

	struct MaterialConstantBuffer
	{
		DirectX::XMMATRIX Transform;
		DirectX::XMFLOAT4 Color;
		float TilingFactor;
		int ID;
	};

	void TestRenderer::Init()
	{
		s_Data.Shaders = Renderer::ShaderLib().Load("assets/Shaders/TestShader.hlsl");
		//s_Data.Shaders = Shaders::Create("assets/Shaders/TestShader.hlsl");

		Vertex vertices[4];
		vertices[0].Position = { -0.5f, +0.5f, 0.0f };
		vertices[1].Position = { +0.5f, +0.5f, 0.0f };
		vertices[2].Position = { +0.5f, -0.5f, 0.0f };
		vertices[3].Position = { -0.5f, -0.5f, 0.0f };

		vertices[0].TexCoord = { 0.0f, 0.0f };
		vertices[1].TexCoord = { 1.0f, 0.0f };
		vertices[2].TexCoord = { 1.0f, 1.0f };
		vertices[3].TexCoord = { 0.0f, 1.0f };

		IndexBuffer::IndexType indices[6] = { 0,1,2, 2,3,0 };

		s_Data.VertexBuffer = VertexBuffer::Create(s_Data.Shaders->GetVertexLayout(), vertices, sizeof(vertices));
		s_Data.IndexBuffer = IndexBuffer::Create(indices, std::size(indices));
		
		s_Data.ViewProjection = ConstantBuffer::Create(64, 0);
		s_Data.Material = ConstantBuffer::Create(96, 1);
		
		uint32_t color = 0xFFFFFFFF;
		BaseTexture = Texture2D::Create(1, 1, &color);

	}

	void TestRenderer::ShutDown()
	{
		s_Data.Shaders.Release();
		s_Data.VertexBuffer.Release();
		s_Data.IndexBuffer.Release();
		s_Data.ViewProjection.Release();
		s_Data.Material.Release();
		BaseTexture.Release();
	}

	void TestRenderer::BeginScean(Camera& camera, const DirectX::XMMATRIX& view)
	{
		DirectX::XMMATRIX vp = view * camera.GetProjection();
		s_Data.ViewProjection->Set((void*)&vp);
	}

	void TestRenderer::BeginScean(EditorCamera& camera)
	{
		auto vp = camera.GetViewProjection();
		s_Data.ViewProjection->Set(&vp);
	}

	void TestRenderer::EndScean()
	{
	}

	void TestRenderer::DrawRotatedQuad(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& rotation, const DirectX::XMFLOAT3& scaling, const DirectX::XMFLOAT4& color, int id)
	{
		auto transform = DirectX::XMMatrixScaling(scaling.x, scaling.y, scaling.z) * DirectX::XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z) * DirectX::XMMatrixTranslation(position.x, position.y, position.z);
		DrawQuad(transform, BaseTexture, 1.0f, color, id);
	}

	void TestRenderer::DrawRotatedQuad(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& rotation, const DirectX::XMFLOAT3& scaling, const Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tintcolor, int id)
	{
		auto transform = DirectX::XMMatrixScaling(scaling.x, scaling.y, scaling.z) * DirectX::XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z) * DirectX::XMMatrixTranslation(position.x, position.y, position.z);
		DrawQuad(transform, texture, tilingfactor, tintcolor, id);
	}

	void TestRenderer::DrawEntity(Entity entity)
	{
		const auto& sr = entity.GetComponent<SpriteRendererComponent>();
		const auto& tf = entity.GetComponent<TransformComponent>();
		DrawQuad(tf.GetTranform(), sr.Texture ? sr.Texture : BaseTexture, sr.TilingFactor, sr.Color, (int)(uint32_t)entity);
	}

	void TestRenderer::DrawQuad(const DirectX::XMMATRIX& transform, const Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tintcolor, int id)
	{
		s_Data.Shaders->Bind();
		s_Data.VertexBuffer->Bind();
		s_Data.IndexBuffer->Bind();
		s_Data.ViewProjection->Bind();
		MaterialConstantBuffer material;
		material.Transform = transform;
		material.TilingFactor = tilingfactor;
		material.Color = tintcolor;
		material.ID = id;
		s_Data.Material->Set(&material);
		s_Data.Material->Bind();
		texture->Bind(0);

		RendererCommand::DrawIndexed(s_Data.IndexBuffer->GetCount());

	}

}