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

	struct Renderer2DSpecifications
	{
		bool UseDepthTesting = true;
	};

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
		Renderer2D(Ref<FrameBuffer> renderTarget, const Renderer2DSpecifications& specifications = {});
		~Renderer2D();

		void Init(Ref<FrameBuffer> renderTarget, const Renderer2DSpecifications& specifications = {});
		void ShutDown();

		void Resize(uint32_t width, uint32_t height);

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
		void DrawCircle(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale, const glm::vec4& color, int id = -1);

		void DrawCircle(const glm::mat4& transform, const glm::vec4& color, int id = -1);


		void DrawLine(const glm::vec2& pos0, const glm::vec2& pos1, const glm::vec4& color, int id = -1);
		void DrawLine(const glm::vec3& pos0, const glm::vec3& pos1, const glm::vec4& color, int id = -1);


		void DrawRect(const glm::vec2& position, const glm::vec2& scaling, const glm::vec4& color, int id = -1);
		void DrawRect(const glm::vec3& position, const glm::vec3& scaling, const glm::vec4& color, int id = -1);
		void DrawRect(const glm::vec2& position, float rotation,            const glm::vec2& scaling, const glm::vec4& color, int id = -1);
		void DrawRect(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scaling, const glm::vec4& color, int id = -1);

		void DrawRect(const glm::mat4& transform, const glm::vec4& color, int id = -1);


		Ref<RenderCommandBuffer> GetCommandBuffer() const { return m_CommandBuffer; }

		const Renderer2DSpecifications& GetSpecifications() const { return m_Specifications; }
		const Statistics& GetStatistics() const { return m_Statistics; }

		Ref<Image2D> GetDepthImage() const { return m_DepthFrameBuffer->GetDepthImage(); }

	private:
		struct QuadBatch;

		void AssureQuadVertexDataSize();
		void AssureCircleVertexDataSize();
		void AssureLineVertexDataSize();

		void BeginQaudBatch();
		uint32_t AddTexture(QuadBatch* batch, Ref<Texture2D> texture);
		void PrepareMaterial(Ref<Material> material, const QuadBatch& batch);
		void ResizeQuadIndexBuffer(uint64_t indexCount);

	public:
		static constexpr uint32_t MaxTextureSlots = 16;

		static constexpr uint32_t DefaultQuadCount = 100;
		static constexpr uint32_t DefaultQuadVertices = DefaultQuadCount * 4;
		static constexpr uint32_t DefaultQuadIndices = DefaultQuadCount * 6;

		static constexpr uint32_t DefaultCircleCount = 100;
		static constexpr uint32_t DefaultCircleVertices = DefaultCircleCount * 4;
		static constexpr uint32_t DefaultCircleIndices = DefaultCircleCount * 6;

		static constexpr uint32_t DefaultLineCount = 100;
		static constexpr uint32_t DefaultLineVertices = DefaultLineCount * 2;

	private:
		const glm::vec4 m_QuadVertexPositions[4] = { { -0.5f, 0.5f, 0.0f, 1.0f }, { 0.5f, 0.5f, 0.0f, 1.0f }, { 0.5f, -0.5f, 0.0f, 1.0f }, { -0.5f, -0.5f, 0.0f, 1.0f } };
		const glm::vec2 m_TextureCoords[4] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
		std::array<glm::vec4, 20> m_CirlceVertexPositions;

	private:
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
		Renderer2DSpecifications m_Specifications;
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


		// Depth
		Ref<FrameBuffer> m_DepthFrameBuffer;

		Ref<Pipeline> m_QuadDepthPassPipeline;
		Ref<Material> m_QuadDepthPassMaterial;
		
		Ref<Pipeline> m_CircleDepthPassPipeline;
		Ref<Material> m_CircleDepthPassMaterial;

		Ref<Pipeline> m_LineDepthPassPipeline;
		Ref<Material> m_LineDepthPassMaterial;

		
		struct QuadBatch
		{
			uint64_t VertexOffset = 0;
			uint64_t IndexCount = 0;
			uint64_t VertexCount = 0;
			std::vector<Ref<Texture2D>> Textures;

			QuadBatch(uint64_t dataOffset)
			{
				VertexOffset = dataOffset;
				Textures.reserve(16);
			}
		};

		// Quad
		Ref<Pipeline> m_QuadPipeline;
		Ref<Material> m_QuadMaterial;
		Ref<Texture2D> m_QuadTextureArray;
		Ref<VertexBuffer> m_QuadVertexBuffer;
		Ref<IndexBuffer> m_QuadIndexBuffer;
		Buffer m_QuadVertexData;
		std::vector<QuadBatch> m_QuadBatches;
		QuadBatch* m_QuadBatch;
		uint32_t m_QuadIndexCount = 0;

		// Circle
		Ref<Pipeline> m_CirclePipeline;
		Ref<Material> m_CircleMaterial;
		Ref<VertexBuffer> m_CircleVertexBuffer;
		Ref<IndexBuffer> m_CircleIndexBuffer;
		Buffer m_CircleVertexData;
		uint32_t m_CircleIndexCount = 0;
		uint64_t m_CircleVertexCount = 0;

		// Line
		Ref<Pipeline> m_LinePipeline;
		Ref<Material> m_LineMaterial;
		Ref<VertexBuffer> m_LineVertexBuffer;
		Buffer m_LineVertexData;
		uint32_t m_LineVertexCount = 0;

	};

}