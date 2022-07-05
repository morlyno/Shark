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
#include "Shark/Render/Material.h"

#include <glm/glm.hpp>

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
			TimeStep GeometryPassTime;
		};

	public:
		Renderer2D(Ref<FrameBuffer> renderTarget);
		~Renderer2D();

		void Init(Ref<FrameBuffer> renderTarget);
		void ShutDown();

		void SetRenderTarget(Ref<FrameBuffer> renderTarget);

		void BeginScene(const glm::mat4& viewProj);
		void EndScene();


		void DrawQuad(const glm::vec2& position, const glm::vec2& scaling, const glm::vec4& color, int id = -1);
		void DrawQuad(const glm::vec3& position, const glm::vec3& scaling, const glm::vec4& color, int id = -1);
		void DrawQuad(const glm::vec2& position, const glm::vec2& scaling, const Ref<Texture2D>& texture, float tilingfactor = 1.0f, const glm::vec4& tintcolor = { 1.0f, 1.0f, 1.0f, 1.0f }, int id = -1);
		void DrawQuad(const glm::vec3& position, const glm::vec3& scaling, const Ref<Texture2D>& texture, float tilingfactor = 1.0f, const glm::vec4& tintcolor = { 1.0f, 1.0f, 1.0f, 1.0f }, int id = -1);

		void DrawRotatedQuad(const glm::vec2& position, float rotation,           const glm::vec2& scaling, const glm::vec4& color, int id = -1);
		void DrawRotatedQuad(const glm::vec3& position, const glm::vec3& roation, const glm::vec3& scaling, const glm::vec4& color, int id = -1);
		void DrawRotatedQuad(const glm::vec2& position, float rotation,           const glm::vec2& scaling, const Ref<Texture2D>& texture, float tilingfactor = 1.0f, const glm::vec4& tintcolor = { 1.0f, 1.0f, 1.0f, 1.0f }, int id = -1);
		void DrawRotatedQuad(const glm::vec3& position, const glm::vec3& roation, const glm::vec3& scaling, const Ref<Texture2D>& texture, float tilingfactor = 1.0f, const glm::vec4& tintcolor = { 1.0f, 1.0f, 1.0f, 1.0f }, int id = -1);

		void DrawQuad(const glm::mat4& transform, const glm::vec4& color, int id = -1);
		void DrawQuad(const glm::mat4& transform, Ref<Texture2D> texture, float tilingfactor, const glm::vec4& color, int id = -1);


		void DrawFilledCircle(const glm::vec2& position, const glm::vec2& scaling, const glm::vec4& color, float thickness = 1.0f, float fade = 0.002f, int id = -1);
		void DrawFilledCircle(const glm::vec3& position, const glm::vec3& scaling, const glm::vec4& color, float thickness = 1.0f, float fade = 0.002f, int id = -1);
		void DrawFilledCircle(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scaling, const glm::vec4& color, float thickness = 1.0f, float fade = 0.002f, int id = -1);

		void DrawFilledCircle(const glm::mat4& transform, const glm::vec4& color, float thickness, float fade, int id = -1);


		void DrawCircle(const glm::vec2& position, float radius, const glm::vec4& color, int id = -1);
		void DrawCircle(const glm::vec3& position, const glm::vec3& rotation, float radius, const glm::vec4& color, int id = -1);

		void DrawCircle(const glm::mat4& transform, const glm::vec4& color, int id = -1);


		void DrawLine(const glm::vec2& pos0, const glm::vec2& pos1, const glm::vec4& color, int id = -1);
		void DrawLine(const glm::vec3& pos0, const glm::vec3& pos1, const glm::vec4& color, int id = -1);


		void DrawRect(const glm::vec2& position, const glm::vec2& scaling, const glm::vec4& color, int id = -1);
		void DrawRect(const glm::vec3& position, const glm::vec3& scaling, const glm::vec4& color, int id = -1);
		void DrawRect(const glm::vec2& position, float rotation,            const glm::vec2& scaling, const glm::vec4& color, int id = -1);
		void DrawRect(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scaling, const glm::vec4& color, int id = -1);

		void DrawRect(const glm::mat4& transform, const glm::vec4& color, int id = -1);


		void DrawLineOnTop(const glm::vec2& pos0, const glm::vec2& pos1, const glm::vec4& color, int id = -1);
		void DrawLineOnTop(const glm::vec3& pos0, const glm::vec3& pos1, const glm::vec4& color, int id = -1);


		void DrawRectOnTop(const glm::vec2& position, const glm::vec2& scaling, const glm::vec4& color, int id = -1);
		void DrawRectOnTop(const glm::vec3& position, const glm::vec3& scaling, const glm::vec4& color, int id = -1);
		void DrawRectOnTop(const glm::vec2& position, float rotation, const glm::vec2& scaling, const glm::vec4& color, int id = -1);
		void DrawRectOnTop(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scaling, const glm::vec4& color, int id = -1);

		void DrawRectOnTop(const glm::mat4& transform, const glm::vec4& color, int id = -1);


		void DrawCircleOnTop(const glm::vec2& position, float radius, const glm::vec4& color, int id = -1);
		void DrawCircleOnTop(const glm::vec3& position, const glm::vec3& rotation, float radius, const glm::vec4& color, int id = -1);

		void DrawCircleOnTop(const glm::mat4& transform, const glm::vec4& color, int id = -1);


		Ref<RenderCommandBuffer> GetCommandBuffer() const { return m_CommandBuffer; }

		const Statistics& GetStatistics() const { return m_Statistics; }

	private:
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

	private:
		const glm::vec4 m_QuadVertexPositions[4] = { { -0.5f, 0.5f, 0.0f, 1.0f }, { 0.5f, 0.5f, 0.0f, 1.0f }, { 0.5f, -0.5f, 0.0f, 1.0f }, { -0.5f, -0.5f, 0.0f, 1.0f } };
		const glm::vec2 m_TextureCoords[4] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
		std::array<glm::vec4, 20> m_CirlceVertexPositions;

	private:
		using Index = IndexBuffer::IndexType;

		struct QuadVertex
		{
			glm::vec3 WorldPosition;
			glm::vec4 Color;
			glm::vec2 Tex;
			int TextureSlot;
			float TilingFactor;
			int ID;
		};

		struct CircleVertex
		{
			glm::vec3 WorldPosition;
			glm::vec2 LocalPosition;
			glm::vec4 Color;
			float Thickness;
			float Fade;
			int ID;
		};

		struct LineVertex
		{
			glm::vec3 WorldPosition;
			glm::vec4 Color;
			int ID;
		};

		struct CBCamera
		{
			glm::mat4 ViewProjection;
		};

	private:
		Statistics m_Statistics;
		bool m_Active = false;

		Ref<RenderCommandBuffer> m_CommandBuffer;

		Ref<Texture2D> m_WhiteTexture;
		Ref<ConstantBufferSet> m_ConstantBufferSet;
		glm::mat4 m_ViewProj;

		Ref<GPUTimer> m_GeometryPassTimer;
		Ref<GPUTimer> m_QuadFlushQuery;
		Ref<GPUTimer> m_CircleFlushQuery;
		Ref<GPUTimer> m_LineFlushQuery;
		Ref<GPUTimer> m_LineOnTopFlushQuery;

		// Quad
		Ref<Pipeline> m_QuadPipeline;
		Ref<Material> m_QuadMaterial;
		Ref<VertexBuffer> m_QuadVertexBuffer;
		Ref<IndexBuffer> m_QuadIndexBuffer;
		uint32_t m_QuadTextureSlotIndex = 1;
		uint32_t m_QuadIndexCount = 0;
		QuadVertex* m_QuadVertexBasePtr = nullptr;
		QuadVertex* m_QuadVertexIndexPtr = nullptr;


		// Circle
		Ref<Pipeline> m_CirlcePipeline;
		Ref<VertexBuffer> m_CircleVertexBuffer;
		Ref<Material> m_CircleMaterial;
		uint32_t m_CircleIndexCount = 0;
		CircleVertex* m_CircleVertexBasePtr = nullptr;
		CircleVertex* m_CircleVertexIndexPtr = nullptr;

		// Line
		Ref<Pipeline> m_LinePipeline;
		Ref<Material> m_LineMaterial;
		Ref<VertexBuffer> m_LineVertexBuffer;
		uint32_t m_LineVertexCount = 0;
		LineVertex* m_LineVertexBasePtr = nullptr;
		LineVertex* m_LineVertexIndexPtr = nullptr;


		// Line Without Depth Testing
		Ref<Pipeline> m_LineOnTopPipeline;
		Ref<Material> m_LineOnTopMaterial;
		Ref<VertexBuffer> m_LineOnTopVertexBuffer;
		uint32_t m_LineOnTopVertexCount = 0;
		LineVertex* m_LineOnTopVertexBasePtr = nullptr;
		LineVertex* m_LineOnTopVertexIndexPtr = nullptr;

	};

}