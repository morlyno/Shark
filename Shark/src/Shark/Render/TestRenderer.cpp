#include "skpch.h"
#include "TestRenderer.h"

#if 0
#include "Shark/Render/Renderer.h"

#include "Shark/Render/Buffers.h"
#include "Shark/Render/Shaders.h"
#include "Shark/Render/ConstantBuffer.h"

#include "Shark/Scene/Components/SpriteRendererComponent.h"
#include "Shark/Scene/Components/TransformComponent.h"

#include "Shark/Render/RendererCommand.h"
#include "Shark/Render/Material.h"

namespace Shark {

	struct RendererData
	{
		Ref<VertexBuffer> VertexBuffer;
		Ref<IndexBuffer> IndexBuffer;
		Ref<Shaders> Shaders;

		Ref<Material> Material;
		Ref<ConstantBuffer> ViewProjection;
		//Ref<ConstantBuffer> ViewProjection;
		//Ref<ConstantBuffer> Material;
	};

	static RendererData* s_Data;

	struct Vertex
	{
		DirectX::XMFLOAT3 Position;
		DirectX::XMFLOAT2 TexCoord;
	};

	void TestRenderer::Init()
	{
		s_Data = new RendererData;
		s_Data->Shaders = Renderer::GetShaderLib().Get("TestShader");
		//s_Data->Shaders = Shaders::Create("assets/Shaders/TestShader.hlsl");

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

		s_Data->VertexBuffer = VertexBuffer::Create(s_Data->Shaders->GetVertexLayout(), vertices, sizeof(vertices));
		s_Data->IndexBuffer = IndexBuffer::Create(indices, std::size(indices));
		
		s_Data->ViewProjection = s_Data->Shaders->CreateConstantBuffer("SceneData");

		//s_Data->ViewProjection = ConstantBuffer::Create(64, 0);
		//s_Data->Material = ConstantBuffer::Create(96, 1);
		s_Data->Material = Material::Create(s_Data->Shaders, "TestRendererMaterial");
		
	}

	void TestRenderer::ShutDown()
	{
		delete s_Data;
	}

	void TestRenderer::BeginScene(Camera& camera, const DirectX::XMMATRIX& view)
	{
		DirectX::XMMATRIX vp = view * camera.GetProjection();
		s_Data->ViewProjection->Set(&vp);
	}

	void TestRenderer::BeginScene(EditorCamera& camera)
	{
		DirectX::XMMATRIX vp = camera.GetViewProjection();
		s_Data->ViewProjection->Set(&vp);
	}

	void TestRenderer::EndScene()
	{
	}

	void TestRenderer::DrawRotatedQuad(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& rotation, const DirectX::XMFLOAT3& scaling, const DirectX::XMFLOAT4& color, int id)
	{
		DrawRotatedQuad(position, rotation, scaling, Renderer::GetWhiteTexture(), 1.0f, color, id);
	}

	void TestRenderer::DrawRotatedQuad(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& rotation, const DirectX::XMFLOAT3& scaling, const Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tintcolor, int id)
	{
		auto transform = DirectX::XMMatrixScaling(scaling.x, scaling.y, scaling.z) * DirectX::XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z) * DirectX::XMMatrixTranslation(position.x, position.y, position.z);
		s_Data->Material->Set("c_Transform", &transform, sizeof(DirectX::XMMATRIX));
		s_Data->Material->Set("c_Color", &tintcolor, sizeof(DirectX::XMFLOAT4));
		s_Data->Material->Set("c_ID", &id, sizeof(int));
		s_Data->Material->Set("in_Texture", texture);
		s_Data->Material->Set("c_TilingFactor", &tilingfactor, sizeof(float));
		DrawQuad(s_Data->Material);
	}

	void TestRenderer::DrawEntity(Entity entity)
	{
		const auto& tf = entity.GetComponent<TransformComponent>();
		auto material = entity.GetComponent<MaterialComponent>().Material;

		auto transform = tf.GetTranform();
		material->Set("c_Transform", &transform, sizeof(DirectX::XMMATRIX));
		material->Set("c_ID", &entity, sizeof(uint32_t));
		DrawQuad(material);

	}

	void TestRenderer::DrawQuad(const Ref<Material>& material)
	{
		s_Data->Shaders->Bind();
		s_Data->VertexBuffer->Bind();
		s_Data->IndexBuffer->Bind();
		s_Data->ViewProjection->Bind();
		material->Bind();

		RendererCommand::DrawIndexed(s_Data->IndexBuffer->GetCount());

	}

	Ref<Material> TestRenderer::GetMaterial()
	{
		return s_Data->Material;
	}

}
#endif