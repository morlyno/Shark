#include "skpch.h"
#include "Renderer2D.h"
#include "Shark/Render/RendererCommand.h"

#include "Shark/Render/Buffers.h"
#include "Shark/Render/Shaders.h"

#include <DirectXMath.h>

namespace Shark {

	struct RendererData
	{
		Ref<VertexBuffer> VertexBuffer;
		Ref<IndexBuffer> IndexBuffer;
		Ref<Shaders> Shaders;

		Ref<Texture2D> WhiteTexture;
	};

	struct SceanData
	{
		DirectX::XMMATRIX ViewProjectionMatrix;
	};

	static Scope<RendererData> s_Data;
	static Scope<SceanData> s_SceanData;

	struct Vertex
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT2 tex;
	};

	void Renderer2D::Init()
	{
		SK_CORE_ASSERT(s_SceanData == nullptr, "SceanData already set");
		SK_CORE_ASSERT(s_Data == nullptr, "SceanData already set");

		s_SceanData = CreateScope<SceanData>();
		s_Data = CreateScope<RendererData>();

		s_Data->Shaders = ShaderLib::Load("assets/Shaders/MainShader.hlsl");

		s_Data->WhiteTexture = Texture2D::Create(1, 1, 0xFFFFFFFF);

		Vertex vertices[4] =
		{
			{ { -0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f } },
			{ { -0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f } },
			{ {  0.5f,  0.5f, 0.0f }, { 1.0f, 0.0f } },
			{ {  0.5f, -0.5f, 0.0f }, { 1.0f, 1.0f } }
		};
		uint32_t indices[6] = { 0,1,2, 2,3,0 };

		s_Data->VertexBuffer = VertexBuffer::Create(s_Data->Shaders->GetVertexLayout(), vertices, (uint32_t)sizeof(vertices));
		s_Data->IndexBuffer = IndexBuffer::Create(indices, (uint32_t)std::size(indices));
	}

	void Renderer2D::ShutDown()
	{
	}

	void Renderer2D::BeginScean(OrtographicCamera& camera)
	{
		s_SceanData->ViewProjectionMatrix = camera.GetViewProjection();
		s_Data->Shaders->Bind();
		s_Data->VertexBuffer->Bind();
		s_Data->IndexBuffer->Bind();
		s_Data->Shaders->SetBuffer("SceanData", s_SceanData.get(), sizeof(SceanData));
	}

	void Renderer2D::EndScean()
	{
	}

	void Renderer2D::DrawQuad(const DirectX::XMFLOAT2& pos, const DirectX::XMFLOAT2& scaling, const DirectX::XMFLOAT4& color)
	{
		DrawQuad({ pos.x, pos.y, 0.0f }, scaling, color);
	}

	void Renderer2D::DrawQuad(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT2& scaling, const DirectX::XMFLOAT4& color)
	{
		struct { DirectX::XMMATRIX Translation; DirectX::XMFLOAT4 Color; } objectData;
		objectData.Color = color;
		objectData.Translation = DirectX::XMMatrixScaling(scaling.x, scaling.y, 1.0f) * DirectX::XMMatrixTranslation(pos.x, pos.y, pos.z);

		s_Data->Shaders->SetBuffer("ObjectData", &objectData, sizeof(objectData));
		s_Data->WhiteTexture->Bind();

		RendererCommand::DrawIndexed(6);
	}

	void Renderer2D::DrawQuad(const DirectX::XMFLOAT2& pos, const DirectX::XMFLOAT2& scaling, Ref<Texture2D>& texture, const DirectX::XMFLOAT4& color)
	{
		DrawQuad({ pos.x, pos.y, 0.0f }, scaling, texture, color);
	}

	void Renderer2D::DrawQuad(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT2& scaling, Ref<Texture2D>& texture, const DirectX::XMFLOAT4& color)
	{
		struct { DirectX::XMMATRIX Translation; DirectX::XMFLOAT4 Color; } objectData;
		objectData.Color = color;
		objectData.Translation = DirectX::XMMatrixScaling(scaling.x, scaling.y, 1.0f) * DirectX::XMMatrixTranslation(pos.x, pos.y, pos.z);

		s_Data->Shaders->SetBuffer("ObjectData", &objectData, sizeof(objectData));
		texture->Bind();

		RendererCommand::DrawIndexed(6);
	}

	void Renderer2D::DrawRotatedQuad(const DirectX::XMFLOAT2& pos, float rotation, const DirectX::XMFLOAT2& scaling, const DirectX::XMFLOAT4& color)
	{
		DrawRotatedQuad({ pos.x, pos.y, 0.0f }, rotation, scaling, color);
	}

	void Renderer2D::DrawRotatedQuad(const DirectX::XMFLOAT3& pos, float rotation, const DirectX::XMFLOAT2& scaling, const DirectX::XMFLOAT4& color)
	{
		struct { DirectX::XMMATRIX Translation; DirectX::XMFLOAT4 Color; } objectData;
		objectData.Color = color;
		objectData.Translation = DirectX::XMMatrixScaling(scaling.x, scaling.y, 1.0f) * DirectX::XMMatrixRotationZ(rotation) * DirectX::XMMatrixTranslation(pos.x, pos.y, pos.z);

		s_Data->Shaders->SetBuffer("ObjectData", &objectData, sizeof(objectData));
		s_Data->WhiteTexture->Bind();

		RendererCommand::DrawIndexed(6);
	}

	void Renderer2D::DrawRotatedQuad(const DirectX::XMFLOAT2& pos, float rotation, const DirectX::XMFLOAT2& scaling, Ref<Texture2D>& texture, const DirectX::XMFLOAT4& color)
	{
		DrawRotatedQuad({ pos.x, pos.y, 0.0f }, rotation, scaling, texture, color);
	}

	void Renderer2D::DrawRotatedQuad(const DirectX::XMFLOAT3& pos, float rotation, const DirectX::XMFLOAT2& scaling, Ref<Texture2D>& texture, const DirectX::XMFLOAT4& color)
	{
		struct { DirectX::XMMATRIX Translation; DirectX::XMFLOAT4 Color; } objectData;
		objectData.Color = color;
		objectData.Translation = DirectX::XMMatrixScaling(scaling.x, scaling.y, 1.0f) * DirectX::XMMatrixRotationZ(rotation) * DirectX::XMMatrixTranslation(pos.x, pos.y, pos.z);

		s_Data->Shaders->SetBuffer("ObjectData", &objectData, sizeof(objectData));
		texture->Bind();

		RendererCommand::DrawIndexed(6);
	}

}
