#include "skpch.h"
#include "Renderer2D.h"
#include "Shark/Render/RendererCommand.h"

#include "Shark/Render/Buffers.h"
#include "Shark/Render/Shaders.h"

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

			s_DrawData.Shaders->SetBuffer("SceanData", &s_SceanData);

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
		s_DrawData.VertexBuffer = VertexBuffer::Create(s_DrawData.Shaders->GetVertexLayout(), nullptr, 0, true);
		s_DrawData.IndexBuffer = IndexBuffer::Create(nullptr, 0, true);
		s_DrawData.WitheTexture = Texture2D::Create({}, 1, 1, 0xFFFFFFFF);

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
		Utils::AddQuad(transform.GetTranform(), s_DrawData.WitheTexture, 1.0f, color, id);
	}

	Renderer2D::Statistics Renderer2D::GetStatistics()
	{
		return s_DrawData.Stats;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef SK_OLD_RENDERER2D

	namespace Old {

		struct SceanBuffer
		{
			DirectX::XMMATRIX ViewProjectionMatrix;
		};
		static SceanBuffer s_SceanBuffer;

		struct QuadVertex
		{
			DirectX::XMFLOAT3 Pos;
			DirectX::XMFLOAT4 Color;

			DirectX::XMFLOAT2 Tex;
			int TextureIndex;
			float TilingFactor;
		};

		struct CircleVertex
		{
			DirectX::XMFLOAT3 Pos;
			DirectX::XMFLOAT4 Color;

			DirectX::XMFLOAT3 Center;
			float Radius;

			DirectX::XMFLOAT2 Tex;
			int TextureIndex;
			float TilingFactor;

		};

		struct BatchRendererData
		{
			static constexpr uint32_t MaxGeometry = 20000;
			static constexpr uint32_t MaxVertices = MaxGeometry * 4;
			static constexpr uint32_t MaxIndices = MaxGeometry * 6;
			static constexpr uint32_t MaxQuadVerticesSize = MaxVertices * sizeof(QuadVertex);
			static constexpr uint32_t MaxCircleVerticesSize = MaxVertices * sizeof(CircleVertex);

			static constexpr uint32_t MaxTextureSlots = 16; // 16 is the max for samplers

			Ref<VertexBuffer> QuadVertexBuffer;
			Ref<VertexBuffer> CircleVertexBuffer;
			Ref<IndexBuffer> IndexBuffer;
			Ref<Shaders> QuadShader;
			Ref<Shaders> CircleShader;

			QuadVertex* QuadVertexBasePtr = nullptr;
			QuadVertex* QuadVertexIndexPtr = nullptr;
			uint32_t QuadCount = 0;

			CircleVertex* CircleVertexBasePtr = nullptr;
			CircleVertex* CircleVertexIndexPtr = nullptr;
			uint32_t CircleCount = 0;

			std::array<Ref<Texture2D>, BatchRendererData::MaxTextureSlots> QuadTextures;
			uint32_t QuadTextureCount = 0;

			std::array<Ref<Texture2D>, BatchRendererData::MaxTextureSlots> CircleTextures;
			uint32_t CircleTextureCount = 0;

			Ref<Texture2D> WitheTexture;

			Renderer2D::BatchStatistics Statistics;
		};

		static BatchRendererData s_BatchData;

		namespace Utils {

			static void ResetStatistics()
			{
				s_BatchData.Statistics.DrawCalls = 0;
				s_BatchData.Statistics.QuadCount = 0;
				s_BatchData.Statistics.CircleCount = 0;
				s_BatchData.Statistics.QuadTextureCount = 0;
				s_BatchData.Statistics.CircleTextureCount = 0;
				s_BatchData.Statistics.CircleCount = 0;
			}

			static void FlushQuad()
			{
				if (s_BatchData.QuadCount == 0)
					return;

				s_BatchData.IndexBuffer->Bind();
				s_BatchData.QuadVertexBuffer->Bind();
				s_BatchData.QuadShader->Bind();
				for (uint32_t i = 0; i < s_BatchData.QuadTextureCount; i++)
					s_BatchData.QuadTextures[i]->Bind();

				s_BatchData.QuadShader->SetBuffer("SceanData", Buffer::Ref(s_SceanBuffer));
				s_BatchData.QuadVertexBuffer->SetData(Buffer::Ref(s_BatchData.QuadVertexBasePtr, s_BatchData.MaxQuadVerticesSize));

				RendererCommand::DrawIndexed(s_BatchData.QuadCount * 6);

				s_BatchData.Statistics.DrawCalls++;
				s_BatchData.Statistics.QuadCount += s_BatchData.QuadCount;
				s_BatchData.Statistics.QuadTextureCount += s_BatchData.QuadTextureCount;

				s_BatchData.QuadVertexIndexPtr = s_BatchData.QuadVertexBasePtr;
				s_BatchData.QuadCount = 0;
				s_BatchData.QuadTextureCount = 1;
				s_BatchData.QuadTextures = {};
				s_BatchData.QuadTextures[0] = s_BatchData.WitheTexture;
			}

			static void FlushCircle()
			{
				if (s_BatchData.CircleCount == 0)
					return;

				s_BatchData.IndexBuffer->Bind();
				s_BatchData.CircleVertexBuffer->Bind();
				s_BatchData.CircleShader->Bind();
				for (uint32_t i = 0; i < s_BatchData.CircleTextureCount; i++)
					s_BatchData.CircleTextures[i]->Bind();

				s_BatchData.CircleShader->SetBuffer("SceanData", Buffer::Ref(s_SceanBuffer));
				s_BatchData.CircleVertexBuffer->SetData(Buffer::Ref(s_BatchData.CircleVertexBasePtr, s_BatchData.MaxCircleVerticesSize));

				RendererCommand::DrawIndexed(s_BatchData.CircleCount * 6);

				s_BatchData.Statistics.DrawCalls++;
				s_BatchData.Statistics.CircleCount += s_BatchData.CircleCount;
				s_BatchData.Statistics.CircleTextureCount += s_BatchData.CircleTextureCount;

				s_BatchData.CircleVertexIndexPtr = s_BatchData.CircleVertexBasePtr;
				s_BatchData.CircleCount = 0;
				s_BatchData.CircleTextureCount = 1;
				s_BatchData.CircleTextures = {};
				s_BatchData.CircleTextures[0] = s_BatchData.WitheTexture;
			}

			static void Flush()
			{
				FlushQuad();
				FlushCircle();
			}

			static void AddQuadTexture(const Ref<Texture2D>& texture)
			{
				bool found = false;
				for (uint32_t i = 0; i < s_BatchData.MaxTextureSlots; i++)
				{
					if (texture == s_BatchData.QuadTextures[i])
					{
						texture->SetSlot(i);
						found = true;
						break;
					}
				}

				if (!found)
				{
					s_BatchData.QuadTextures[s_BatchData.QuadTextureCount] = texture;
					texture->SetSlot(s_BatchData.QuadTextureCount);
					s_BatchData.QuadTextureCount++;
				}
			}

			static void AddCircleTexture(const Ref<Texture2D>& texture)
			{
				bool found = false;
				for (uint32_t i = 0; i < s_BatchData.MaxTextureSlots; i++)
				{
					if (texture == s_BatchData.CircleTextures[i])
					{
						texture->SetSlot(i);
						found = true;
						break;
					}
				}

				if (!found)
				{
					s_BatchData.CircleTextures[s_BatchData.CircleTextureCount] = texture;
					texture->SetSlot(s_BatchData.CircleTextureCount);
					s_BatchData.CircleTextureCount++;
				}
			}

			static void AddQuad(const DirectX::XMMATRIX& translation, const Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tintcolor)
			{
				constexpr DirectX::XMFLOAT3 Vertices[4] = { { -0.5f, 0.5f, 0.0f }, { 0.5f, 0.5f, 0.0f }, { 0.5f, -0.5f, 0.0f }, { -0.5f, -0.5f, 0.0f } };
				constexpr DirectX::XMFLOAT2 TexCoords[4] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

				if (s_BatchData.QuadCount >= s_BatchData.MaxGeometry)
					FlushQuad();

				for (uint32_t i = 0; i < 4; i++)
				{
					DirectX::XMStoreFloat3(&s_BatchData.QuadVertexIndexPtr->Pos, DirectX::XMVector3Transform(DirectX::XMLoadFloat3(&Vertices[i]), translation));
					s_BatchData.QuadVertexIndexPtr->Color = tintcolor;
					s_BatchData.QuadVertexIndexPtr->Tex = TexCoords[i];
					s_BatchData.QuadVertexIndexPtr->TextureIndex = texture->GetSlot();
					s_BatchData.QuadVertexIndexPtr->TilingFactor = tilingfactor;

					s_BatchData.QuadVertexIndexPtr++;
				}
				s_BatchData.QuadCount++;
			}

			static void AddCircle(const DirectX::XMMATRIX& translation, const DirectX::XMFLOAT2& center, float radius, const Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tintcolor)
			{
				constexpr DirectX::XMFLOAT3 Vertices[4] = { { -0.5f, 0.5f, 0.0f }, { 0.5f, 0.5f, 0.0f }, { 0.5f, -0.5f, 0.0f }, { -0.5f, -0.5f, 0.0f } };
				constexpr DirectX::XMFLOAT2 TexCoords[4] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

				if (s_BatchData.CircleCount >= s_BatchData.MaxGeometry)
					FlushCircle();

				for (uint32_t i = 0; i < 4; i++)
				{
					DirectX::XMStoreFloat3(&s_BatchData.CircleVertexIndexPtr->Pos, DirectX::XMVector3Transform(DirectX::XMLoadFloat3(&Vertices[i]), translation));
					s_BatchData.CircleVertexIndexPtr->Color = tintcolor;
					s_BatchData.CircleVertexIndexPtr->Tex = TexCoords[i];
					s_BatchData.CircleVertexIndexPtr->TextureIndex = texture->GetSlot();
					s_BatchData.CircleVertexIndexPtr->TilingFactor = tilingfactor;
					s_BatchData.CircleVertexIndexPtr->Radius = radius;
					s_BatchData.CircleVertexIndexPtr->Center = { center.x, center.y, 0.0f };

					s_BatchData.CircleVertexIndexPtr++;
				}
				s_BatchData.CircleCount++;
			}

		}

		void Renderer2D::Init()
		{
			s_BatchData.WitheTexture = Texture2D::Create({}, 1, 1, 0xFFFFFFFF);
			s_BatchData.WitheTexture->SetSlot(0);
			s_BatchData.QuadTextures[0] = s_BatchData.WitheTexture;
			s_BatchData.CircleTextures[0] = s_BatchData.WitheTexture;

			s_BatchData.QuadVertexBasePtr = new QuadVertex[s_BatchData.MaxVertices];
			s_BatchData.QuadVertexIndexPtr = s_BatchData.QuadVertexBasePtr;
			s_BatchData.CircleVertexBasePtr = new CircleVertex[s_BatchData.MaxVertices];
			s_BatchData.CircleVertexIndexPtr = s_BatchData.CircleVertexBasePtr;

			s_BatchData.QuadShader = Shaders::Create("assets/Shaders/MainShader.hlsl");
			s_BatchData.CircleShader = Shaders::Create("assets/Shaders/CircleShader.hlsl");

			s_BatchData.QuadVertexBuffer = VertexBuffer::Create(s_BatchData.QuadShader->GetVertexLayout(), {}, true);
			s_BatchData.QuadVertexBuffer->SetData(Buffer::Ref(s_BatchData.QuadVertexBasePtr, s_BatchData.MaxQuadVerticesSize));
			s_BatchData.CircleVertexBuffer = VertexBuffer::Create(s_BatchData.CircleShader->GetVertexLayout(), {}, true);
			s_BatchData.CircleVertexBuffer->SetData(Buffer::Ref(s_BatchData.CircleVertexBasePtr, s_BatchData.MaxCircleVerticesSize));

			uint32_t* indices = new uint32_t[s_BatchData.MaxIndices];
			uint32_t offset = 0;
			for (uint32_t i = 0; i < s_BatchData.MaxIndices; i += 6)
			{
				indices[i + 0] = offset + 0;
				indices[i + 1] = offset + 1;
				indices[i + 2] = offset + 2;

				indices[i + 3] = offset + 2;
				indices[i + 4] = offset + 3;
				indices[i + 5] = offset + 0;

				offset += 4;
			}

			s_BatchData.IndexBuffer = IndexBuffer::Create(Buffer::Ref(indices, s_BatchData.MaxIndices));
			delete[] indices;

			s_BatchData.QuadCount = 0;
			s_BatchData.CircleCount = 0;
			s_BatchData.QuadTextureCount = 0;
			s_BatchData.CircleTextureCount = 0;
			s_BatchData.QuadTextures[0] = s_BatchData.WitheTexture;
			s_BatchData.CircleTextures[0] = s_BatchData.WitheTexture;
			Utils::ResetStatistics();

#ifdef SK_DEBUG
			for (uint32_t i = 0; i < s_BatchData.MaxTextureSlots; i++)
				s_BatchData.WitheTexture->Bind(i);
#endif
		}

		void Renderer2D::ShutDown()
		{
			delete[] s_BatchData.QuadVertexBasePtr;
			delete[] s_BatchData.CircleVertexBasePtr;
			s_BatchData.QuadVertexBuffer.Release();
			s_BatchData.CircleVertexBuffer.Release();
			s_BatchData.QuadShader.Release();
			s_BatchData.CircleShader.Release();
			s_BatchData.IndexBuffer.Release();
			s_BatchData.QuadTextures = {};
			s_BatchData.CircleTextures = {};
			s_BatchData.WitheTexture.Release();
			s_BatchData.QuadCount = 0;
			s_BatchData.CircleCount = 0;
			s_BatchData.QuadTextureCount = 0;
			s_BatchData.CircleTextureCount = 0;
			Utils::ResetStatistics();
		}

		void Renderer2D::BeginScean(Camera& camera, const DirectX::XMMATRIX& view)
		{
			Utils::ResetStatistics();
			s_SceanBuffer.ViewProjectionMatrix = view * camera.GetProjection();
		}

		void Renderer2D::BeginScean(EditorCamera& camera)
		{
			Utils::ResetStatistics();
			s_SceanBuffer.ViewProjectionMatrix = camera.GetViewProjection();
		}

		void Renderer2D::EndScean()
		{
			Utils::Flush();
		}

		void Renderer2D::DrawQuad(const DirectX::XMFLOAT2& position, const DirectX::XMFLOAT2& scaling, const DirectX::XMFLOAT4& color)
		{
			const auto translation = DirectX::XMMatrixScaling(scaling.x, scaling.y, 1.0f) * DirectX::XMMatrixTranslation(position.x, position.y, 0.0f);
			Utils::AddQuad(translation, s_BatchData.WitheTexture, 1.0f, color);
		}

		void Renderer2D::DrawQuad(const DirectX::XMFLOAT2& position, const DirectX::XMFLOAT2& scaling, const Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tintcolor)
		{
			const auto translation = DirectX::XMMatrixScaling(scaling.x, scaling.y, 1.0f) * DirectX::XMMatrixTranslation(position.x, position.y, 0.0f);
			Utils::AddQuadTexture(texture);
			Utils::AddQuad(translation, texture, tilingfactor, tintcolor);
		}

		void Renderer2D::DrawRotatedQuad(const DirectX::XMFLOAT2& position, float rotation, const DirectX::XMFLOAT2& scaling, const DirectX::XMFLOAT4& color)
		{
			const auto translation = DirectX::XMMatrixScaling(scaling.x, scaling.y, 1.0f) * DirectX::XMMatrixRotationZ(rotation) * DirectX::XMMatrixTranslation(position.x, position.y, 0.0f);
			Utils::AddQuad(translation, s_BatchData.WitheTexture, 1.0f, color);
		}

		void Renderer2D::DrawRotatedQuad(const DirectX::XMFLOAT2& position, float rotation, const DirectX::XMFLOAT2& scaling, const Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tintcolor)
		{
			const auto translation = DirectX::XMMatrixScaling(scaling.x, scaling.y, 1.0f) * DirectX::XMMatrixRotationZ(rotation) * DirectX::XMMatrixTranslation(position.x, position.y, 0.0f);
			Utils::AddQuadTexture(texture);
			Utils::AddQuad(translation, texture, tilingfactor, tintcolor);
		}

		void Renderer2D::DrawCircle(const DirectX::XMFLOAT2& center, float radius, const DirectX::XMFLOAT4& color)
		{
			const auto translation = DirectX::XMMatrixScaling(radius * 2.0f, radius * 2.0f, 1.0f) * DirectX::XMMatrixTranslation(center.x, center.y, 0.0f);
			Utils::AddCircle(translation, center, radius, s_BatchData.WitheTexture, 1.0f, color);
		}

		void Renderer2D::DrawCircle(const DirectX::XMFLOAT2& center, float radius, const Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tintcolor)
		{
			const auto translation = DirectX::XMMatrixScaling(radius * 2.0f, radius * 2.0f, 1.0f) * DirectX::XMMatrixTranslation(center.x, center.y, 0.0f);
			Utils::AddCircleTexture(texture);
			Utils::AddCircle(translation, center, radius, texture, tilingfactor, tintcolor);
		}

		void Renderer2D::DrawRotatedCircle(const DirectX::XMFLOAT2& center, float radius, float rotation, const DirectX::XMFLOAT4& color)
		{
			const auto translation = DirectX::XMMatrixScaling(radius * 2.0f, radius * 2.0f, 1.0f) * DirectX::XMMatrixRotationZ(rotation) * DirectX::XMMatrixTranslation(center.x, center.y, 0.0f);
			Utils::AddCircle(translation, center, radius, s_BatchData.WitheTexture, 1.0f, color);
		}

		void Renderer2D::DrawRotatedCircle(const DirectX::XMFLOAT2& center, float radius, float rotation, const Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tintcolor)
		{
			const auto translation = DirectX::XMMatrixScaling(radius * 2.0f, radius * 2.0f, 1.0f) * DirectX::XMMatrixRotationZ(rotation) * DirectX::XMMatrixTranslation(center.x, center.y, 0.0f);
			Utils::AddCircleTexture(texture);
			Utils::AddCircle(translation, center, radius, texture, tilingfactor, tintcolor);
		}

		void Renderer2D::DrawEntity(Entity entity)
		{
			SK_CORE_ASSERT(entity.HasComponent<TransformComponent>(), "Tried to Draw Entity without Transform Component");
			SK_CORE_ASSERT(entity.HasComponent<SpriteRendererComponent>(), "Tried to Draw Entity without Sprite Renderer Component");

			auto& tc = entity.GetComponent<TransformComponent>();
			auto& src = entity.GetComponent<SpriteRendererComponent>();

			uint32_t id = entity;

			Ref<Texture2D> texture = src.Texture ? src.Texture : s_BatchData.WitheTexture;
			Utils::AddQuadTexture(texture);
			Utils::AddQuad(tc.GetTranform(), texture, src.TilingFactor, src.Color);
		}

		Renderer2D::BatchStatistics Renderer2D::GetBatchStatistics()
		{
			return s_BatchData.Statistics;
		}

	}

#endif

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//#ifdef SK_OLD_RENDERER2D
//
//	namespace Old {
//
//		struct QuadVertex
//		{
//			DirectX::XMFLOAT3 Pos;
//			DirectX::XMFLOAT4 Color;
//			DirectX::XMFLOAT2 Tex;
//			int TextureIndex;
//			float TilingFactor;
//		};
//
//		struct BatchRendererData
//		{
//			static constexpr uint32_t MaxQuads = 20000;
//			static constexpr uint32_t MaxQuadVertices = MaxQuads * 4;
//			static constexpr uint32_t MaxQuadIndices = MaxQuads * 6;
//			static constexpr uint32_t MaxQuadVerticesSize = MaxQuadVertices * sizeof(QuadVertex);
//			static constexpr uint32_t MaxTextureSlots = 16; // 16 is the max for samplers
//
//			Ref<VertexBuffer> QuadVertexBuffer;
//			Ref<IndexBuffer> QuadIndexBuffer;
//			Ref<Shaders> Shader;
//
//			QuadVertex* QuadVertexBasePtr = nullptr;
//			QuadVertex* QuadVertexIndexPtr = nullptr;
//			uint32_t QuadCount = 0;
//
//			Ref<Texture2D> WitheTexture;
//			std::array<Ref<Texture2D>, MaxTextureSlots> Textures;
//			uint32_t TextureCount = 0;
//
//			Renderer2D::Statistiks Stats;
//		};
//		static BatchRendererData s_BatchData;
//
//		struct SceanData
//		{
//			DirectX::XMMATRIX ViewProjectionMatrix;
//		};
//		static SceanData s_SceanData;
//
//		void Renderer2D::Init()
//		{
//			SK_CORE_ASSERT(!s_BatchData.QuadVertexBasePtr, "QuadVertexBasePtr allredy set!");
//
//			s_BatchData.QuadVertexBasePtr = new QuadVertex[s_BatchData.MaxQuadVertices];
//			s_BatchData.QuadVertexIndexPtr = s_BatchData.QuadVertexBasePtr;
//
//			s_BatchData.Shader = Shaders::Create("assets/Shaders/MainShader.hlsl");
//			s_BatchData.WitheTexture = Texture2D::Create({}, 1, 1, 0xFFFFFFFF);
//			s_BatchData.Textures[0] = s_BatchData.WitheTexture;
//
//			s_BatchData.QuadVertexBuffer = VertexBuffer::Create(s_BatchData.Shader->GetVertexLayout(), {}, true);
//			s_BatchData.QuadVertexBuffer->SetData(Buffer::Ref(s_BatchData.QuadVertexBasePtr, sizeof(QuadVertex) * s_BatchData.MaxQuadVertices));
//
//			uint32_t* indices = new uint32_t[s_BatchData.MaxQuadIndices];
//			uint32_t offset = 0;
//			for (uint32_t i = 0; i < s_BatchData.MaxQuadIndices; i += 6)
//			{
//				indices[i + 0] = offset + 0;
//				indices[i + 1] = offset + 1;
//				indices[i + 2] = offset + 2;
//
//				indices[i + 3] = offset + 2;
//				indices[i + 4] = offset + 3;
//				indices[i + 5] = offset + 0;
//
//				offset += 4;
//			}
//
//			s_BatchData.QuadIndexBuffer = IndexBuffer::Create(Buffer::Ref(indices, s_BatchData.MaxQuadIndices));
//			delete[] indices;
//
//#ifdef SK_DEBUG
//			for (uint32_t i = 0; i < s_BatchData.MaxTextureSlots; ++i)
//				s_BatchData.Textures[0]->Bind(i);
//#endif
//		}
//
//		void Renderer2D::ShutDown()
//		{
//			delete[] s_BatchData.QuadVertexBasePtr;
//			s_BatchData.QuadVertexBuffer.Release();
//			s_BatchData.QuadIndexBuffer.Release();
//			s_BatchData.Shader.Release();
//			s_BatchData.Textures = {};
//			s_BatchData.WitheTexture.Release();
//		}
//
//		static void ReBind()
//		{
//			s_BatchData.Shader->Bind();
//			s_BatchData.QuadVertexBuffer->Bind();
//			s_BatchData.QuadIndexBuffer->Bind();
//			for (uint32_t i = 0; i < s_BatchData.TextureCount; ++i)
//				s_BatchData.Textures[i]->Bind();
//		}
//
//		static void Flush()
//		{
//			if (s_BatchData.QuadCount == 0)
//				return;
//
//			ReBind();
//
//			s_BatchData.Shader->SetBuffer("SceanData", Buffer::Ref(s_SceanData));
//			s_BatchData.QuadVertexBuffer->SetData(Buffer::Ref(s_BatchData.QuadVertexBasePtr, s_BatchData.MaxQuadVerticesSize));
//
//			RendererCommand::DrawIndexed(s_BatchData.QuadCount * 6);
//
//			s_BatchData.Stats.DrawCalls++;
//			s_BatchData.Stats.QuadCount += s_BatchData.QuadCount;
//			s_BatchData.Stats.TextureCount += s_BatchData.TextureCount;
//
//			s_BatchData.QuadVertexIndexPtr = s_BatchData.QuadVertexBasePtr;
//			s_BatchData.QuadCount = 0;
//
//			s_BatchData.TextureCount = 1;
//			s_BatchData.Textures = {};
//			s_BatchData.Textures[0] = s_BatchData.WitheTexture;
//		}
//
//		Renderer2D::Statistiks Renderer2D::GetStats()
//		{
//			return s_BatchData.Stats;
//		}
//
//		void Renderer2D::ResetStats()
//		{
//			s_BatchData.Stats.DrawCalls = 0;
//			s_BatchData.Stats.QuadCount = 0;
//			s_BatchData.Stats.TextureCount = 0;
//		}
//
//		void Renderer2D::BeginScean(Camera& camera, const DirectX::XMMATRIX& view)
//		{
//			ResetStats();
//			s_SceanData.ViewProjectionMatrix = view * camera.GetProjection();
//		}
//
//		void Renderer2D::BeginScean(EditorCamera& camera)
//		{
//			ResetStats();
//			s_SceanData.ViewProjectionMatrix = camera.GetViewProjection();
//		}
//
//		void Renderer2D::EndScean()
//		{
//			Flush();
//		}
//
//		void Renderer2D::DrawQuad(const DirectX::XMFLOAT2& pos, const DirectX::XMFLOAT2& scaling, const DirectX::XMFLOAT4& color)
//		{
//			DrawQuad({ pos.x, pos.y, 0.0f }, scaling, color);
//		}
//
//		void Renderer2D::DrawQuad(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT2& scaling, const DirectX::XMFLOAT4& color)
//		{
//			const auto translation = DirectX::XMMatrixScaling(scaling.x, scaling.y, 1.0f) * DirectX::XMMatrixTranslation(pos.x, pos.y, pos.z);
//			DrawQuad(translation, color);
//		}
//
//		void Renderer2D::DrawQuad(const DirectX::XMFLOAT2& pos, const DirectX::XMFLOAT2& scaling, Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tint_color)
//		{
//			DrawQuad({ pos.x, pos.y, 0.0f }, scaling, texture, tilingfactor, tint_color);
//		}
//
//		void Renderer2D::DrawQuad(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT2& scaling, Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tint_color)
//		{
//			const auto translation = DirectX::XMMatrixScaling(scaling.x, scaling.y, 1.0f) * DirectX::XMMatrixTranslation(pos.x, pos.y, pos.z);
//			DrawQuad(translation, texture, tilingfactor, tint_color);
//		}
//
//		void Renderer2D::DrawRotatedQuad(const DirectX::XMFLOAT2& pos, float rotation, const DirectX::XMFLOAT2& scaling, const DirectX::XMFLOAT4& color)
//		{
//			DrawRotatedQuad({ pos.x, pos.y, 0.0f }, rotation, scaling, color);
//		}
//
//		void Renderer2D::DrawRotatedQuad(const DirectX::XMFLOAT3& pos, float rotation, const DirectX::XMFLOAT2& scaling, const DirectX::XMFLOAT4& color)
//		{
//			const auto translation = DirectX::XMMatrixScaling(scaling.x, scaling.y, 1.0f) * DirectX::XMMatrixRotationZ(rotation) * DirectX::XMMatrixTranslation(pos.x, pos.y, pos.z);
//			DrawQuad(translation, color);
//		}
//
//		void Renderer2D::DrawRotatedQuad(const DirectX::XMFLOAT2& pos, float rotation, const DirectX::XMFLOAT2& scaling, Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tint_color)
//		{
//			DrawRotatedQuad({ pos.x, pos.y, 0.0f }, rotation, scaling, texture, tilingfactor, tint_color);
//		}
//
//		void Renderer2D::DrawRotatedQuad(const DirectX::XMFLOAT3& pos, float rotation, const DirectX::XMFLOAT2& scaling, Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tint_color)
//		{
//			const auto translation = DirectX::XMMatrixScaling(scaling.x, scaling.y, 1.0f) * DirectX::XMMatrixRotationZ(rotation) * DirectX::XMMatrixTranslation(pos.x, pos.y, pos.z);
//			DrawQuad(translation, texture, tilingfactor, tint_color);
//		}
//
//		static void AddTexture(Ref<Texture2D> texture)
//		{
//			SK_CORE_ASSERT(texture != s_BatchData.WitheTexture, "BatchRendererData::WitheTexture Cant Be Added");
//
//			uint32_t slot = 0;
//			for (uint32_t i = 0; i < s_BatchData.TextureCount; ++i)
//			{
//				if (s_BatchData.Textures[i] == texture)
//				{
//					slot = i;
//					texture->SetSlot(slot);
//					break;
//				}
//			}
//
//			if (slot == 0)
//			{
//				if (s_BatchData.TextureCount >= s_BatchData.MaxTextureSlots)
//					Flush();
//
//				s_BatchData.Textures[s_BatchData.TextureCount] = texture;
//				texture->SetSlot(s_BatchData.TextureCount++);
//			}
//		}
//
//		static void AddQuad(const DirectX::XMMATRIX& translation, const Ref<Texture2D> texture, float tilingfactor, const DirectX::XMFLOAT4& tintcolor)
//		{
//			constexpr DirectX::XMFLOAT3 Vertices[4] = { { -0.5f, 0.5f, 0.0f }, { 0.5f, 0.5f, 0.0f }, { 0.5f, -0.5f, 0.0f }, { -0.5f, -0.5f, 0.0f } };
//			constexpr DirectX::XMFLOAT2 TexCoords[4] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
//
//			for (uint32_t i = 0; i < 4; i++)
//			{
//				DirectX::XMStoreFloat3(&s_BatchData.QuadVertexIndexPtr->Pos, DirectX::XMVector3Transform(DirectX::XMLoadFloat3(&Vertices[i]), translation));
//				s_BatchData.QuadVertexIndexPtr->Color = tintcolor;
//				s_BatchData.QuadVertexIndexPtr->Tex = TexCoords[i];
//				s_BatchData.QuadVertexIndexPtr->TextureIndex = texture->GetSlot();
//				s_BatchData.QuadVertexIndexPtr->TilingFactor = tilingfactor;
//				s_BatchData.QuadVertexIndexPtr++;
//			}
//
//			s_BatchData.QuadCount++;
//
//		}
//
//		void Renderer2D::DrawQuad(const DirectX::XMMATRIX& translation, const DirectX::XMFLOAT4& color)
//		{
//			if (s_BatchData.QuadCount >= s_BatchData.MaxQuads)
//				Flush();
//
//			AddQuad(translation, s_BatchData.WitheTexture, 1.0f, color);
//		}
//
//		void Renderer2D::DrawQuad(const DirectX::XMMATRIX& translation, Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tint_color)
//		{
//			if (s_BatchData.QuadCount >= s_BatchData.MaxQuads)
//				Flush();
//
//			AddQuad(translation, texture, tilingfactor, tint_color);
//		}
//
//		void Renderer2D::DrawEntity(Entity entity)
//		{
//			SK_CORE_ASSERT(entity.HasComponent<TransformComponent>(), "Tried to Draw Entity without Transform Component");
//			SK_CORE_ASSERT(entity.HasComponent<SpriteRendererComponent>(), "Tried to Draw Entity without Sprite Renderer Component")
//
//				if (s_BatchData.QuadCount >= s_BatchData.MaxQuads)
//					Flush();
//
//
//			auto& src = entity.GetComponent<SpriteRendererComponent>();
//			auto& tc = entity.GetComponent<TransformComponent>();
//			if (src.Texture)
//			{
//				AddTexture(src.Texture);
//				AddQuad(tc.GetTranform(), src.Texture, src.TilingFactor, src.Color);
//			}
//			else
//			{
//				AddQuad(tc.GetTranform(), s_BatchData.WitheTexture, src.TilingFactor, src.Color);
//			}
//
//		}
//
//	}
//
//#endif

}
