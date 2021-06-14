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

namespace Shark {

	struct SceneData
	{
		DirectX::XMMATRIX ViewProjectionMatrix;
	};
	static SceneData s_SceneData;

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

		std::vector<Vertex> VertexBase;
		std::vector<Index> IndexBase;
		uint32_t TextureBaseIndex = 0;

		uint32_t VertexCount = 0;
		uint32_t IndexCount = 0;
		uint32_t TextureCount = 0;

		bool IsLocked = false;

		Ref<Material> Material;
	};

	struct DrawData
	{
		Ref<ConstantBuffer> ViewProjection;
		Ref<VertexBuffer> VertexBuffer;
		Ref<IndexBuffer> IndexBuffer;

		uint32_t TotalVertexCount = 0;
		uint32_t TotalIndexCount = 0;
		std::vector<Ref<Texture2D>> Textures;

		std::vector<DrawCommand> DrawCmdList;
		uint32_t ActiveCommands = 0;
		std::unordered_map<std::string, Ref<Material>> MaterialMap;
		DrawCommand* SelectedCommand = nullptr;

		Renderer2D::Statistics Stats;
	};

	static DrawData* s_DrawData;

	namespace Utils {

		static void ResetStates()
		{
			auto& s = s_DrawData->Stats;
			s.DrawCalls = 0;
			s.DrawCommands = 0;
			s.ElementCount = 0;
			s.VertexCount = 0;
			s.IndexCount = 0;
			s.TextureCount = 0;
			s.Callbacks = 0;
		}

		static DrawCommand* AddDrawCommand(Ref<Material> material)
		{
			if (s_DrawData->ActiveCommands >= s_DrawData->DrawCmdList.size())
				s_DrawData->DrawCmdList.emplace_back();
			DrawCommand* cmd = &s_DrawData->DrawCmdList[s_DrawData->ActiveCommands];
			cmd->VertexBase.reserve(10000 * 4);
			cmd->IndexBase.reserve(10000 * 6);
			cmd->TextureBaseIndex = s_DrawData->ActiveCommands * DrawCommand::MaxTextures;
			cmd->Material = material;
			s_DrawData->ActiveCommands++;
			return cmd;
		}

		static void SelectDrawCommand(const Ref<Material>& material)
		{
			s_DrawData->SelectedCommand = nullptr;
			for (auto& cmd : s_DrawData->DrawCmdList)
			{
				if (cmd.Material == material)
				{
					s_DrawData->SelectedCommand = &cmd;
					return;
				}
			}

			auto& map = s_DrawData->MaterialMap;
			if (auto&& it = map.find(material->GetName()); it == map.end())
			{
				map.insert({ material->GetName(), material });
				SK_CORE_INFO("New Material Added: {0}", material->GetName());
			}

			s_DrawData->SelectedCommand = AddDrawCommand(material);
		}

		static void Flush()
		{
			if (s_DrawData->TotalVertexCount == 0 || s_DrawData->TotalIndexCount == 0)
				return;

			if (s_DrawData->TotalVertexCount * sizeof(Vertex) > s_DrawData->VertexBuffer->GetSize())
				s_DrawData->VertexBuffer->Resize(s_DrawData->TotalVertexCount * sizeof(Vertex));
			if (s_DrawData->TotalIndexCount * sizeof(Index) > s_DrawData->IndexBuffer->GetSize())
				s_DrawData->IndexBuffer->Resize(s_DrawData->TotalIndexCount);

			{
				byte* vtx = (byte*)s_DrawData->VertexBuffer->Map();
				byte* idx = (byte*)s_DrawData->IndexBuffer->Map();

				uint32_t vtxOffset = 0;
				uint32_t idxOffset = 0;
				for (auto& cmd : s_DrawData->DrawCmdList)
				{
					memcpy(vtx + vtxOffset, cmd.VertexBase.data(), cmd.VertexCount * sizeof(Vertex));
					memcpy(idx + idxOffset, cmd.IndexBase.data(), cmd.IndexCount * sizeof(Index));
					vtxOffset += cmd.VertexCount * sizeof(Vertex);
					idxOffset += cmd.IndexCount * sizeof(Index);
				}

				s_DrawData->VertexBuffer->UnMap();
				s_DrawData->IndexBuffer->UnMap();
			}

			s_DrawData->VertexBuffer->Bind();
			s_DrawData->IndexBuffer->Bind();
			s_DrawData->ViewProjection->Bind();

			s_DrawData->ViewProjection->Set(&s_SceneData);

			uint32_t vtxOffset = 0;
			uint32_t idxOffset = 0;

			auto& cmdList = s_DrawData->DrawCmdList;
			for (uint32_t i = 0; i < s_DrawData->DrawCmdList.size(); i++)
			{
				DrawCommand* cmd = &s_DrawData->DrawCmdList[i];
				cmd->Material->GetShaders()->Bind();

				for (uint32_t i = 0; i < cmd->TextureCount; i++)
					s_DrawData->Textures[i + cmd->TextureBaseIndex]->Bind();

				RendererCommand::DrawIndexed(cmd->IndexCount, idxOffset, vtxOffset);
				idxOffset += cmd->IndexCount;
				vtxOffset += cmd->VertexCount;

				s_DrawData->Stats.DrawCalls++;
			}
			
			s_DrawData->Stats.DrawCommands += s_DrawData->DrawCmdList.size();
			s_DrawData->Stats.VertexCount += s_DrawData->TotalVertexCount;
			s_DrawData->Stats.IndexCount += s_DrawData->TotalIndexCount;
			s_DrawData->Stats.TextureCount += s_DrawData->Textures.size();

			for (auto& cmd : s_DrawData->DrawCmdList)
			{
				cmd.VertexBase.clear();
				cmd.IndexBase.clear();
				cmd.TextureBaseIndex = 0;
				cmd.VertexCount = 0;
				cmd.IndexCount = 0;
				cmd.TextureCount = 0;
				cmd.IsLocked = false;
				cmd.Material = nullptr;
			}
			s_DrawData->ActiveCommands = 0;
			s_DrawData->TotalVertexCount = 0;
			s_DrawData->TotalIndexCount = 0;
			for (auto& tex : s_DrawData->Textures)
				tex = nullptr;

		}

		static void AddTexture(const Ref<Texture2D>& texture)
		{
			DrawCommand* cmd = s_DrawData->SelectedCommand;

			for (uint32_t i = 0; i < cmd->TextureCount; i++)
			{
				if (texture == s_DrawData->Textures[i + cmd->TextureBaseIndex])
				{
					texture->SetSlot(i);
					return;
				}
			}

			if (cmd->TextureCount >= DrawCommand::MaxTextures)
			{
				s_DrawData->SelectedCommand->IsLocked = true;
				cmd = AddDrawCommand(cmd->Material);
				s_DrawData->SelectedCommand = cmd;
			}

			uint32_t index = cmd->TextureBaseIndex + cmd->TextureCount;
			if (index >= s_DrawData->Textures.size())
				s_DrawData->Textures.resize(index + 1);
			s_DrawData->Textures[index] = texture;
			texture->SetSlot(cmd->TextureCount++);
		}

		static void AddQuad(const DirectX::XMMATRIX& translation, const Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tintcolor, int id)
		{
			constexpr DirectX::XMFLOAT3 Vertices[4] = { { -0.5f, 0.5f, 0.0f }, { 0.5f, 0.5f, 0.0f }, { 0.5f, -0.5f, 0.0f }, { -0.5f, -0.5f, 0.0f } };
			constexpr DirectX::XMFLOAT2 TexCoords[4] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

			DrawCommand* cmd = s_DrawData->SelectedCommand;
			cmd->VertexBase.resize(cmd->VertexCount + 4);
			cmd->IndexBase.resize(cmd->IndexCount + 6);

			for (uint32_t i = 0; i < 4; i++)
			{
				Vertex* vtx = cmd->VertexBase.data() + cmd->VertexCount + i;
				DirectX::XMStoreFloat3(&vtx->Pos, DirectX::XMVector3Transform(DirectX::XMLoadFloat3(&Vertices[i]), translation));
				vtx->Color = tintcolor;
				vtx->Tex = TexCoords[i];
				vtx->TextureIndex = texture->GetSlot();
				vtx->TilingFactor = tilingfactor;
				vtx->ID = id;
			}

			Index* idx = cmd->IndexBase.data() + cmd->IndexCount;
			idx[0] = 0 + cmd->VertexCount;
			idx[1] = 1 + cmd->VertexCount;
			idx[2] = 2 + cmd->VertexCount;

			idx[3] = 2 + cmd->VertexCount;
			idx[4] = 3 + cmd->VertexCount;
			idx[5] = 0 + cmd->VertexCount;


			cmd->VertexCount += 4;
			cmd->IndexCount += 6;
			s_DrawData->TotalVertexCount += 4;
			s_DrawData->TotalIndexCount += 6;

			s_DrawData->Stats.ElementCount++;
		}

	}

	void Renderer2D::Init()
	{
		s_DrawData = new DrawData;
		s_DrawData->ViewProjection = ConstantBuffer::Create(64, 0);
		s_DrawData->VertexBuffer = VertexBuffer::Create(Renderer::GetDefault2DShader()->GetVertexLayout(), nullptr, 0, true);
		s_DrawData->IndexBuffer = IndexBuffer::Create(nullptr, 0, true);
		

#ifdef SK_DEBUG
		auto texture = Renderer::GetWhiteTexture();
		for (uint32_t i = 0; i < DrawCommand::MaxTextures; i++)
			texture->Bind(i);
#endif
	}

	void Renderer2D::ShutDown()
	{
		delete s_DrawData;
	}

	void Renderer2D::BeginScene(Camera& camera, const DirectX::XMMATRIX& view)
	{
		Utils::ResetStates();
		s_SceneData.ViewProjectionMatrix = view * camera.GetProjection();
	}

	void Renderer2D::BeginScene(EditorCamera& camera)
	{
		Utils::ResetStates();
		s_SceneData.ViewProjectionMatrix = camera.GetViewProjection();
	}

	void Renderer2D::EndScene()
	{
		Utils::Flush();
	}

	void Renderer2D::DrawQuad(const DirectX::XMFLOAT2& position, const DirectX::XMFLOAT2& scaling, const DirectX::XMFLOAT4& color, int id)
	{
		const auto translation = DirectX::XMMatrixScaling(scaling.x, scaling.y, 1.0f) * DirectX::XMMatrixTranslation(position.x, position.y, 0.0f);
		Utils::SelectDrawCommand(Renderer::GetDefault2DMaterial());
		Utils::AddQuad(translation, Renderer::GetWhiteTexture(), 1.0f, color, id);
	}

	void Renderer2D::DrawQuad(const DirectX::XMFLOAT2& position, const DirectX::XMFLOAT2& scaling, const Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tintcolor, int id)
	{
		const auto translation = DirectX::XMMatrixScaling(scaling.x, scaling.y, 1.0f) * DirectX::XMMatrixTranslation(position.x, position.y, 0.0f);
		Utils::SelectDrawCommand(Renderer::GetDefault2DMaterial());
		Utils::AddTexture(texture);
		Utils::AddQuad(translation, texture, tilingfactor, tintcolor, id);
	}

	void Renderer2D::DrawRotatedQuad(const DirectX::XMFLOAT2& position, float rotation, const DirectX::XMFLOAT2& scaling, const DirectX::XMFLOAT4& color, int id)
	{
		const auto translation = DirectX::XMMatrixScaling(scaling.x, scaling.y, 1.0f) * DirectX::XMMatrixRotationZ(rotation) * DirectX::XMMatrixTranslation(position.x, position.y, 0.0f);
		Utils::SelectDrawCommand(Renderer::GetDefault2DMaterial());
		Utils::AddQuad(translation, Renderer::GetWhiteTexture(), 1.0f, color, id);
	}

	void Renderer2D::DrawRotatedQuad(const DirectX::XMFLOAT2& position, float rotation, const DirectX::XMFLOAT2& scaling, const Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tintcolor, int id)
	{
		const auto translation = DirectX::XMMatrixScaling(scaling.x, scaling.y, 1.0f) * DirectX::XMMatrixRotationZ(rotation) * DirectX::XMMatrixTranslation(position.x, position.y, 0.0f);
		Utils::SelectDrawCommand(Renderer::GetDefault2DMaterial());
		Utils::AddTexture(texture);
		Utils::AddQuad(translation, texture, tilingfactor, tintcolor, id);
	}

	void Renderer2D::DrawEntity(Entity entity)
	{
		auto tag = entity.GetComponent<TagComponent>().Tag;

		SK_CORE_ASSERT(entity.HasComponent<TransformComponent>(), "Tried to Draw Entity without Transform Component");
		SK_CORE_ASSERT(entity.HasComponent<SpriteRendererComponent>(), "Tried to Draw Entity without Sprite Renderer Component");

		auto& tf = entity.GetComponent<TransformComponent>();
		auto& sr = entity.GetComponent<SpriteRendererComponent>();

		SK_CORE_ASSERT(sr.Material);

		Ref<Texture2D> texture = sr.Texture ? sr.Texture : Renderer::GetWhiteTexture();
		Utils::SelectDrawCommand(sr.Material);
		Utils::AddTexture(texture);
		Utils::AddQuad(tf.GetTranform(), texture, sr.TilingFactor, sr.Color, (int)(uint32_t)entity);
	}

	void Renderer2D::DrawTransform(const TransformComponent& transform, const DirectX::XMFLOAT4& color, int id)
	{
		Utils::SelectDrawCommand(Renderer::GetDefault2DMaterial());
		Utils::AddTexture(Renderer::GetWhiteTexture());
		Utils::AddQuad(transform.GetTranform(), Renderer::GetWhiteTexture(), 1.0f, color, id);
	}

	Ref<Material> Renderer2D::TryGetMaterial(const std::string& name)
	{
		const auto i = s_DrawData->MaterialMap.find(name);
		if (i == s_DrawData->MaterialMap.end())
			return nullptr;
		return i->second;
	}

	const std::unordered_map<std::string, Ref<Material>>& Renderer2D::GetMaterialMap()
	{
		return s_DrawData->MaterialMap;
	}

	Renderer2D::Statistics Renderer2D::GetStatistics()
	{
		return s_DrawData->Stats;
	}

}
