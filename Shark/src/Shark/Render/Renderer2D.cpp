#include "skpch.h"
#include "Renderer2D.h"
#include "Shark/Render/RendererCommand.h"

#include "Shark/Render/Buffers.h"
#include "Shark/Render/Shaders.h"
#include "Shark/Render/ConstantBuffer.h"

#include "Shark/Scean/Components/TransformComponent.h"
#include "Shark/Scean/Components/SpriteRendererComponent.h"

#include <DirectXMath.h>

namespace Shark {

	struct SceanData
	{
		DirectX::XMMATRIX ViewProjectionMatrix;
	};
	static SceanData s_SceanData;

	using Index = IndexBuffer::IndexType;

	struct Vertex
	{
		DirectX::XMFLOAT3 Pos;
		DirectX::XMFLOAT4 Color;
		DirectX::XMFLOAT2 Tex;
		int TextureIndex;
		float TilingFactor;
		int ID;
	};

	struct DrawCommand
	{
		static constexpr uint32_t MaxTextures = 16;

		uint32_t VertexOffset = 0;
		uint32_t IndexOffset = 0;

		uint32_t VertexCount = 0;
		uint32_t IndexCount = 0;

		uint32_t TextureCount = 0;
		uint32_t TextureOffset = 0;

		std::function<void()> Callback;
	};

	struct DrawData
	{
		Ref<Shaders> Shaders;
		Ref<ConstantBuffer> ConstantBuffer;
		Ref<VertexBuffer> VertexBuffer;
		Ref<IndexBuffer> IndexBuffer;
		Ref<Texture2D> WitheTexture;

		std::vector<Vertex> VertexBufferData;
		std::vector<Index> IndexBufferData;
		std::vector<Ref<Texture2D>> Textures;
		std::vector<DrawCommand> DrawCmdList;
		DrawCommand* Cmd;

		Renderer2D::Statistics Stats;
	};

	static DrawData s_DrawData;

	namespace Utils {

		static void ResetStates()
		{
			auto& s = s_DrawData.Stats;
			s.DrawCalls = 0;
			s.DrawCommands = 0;
			s.ElementCount = 0;
			s.VertexCount = 0;
			s.IndexCount = 0;
			s.TextureCount = 0;
			s.Callbacks = 0;
		}

		static void Flush()
		{
			uint32_t totalvertices = s_DrawData.Cmd->VertexCount + s_DrawData.Cmd->VertexOffset;
			uint32_t totalindices = s_DrawData.Cmd->IndexCount + s_DrawData.Cmd->IndexOffset;

			if (totalindices == 0 || totalvertices == 0)
				return;

			if (totalvertices * sizeof(Vertex) > s_DrawData.VertexBuffer->GetSize())
				s_DrawData.VertexBuffer = VertexBuffer::Create(s_DrawData.Shaders->GetVertexLayout(), nullptr, 0, true);
			if (totalindices * sizeof(Index) > s_DrawData.IndexBuffer->GetSize())
				s_DrawData.IndexBuffer = IndexBuffer::Create(nullptr, 0, true);

			s_DrawData.VertexBuffer->SetData(s_DrawData.VertexBufferData.data(), s_DrawData.VertexBufferData.size() * sizeof(Vertex));
			s_DrawData.IndexBuffer->SetData(s_DrawData.IndexBufferData.data(), s_DrawData.IndexBufferData.size());

			s_DrawData.Shaders->Bind();
			s_DrawData.VertexBuffer->Bind();
			s_DrawData.IndexBuffer->Bind();
			s_DrawData.ConstantBuffer->Bind();

			s_DrawData.ConstantBuffer->Set(&s_SceanData);

			for (uint32_t i = 0; i < s_DrawData.DrawCmdList.size(); i++)
			{
				DrawCommand& cmd = s_DrawData.DrawCmdList[i];

				if (cmd.Callback)
				{
					cmd.Callback();
					s_DrawData.Stats.Callbacks++;
				}

				for (uint32_t i = 0; i < cmd.TextureCount; i++)
					s_DrawData.Textures[i + cmd.TextureOffset]->Bind();

				RendererCommand::DrawIndexed(cmd.IndexCount, cmd.IndexOffset, 0);
				s_DrawData.Stats.DrawCalls++;
			}
			
			s_DrawData.Stats.DrawCommands += s_DrawData.DrawCmdList.size();
			s_DrawData.Stats.VertexCount += s_DrawData.VertexBufferData.size();
			s_DrawData.Stats.IndexCount += s_DrawData.IndexBufferData.size();
			s_DrawData.Stats.TextureCount += s_DrawData.Textures.size();

			s_DrawData.VertexBufferData.clear();
			s_DrawData.IndexBufferData.clear();
			s_DrawData.Textures.clear();

			s_DrawData.DrawCmdList.clear();

			s_DrawData.DrawCmdList.emplace_back();
			s_DrawData.Cmd = &s_DrawData.DrawCmdList.back();

		}

		static void BeginNewDrawCommand()
		{
			s_DrawData.DrawCmdList.emplace_back();
			auto* oldcmd = &s_DrawData.DrawCmdList[s_DrawData.DrawCmdList.size() - 2];

			s_DrawData.Cmd = &s_DrawData.DrawCmdList.back();
			s_DrawData.Cmd->VertexOffset = oldcmd->VertexCount + oldcmd->VertexOffset;
			s_DrawData.Cmd->IndexOffset = oldcmd->IndexCount + oldcmd->IndexOffset;
			s_DrawData.Cmd->TextureOffset = oldcmd->TextureCount + oldcmd->TextureOffset;
		}

		static void AddTexture(const Ref<Texture2D>& texture)
		{
			if (s_DrawData.Cmd->TextureCount >= DrawCommand::MaxTextures)
				BeginNewDrawCommand();

			bool found = false;
			for (uint32_t i = 0; i < s_DrawData.Cmd->TextureCount; i++)
			{
				if (texture == s_DrawData.Textures[i + s_DrawData.Cmd->TextureOffset])
				{
					found = true;
					texture->SetSlot(i);
					break;
				}
			}

			if (!found)
			{
				s_DrawData.Textures.emplace_back(texture);
				texture->SetSlot(s_DrawData.Cmd->TextureCount++);
			}
		}

		static void AddQuad(const DirectX::XMMATRIX& translation, const Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tintcolor, int id)
		{
			constexpr DirectX::XMFLOAT3 Vertices[4] = { { -0.5f, 0.5f, 0.0f }, { 0.5f, 0.5f, 0.0f }, { 0.5f, -0.5f, 0.0f }, { -0.5f, -0.5f, 0.0f } };
			constexpr DirectX::XMFLOAT2 TexCoords[4] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

			DrawCommand* cmd = s_DrawData.Cmd;

			for (uint32_t i = 0; i < 4; i++)
			{
				auto& vtx = s_DrawData.VertexBufferData.emplace_back();
				DirectX::XMStoreFloat3(&vtx.Pos, DirectX::XMVector3Transform(DirectX::XMLoadFloat3(&Vertices[i]), translation));
				vtx.Color = tintcolor;
				vtx.Tex = TexCoords[i];
				vtx.TextureIndex = texture->GetSlot();
				vtx.TilingFactor = tilingfactor;
				vtx.ID = id;
			}

			s_DrawData.IndexBufferData.emplace_back(0 + cmd->VertexCount + cmd->VertexOffset);
			s_DrawData.IndexBufferData.emplace_back(1 + cmd->VertexCount + cmd->VertexOffset);
			s_DrawData.IndexBufferData.emplace_back(2 + cmd->VertexCount + cmd->VertexOffset);

			s_DrawData.IndexBufferData.emplace_back(2 + cmd->VertexCount + cmd->VertexOffset);
			s_DrawData.IndexBufferData.emplace_back(3 + cmd->VertexCount + cmd->VertexOffset);
			s_DrawData.IndexBufferData.emplace_back(0 + cmd->VertexCount + cmd->VertexOffset);


			cmd->VertexCount += 4;
			cmd->IndexCount += 6;

			s_DrawData.Stats.ElementCount++;
		}

	}

	void Renderer2D::Init()
	{
		s_DrawData.Shaders = Shaders::Create("assets/Shaders/MainShader.hlsl");
		s_DrawData.ConstantBuffer = ConstantBuffer::Create(64, 0);
		s_DrawData.VertexBuffer = VertexBuffer::Create(s_DrawData.Shaders->GetVertexLayout(), nullptr, 0, true);
		s_DrawData.IndexBuffer = IndexBuffer::Create(nullptr, 0, true);
		uint32_t textureColor = 0xFFFFFFFF;
		s_DrawData.WitheTexture = Texture2D::Create({}, 1, 1, &textureColor);

		s_DrawData.VertexBufferData.reserve(1000 * 4);
		s_DrawData.IndexBufferData.reserve(1000 * 6);
		s_DrawData.Textures.reserve(16);

		s_DrawData.DrawCmdList.emplace_back();
		s_DrawData.Cmd = &s_DrawData.DrawCmdList.back();

#ifdef SK_DEBUG
		for (uint32_t i = 0; i < DrawCommand::MaxTextures; i++)
			s_DrawData.WitheTexture->Bind(i);
#endif
	}

	void Renderer2D::ShutDown()
	{
		s_DrawData.WitheTexture.Release();
		s_DrawData.IndexBuffer.Release();
		s_DrawData.VertexBuffer.Release();
		s_DrawData.Shaders.Release();
		s_DrawData.ConstantBuffer.Release();
	}

	void Renderer2D::BeginScean(Camera& camera, const DirectX::XMMATRIX& view)
	{
		Utils::ResetStates();
		s_SceanData.ViewProjectionMatrix = view * camera.GetProjection();
	}

	void Renderer2D::BeginScean(EditorCamera& camera)
	{
		Utils::ResetStates();
		s_SceanData.ViewProjectionMatrix = camera.GetViewProjection();
	}

	void Renderer2D::EndScean()
	{
		Utils::Flush();
	}

	void DrawQuad(const DirectX::XMFLOAT2& position, const DirectX::XMFLOAT2& scaling, const DirectX::XMFLOAT4& color, int id)
	{
		const auto translation = DirectX::XMMatrixScaling(scaling.x, scaling.y, 1.0f) * DirectX::XMMatrixTranslation(position.x, position.y, 0.0f);
		Utils::AddQuad(translation, s_DrawData.WitheTexture, 1.0f, color, id);
	}

	void DrawQuad(const DirectX::XMFLOAT2& position, const DirectX::XMFLOAT2& scaling, const Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tintcolor, int id)
	{
		const auto translation = DirectX::XMMatrixScaling(scaling.x, scaling.y, 1.0f) * DirectX::XMMatrixTranslation(position.x, position.y, 0.0f);
		Utils::AddTexture(texture);
		Utils::AddQuad(translation, texture, tilingfactor, tintcolor, id);
	}

	void Renderer2D::DrawRotatedQuad(const DirectX::XMFLOAT2& position, float rotation, const DirectX::XMFLOAT2& scaling, const DirectX::XMFLOAT4& color, int id)
	{
		const auto translation = DirectX::XMMatrixScaling(scaling.x, scaling.y, 1.0f) * DirectX::XMMatrixRotationZ(rotation) * DirectX::XMMatrixTranslation(position.x, position.y, 0.0f);
		Utils::AddQuad(translation, s_DrawData.WitheTexture, 1.0f, color, id);
	}

	void Renderer2D::DrawRotatedQuad(const DirectX::XMFLOAT2& position, float rotation, const DirectX::XMFLOAT2& scaling, const Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tintcolor, int id)
	{
		const auto translation = DirectX::XMMatrixScaling(scaling.x, scaling.y, 1.0f) * DirectX::XMMatrixRotationZ(rotation) * DirectX::XMMatrixTranslation(position.x, position.y, 0.0f);
		Utils::AddTexture(texture);
		Utils::AddQuad(translation, texture, tilingfactor, tintcolor, id);
	}

	void Renderer2D::AddCallbackFunction(const std::function<void()>& func)
	{
		Utils::BeginNewDrawCommand();
		s_DrawData.Cmd->Callback = func;
	}

	void Renderer2D::DrawEntity(Entity entity)
	{
		SK_CORE_ASSERT(entity.HasComponent<TransformComponent>(), "Tried to Draw Entity without Transform Component");
		SK_CORE_ASSERT(entity.HasComponent<SpriteRendererComponent>(), "Tried to Draw Entity without Sprite Renderer Component");

		auto& tc = entity.GetComponent<TransformComponent>();
		auto& src = entity.GetComponent<SpriteRendererComponent>();

		Ref<Texture2D> texture = src.Texture ? src.Texture : s_DrawData.WitheTexture;
		Utils::AddTexture(texture);
		Utils::AddQuad(tc.GetTranform(), texture, src.TilingFactor, src.Color, (int)(uint32_t)entity);
	}

	void Renderer2D::DrawTransform(const TransformComponent& transform, const DirectX::XMFLOAT4& color, int id)
	{
		Utils::AddTexture(s_DrawData.WitheTexture);
		Utils::AddQuad(transform.GetTranform(), s_DrawData.WitheTexture, 1.0f, color, id);
	}

	Renderer2D::Statistics Renderer2D::GetStatistics()
	{
		return s_DrawData.Stats;
	}

}
