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
#include "Shark/Render/Font.h"
#include "Shark/Render/RenderPass.h"

#include <glm/glm.hpp>

#define RENDERER2D_DEPTH_ONLY_PASS 1

namespace Shark {

	struct Renderer2DSpecifications
	{
		uint32_t Width = 0, Height = 0;

		bool UseDepthTesting = true;
	};

	class Renderer2D : public RefCount
	{
	public:
		struct Statistics
		{
			uint32_t DrawCalls = 0;

			uint32_t QuadCount = 0;
			uint32_t CircleCount = 0;
			uint32_t LineCount = 0;
			uint32_t GlyphCount = 0;

			uint32_t VertexCount = 0;
			uint32_t IndexCount = 0;

			uint32_t TextureCount = 0;

			TimeStep GeometryPassTime = 0;
			TimeStep QuadPassTime = 0;
			TimeStep CirclePassTime = 0;
			TimeStep LinePassTime = 0;
			TimeStep TextPassTime = 0;
		};

	public:
		Renderer2D(Ref<FrameBuffer> targetFramebuffer, const Renderer2DSpecifications& specifications = {});
		~Renderer2D();

		Ref<FrameBuffer> GetTargetFramebuffer() const;
		void SetTargetFramebuffer(Ref<FrameBuffer> framebuffer);

		void Init(Ref<FrameBuffer> targetFramebuffer);
		void ShutDown();

		void Resize(uint32_t width, uint32_t height);

		void BeginScene(const glm::mat4& viewProj);
		void EndScene();

		void DrawQuad(const glm::vec2& position, const glm::vec2& scaling, const glm::vec4& color, int id = -1);
		void DrawQuad(const glm::vec3& position, const glm::vec3& scaling, const glm::vec4& color, int id = -1);
		void DrawQuad(const glm::vec2& position, const glm::vec2& scaling, const Ref<Texture2D>& texture, const glm::vec2& tilingfactor = glm::vec2(1.0f), const glm::vec4& tintcolor = { 1.0f, 1.0f, 1.0f, 1.0f }, int id = -1);
		void DrawQuad(const glm::vec3& position, const glm::vec3& scaling, const Ref<Texture2D>& texture, const glm::vec2& tilingfactor = glm::vec2(1.0f), const glm::vec4& tintcolor = { 1.0f, 1.0f, 1.0f, 1.0f }, int id = -1);

		void DrawRotatedQuad(const glm::vec2& position, float rotation,           const glm::vec2& scaling, const glm::vec4& color, int id = -1);
		void DrawRotatedQuad(const glm::vec3& position, const glm::vec3& roation, const glm::vec3& scaling, const glm::vec4& color, int id = -1);
		void DrawRotatedQuad(const glm::vec2& position, float rotation,           const glm::vec2& scaling, const Ref<Texture2D>& texture, const glm::vec2& tilingfactor = glm::vec2(1.0f), const glm::vec4& tintcolor = { 1.0f, 1.0f, 1.0f, 1.0f }, int id = -1);
		void DrawRotatedQuad(const glm::vec3& position, const glm::vec3& roation, const glm::vec3& scaling, const Ref<Texture2D>& texture, const glm::vec2& tilingfactor = glm::vec2(1.0f), const glm::vec4& tintcolor = { 1.0f, 1.0f, 1.0f, 1.0f }, int id = -1);

		void DrawQuad(const glm::mat4& transform, const glm::vec4& color, int id = -1);
		void DrawQuad(const glm::mat4& transform, Ref<Texture2D> texture, const glm::vec2& tilingfactor, const glm::vec4& color, int id = -1);


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

		void DrawString(const std::string& string, Ref<Font> font, const glm::mat4& transform, float kerning, float lineSpacing, const glm::vec4& color, int id = -1);

		const Renderer2DSpecifications& GetSpecifications() const { return m_Specifications; }
		const Statistics& GetStatistics() const { return m_Statistics; }

	private:
		void GeometryPass();

	private:
		struct QuadBatch;

		void AssureQuadVertexDataSize();
		void AssureCircleVertexDataSize();
		void AssureLineVertexDataSize();
		void AssureTextVertexDataSize(uint32_t glyphCount);

		uint32_t AddTexture(Ref<Texture2D> texture);
		Ref<Material> PrepareMaterial(uint32_t batchindex, const QuadBatch& batch);
		void ResizeQuadIndexBuffer(uint32_t indexCount);

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

		static constexpr uint32_t DefaultTextCount = 100;
		static constexpr uint32_t DefaultTextVertices = DefaultTextCount * 4;
		static constexpr uint32_t DefaultTextIndices = DefaultTextCount * 6;

		static constexpr uint32_t MaxCircleVertexPositions = 40;

	private:
		const glm::vec4 m_QuadVertexPositions[4] = { { -0.5f, 0.5f, 0.0f, 1.0f }, { 0.5f, 0.5f, 0.0f, 1.0f }, { 0.5f, -0.5f, 0.0f, 1.0f }, { -0.5f, -0.5f, 0.0f, 1.0f } };
		const glm::vec2 m_TextureCoords[4] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
		std::array<glm::vec4, MaxCircleVertexPositions> m_CircleVertexPositions;

	private:
		struct QuadVertex
		{
			glm::vec3 WorldPosition;
			glm::vec4 Color;
			glm::vec2 Tex;
			int TextureSlot;
			glm::vec2 TilingFactor;
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
		};

		struct TextVertex
		{
			glm::vec3 WorldPosition;
			glm::vec4 Color;
			glm::vec2 TexCoord;
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
		Ref<ConstantBuffer> m_CBCamera;

		glm::mat4 m_ViewProj;

		struct QuadBatch
		{
			uint64_t VertexOffset = 0;
			uint32_t IndexCount = 0;
			uint32_t VertexCount = 0;
			std::vector<Ref<Texture2D>> Textures;

			QuadBatch(uint64_t dataOffset)
			{
				VertexOffset = dataOffset;
				Textures.reserve(16);
			}
		};

		// Quad
		Ref<RenderPass> m_QuadPass;
		Ref<Pipeline> m_QuadPipeline;
		Ref<VertexBuffer> m_QuadVertexBuffer;
		Ref<IndexBuffer> m_QuadIndexBuffer;
		Buffer m_QuadVertexData;
		std::vector<QuadBatch> m_QuadBatches;
		QuadBatch* m_QuadBatch;
		uint32_t m_QuadIndexCount = 0;

		std::vector<Ref<Material>> m_Materials;

		// Circle
		Ref<RenderPass> m_CirclePass;
		Ref<Pipeline> m_CirclePipeline;
		Ref<VertexBuffer> m_CircleVertexBuffer;
		Ref<IndexBuffer> m_CircleIndexBuffer;
		Buffer m_CircleVertexData;
		uint32_t m_CircleIndexCount = 0;
		uint32_t m_CircleVertexCount = 0;
		
		// Line
		Ref<RenderPass> m_LinePass;
		Ref<Pipeline> m_LinePipeline;
		Ref<VertexBuffer> m_LineVertexBuffer;
		Buffer m_LineVertexData;
		uint32_t m_LineVertexCount = 0;

		// Text
		Ref<RenderPass> m_TextPass;
		Ref<Pipeline> m_TextPipeline;
		Ref<Material> m_TextMaterial;
		Ref<VertexBuffer> m_TextVertexBuffer;
		Ref<IndexBuffer> m_TextIndexBuffer;
		Buffer m_TextVertexData;
		uint32_t m_TextIndexCount = 0;
		uint32_t m_TextVertexCount = 0;

		friend class SceneRenderer;
	};

}