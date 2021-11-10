#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/EditorCamera.h"
#include "Shark/Render/Texture.h"

#include "Shark/Render/RenderCommandBuffer.h"
#include "Shark/Render/Shader.h"
#include "Shark/Render/FrameBuffer.h"
#include "Shark/Render/Buffers.h"
#include "Shark/Render/ConstantBuffer.h"
#include "Shark/Render/Pipeline.h"

#include "Shark/Scene/Entity.h"

namespace Shark {

	class Renderer2D : public RefCount
	{
	public:
		struct Statistics
		{
			uint32_t DrawCalls;

			uint32_t QuadCount;
			uint32_t CircleCount;
			uint32_t LineCount;
			uint32_t LineOnTopCount;

			uint32_t VertexCount;
			uint32_t IndexCount;

			uint32_t TextureCount;

			// Currently not correct
			// only messures the last draw calles not the Flushes
			// Can be fixed by switching to resizable Batches
			Ref<GPUTimer> GeometryPassTimer;
		};

	public:
		Renderer2D(Ref<FrameBuffer> renderTarget);
		~Renderer2D();

		void Init(Ref<FrameBuffer> renderTarget);
		void ShutDown();

		void SetRenderTarget(Ref<FrameBuffer> renderTarget);

		void BeginScene(const DirectX::XMMATRIX& viewProj);
		void EndScene();

		void DrawQuad(const DirectX::XMFLOAT2& position, const DirectX::XMFLOAT2& scaling, const DirectX::XMFLOAT4& color, int id = -1);
		void DrawQuad(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& scaling, const DirectX::XMFLOAT4& color, int id = -1);
		void DrawQuad(const DirectX::XMFLOAT2& position, const DirectX::XMFLOAT2& scaling, const Ref<Texture2D>& texture, float tilingfactor = 1.0f, const DirectX::XMFLOAT4& tintcolor = { 1.0f, 1.0f, 1.0f, 1.0f }, int id = -1);
		void DrawQuad(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& scaling, const Ref<Texture2D>& texture, float tilingfactor = 1.0f, const DirectX::XMFLOAT4& tintcolor = { 1.0f, 1.0f, 1.0f, 1.0f }, int id = -1);

		void DrawRotatedQuad(const DirectX::XMFLOAT2& position, float rotation,                   const DirectX::XMFLOAT2& scaling, const DirectX::XMFLOAT4& color, int id = -1);
		void DrawRotatedQuad(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& roation, const DirectX::XMFLOAT3& scaling, const DirectX::XMFLOAT4& color, int id = -1);
		void DrawRotatedQuad(const DirectX::XMFLOAT2& position, float rotation,                   const DirectX::XMFLOAT2& scaling, const Ref<Texture2D>& texture, float tilingfactor = 1.0f, const DirectX::XMFLOAT4& tintcolor = { 1.0f, 1.0f, 1.0f, 1.0f }, int id = -1);
		void DrawRotatedQuad(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& roation, const DirectX::XMFLOAT3& scaling, const Ref<Texture2D>& texture, float tilingfactor = 1.0f, const DirectX::XMFLOAT4& tintcolor = { 1.0f, 1.0f, 1.0f, 1.0f }, int id = -1);


		void DrawFilledCircle(const DirectX::XMFLOAT2& position, const DirectX::XMFLOAT2& scaling, const DirectX::XMFLOAT4& color, float thickness = 1.0f, float fade = 0.002f, int id = -1);
		void DrawFilledCircle(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& scaling, const DirectX::XMFLOAT4& color, float thickness = 1.0f, float fade = 0.002f, int id = -1);
		void DrawFilledCircle(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& rotation, const DirectX::XMFLOAT3& scaling, const DirectX::XMFLOAT4& color, float thickness = 1.0f, float fade = 0.002f, int id = -1);


		void DrawCircle(const DirectX::XMFLOAT2& position, float radius, const DirectX::XMFLOAT4& color, float delta = DirectX::XM_PI / 10.0f, int id = -1);
		void DrawCircle(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& rotation, float radius, const DirectX::XMFLOAT4& color, float delta = DirectX::XM_PI / 10.0f, int id = -1);


		void DrawLine(const DirectX::XMFLOAT2& pos0, const DirectX::XMFLOAT2& pos1, const DirectX::XMFLOAT4& color, int id = -1);
		void DrawLine(const DirectX::XMFLOAT3& pos0, const DirectX::XMFLOAT3& pos1, const DirectX::XMFLOAT4& color, int id = -1);


		void DrawRect(const DirectX::XMFLOAT2& position, const DirectX::XMFLOAT2& scaling, const DirectX::XMFLOAT4& color, int id = -1);
		void DrawRect(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& scaling, const DirectX::XMFLOAT4& color, int id = -1);

		void DrawRect(const DirectX::XMFLOAT2& position, float rotation,                    const DirectX::XMFLOAT2& scaling, const DirectX::XMFLOAT4& color, int id = -1);
		void DrawRect(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& rotation, const DirectX::XMFLOAT3& scaling, const DirectX::XMFLOAT4& color, int id = -1);


		void DrawLineOnTop(const DirectX::XMFLOAT2& pos0, const DirectX::XMFLOAT2& pos1, const DirectX::XMFLOAT4& color, int id = -1);
		void DrawLineOnTop(const DirectX::XMFLOAT3& pos0, const DirectX::XMFLOAT3& pos1, const DirectX::XMFLOAT4& color, int id = -1);

		void DrawRectOnTop(const DirectX::XMFLOAT2& position, const DirectX::XMFLOAT2& scaling, const DirectX::XMFLOAT4& color, int id = -1);
		void DrawRectOnTop(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& scaling, const DirectX::XMFLOAT4& color, int id = -1);

		void DrawRectOnTop(const DirectX::XMFLOAT2& position, float rotation, const DirectX::XMFLOAT2& scaling, const DirectX::XMFLOAT4& color, int id = -1);
		void DrawRectOnTop(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& rotation, const DirectX::XMFLOAT3& scaling, const DirectX::XMFLOAT4& color, int id = -1);

		void DrawCircleOnTop(const DirectX::XMFLOAT2& position, float radius, const DirectX::XMFLOAT4& color, float delta = DirectX::XM_PI / 10.0f, int id = -1);
		void DrawCircleOnTop(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& rotation, float radius, const DirectX::XMFLOAT4& color, float delta = DirectX::XM_PI / 10.0f, int id = -1);


		Ref<RenderCommandBuffer> GetCommandBuffer() const { return m_CommandBuffer; }

		Statistics GetStatistics() const { return m_Statistics; }

	private:
		// FlushAndReset*() dosn't execute the RenderCommandBuffer
		// Only EndScene executes the RenderCommandBuffer

		void FlushAndResetQuad();
		void FlushAndResetCircle();
		void FlushAndResetLine();
		void FlushAndResetLineOnTop();

	public:
		static constexpr uint32_t MaxTextureSlots = 16;

		static constexpr uint32_t MaxQuads = 200000;
		static constexpr uint32_t MaxQuadVertices = MaxQuads * 4;
		static constexpr uint32_t MaxQuadIndices = MaxQuads * 6;

		static constexpr uint32_t MaxCircles = 20000;
		static constexpr uint32_t MaxCircleVertices = MaxCircles * 4;
		static constexpr uint32_t MaxCircleIndices = MaxCircles * 6;

		static constexpr uint32_t MaxLines = 20000;
		static constexpr uint32_t MaxLineVertices = MaxLines * 2;

		static constexpr uint32_t MaxLinesOnTop = 2000;
		static constexpr uint32_t MaxLineOnTopVertices = MaxLinesOnTop * 2;

		static constexpr DirectX::XMFLOAT3 QuadVertexPositions[4] = { { -0.5f, 0.5f, 0.0f }, { 0.5f, 0.5f, 0.0f }, { 0.5f, -0.5f, 0.0f }, { -0.5f, -0.5f, 0.0f } };
		static constexpr DirectX::XMFLOAT2 TextureCoords[4] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

	private:
		using Index = IndexBuffer::IndexType;

		struct QuadVertex
		{
			DirectX::XMFLOAT3 WorldPosition;
			DirectX::XMFLOAT4 Color;
			DirectX::XMFLOAT2 Tex;
			int TextureSlot;
			float TilingFactor;
			int ID;
		};

		struct CircleVertex
		{
			DirectX::XMFLOAT3 WorldPosition;
			DirectX::XMFLOAT2 LocalPosition;
			DirectX::XMFLOAT4 Color;
			float Thickness;
			float Fade;
			int ID;
		};

		struct LineVertex
		{
			DirectX::XMFLOAT3 WorldPosition;
			DirectX::XMFLOAT4 Color;
			int ID;
		};

		struct CBCamera
		{
			DirectX::XMMATRIX ViewProjection;
		};

	private:
		Statistics m_Statistics;
		bool m_Active = false;

		Ref<FrameBuffer> m_RenderTarget;
		Ref<RenderCommandBuffer> m_CommandBuffer;

		Ref<Texture2D> m_WhiteTexture;
		Ref<ConstantBufferSet> m_ConstantBufferSet;


		// Quad
		Ref<Pipeline> m_QuadPipeline;
		Ref<VertexBuffer> m_QuadVertexBuffer;
		Ref<IndexBuffer> m_QuadIndexBuffer;
		Ref<Texture2DArray> m_QuadTextureArray;
		uint32_t m_QuadTextureSlotIndex = 1;
		uint32_t m_QuadIndexCount = 0;
		QuadVertex* m_QuadVertexBasePtr = nullptr;
		QuadVertex* m_QuadVertexIndexPtr = nullptr;


		// Circle
		Ref<Pipeline> m_CirlcePipeline;
		Ref<VertexBuffer> m_CircleVertexBuffer;
		uint32_t m_CircleIndexCount = 0;
		CircleVertex* m_CircleVertexBasePtr = nullptr;
		CircleVertex* m_CircleVertexIndexPtr = nullptr;


		// Line
		Ref<Pipeline> m_LinePipeline;
		Ref<VertexBuffer> m_LineVertexBuffer;
		uint32_t m_LineVertexCount = 0;
		LineVertex* m_LineVertexBasePtr = nullptr;
		LineVertex* m_LineVertexIndexPtr = nullptr;


		// Line Without Depth Testing
		Ref<Pipeline> m_LineOnTopPipeline;
		Ref<VertexBuffer> m_LineOnTopVertexBuffer;
		uint32_t m_LineOnTopVertexCount = 0;
		LineVertex* m_LineOnTopVertexBasePtr = nullptr;
		LineVertex* m_LineOnTopVertexIndexPtr = nullptr;

	};

}