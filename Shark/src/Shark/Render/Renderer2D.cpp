#include "skpch.h"
#include "Renderer2D.h"

#include "Shark/Render/Renderer.h"
#include "Shark/Render/MSDFData.h"

#include "Shark/Math/Math.h"
#include "Shark/Debug/Profiler.h"

#include "Shark/Debug/Profiler.h"

#include <glm/gtc/matrix_transform.hpp>

#if SK_ENABLE_VALIDATION
#define SK_FILL_TEXTURE_ARRAY_DEBUG(texArr, whiteTexture) { auto&& textureArray = (texArr); for (uint32_t i = 0; i < textureArray->Count(); i++) if (!textureArray->Get(i)) textureArray->Set(i, whiteTexture); }
#else
#define SK_FILL_TEXTURE_ARRAY_DEBUG(...)
#endif

namespace Shark {

	Renderer2D::Renderer2D(Ref<FrameBuffer> renderTarget, const Renderer2DSpecifications& specifications)
	{
		Init(renderTarget, specifications);
	}

	Renderer2D::~Renderer2D()
	{
		ShutDown();
	}

	void Renderer2D::Init(Ref<FrameBuffer> framebuffer, const Renderer2DSpecifications& specifications)
	{
		SK_PROFILE_FUNCTION();
		
		m_Specifications = specifications;
		m_GeometryFrameBuffer = framebuffer;

		m_WhiteTexture = Renderer::GetWhiteTexture();

		m_ConstantBufferSet = ConstantBufferSet::Create();
		m_ConstantBufferSet->Create(sizeof(CBCamera), 0);

		m_CommandBuffer = RenderCommandBuffer::Create();

		m_GeometryPassTimer = GPUTimer::Create("Geometry Pass");
		m_OpaqueGeometryPassTimer = GPUTimer::Create("Opaque Geometry Pass");
		m_OITGeoemtryPassTimer = GPUTimer::Create("OIT Geometry Pass");

		// Composite
		{
			PipelineSpecification pipelineSpec;
			pipelineSpec.TargetFrameBuffer = framebuffer;
			pipelineSpec.DebugName = "Renderer2D - Composite";
			pipelineSpec.Shader = Renderer::GetShaderLib()->Get("Renderer2D_Composite");
			pipelineSpec.Layout = {
				{ VertexDataType::Float2, "Position" }
			};
			//pipelineSpec.BackFaceCulling = true;
			pipelineSpec.DepthEnabled = true;
			pipelineSpec.WriteDepth = true;
			pipelineSpec.DepthOperator = DepthCompareOperator::Less;
			m_CompositePipeline = Pipeline::Create(pipelineSpec);
			m_CompositeMaterial = Material::Create(pipelineSpec.Shader);
		}

		{
			FrameBufferSpecification transparentGeometryFramebufferSpec;
			transparentGeometryFramebufferSpec.Width = framebuffer->GetWidth();
			transparentGeometryFramebufferSpec.Height = framebuffer->GetHeight();
			transparentGeometryFramebufferSpec.Atachments = { ImageFormat::RGBA16F, ImageFormat::R16F, ImageFormat::R32_SINT, ImageFormat::Depth32 };
			transparentGeometryFramebufferSpec.Atachments[3].Image = framebuffer->GetDepthImage();
			transparentGeometryFramebufferSpec.ClearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
			transparentGeometryFramebufferSpec.IndipendendClearColor[1] = { 1.0f, 0.0f, 0.0f, 0.0f };
			transparentGeometryFramebufferSpec.IndipendendClearColor[2] = { -1.0f, -1.0f, -1.0f, -1.0f };
			transparentGeometryFramebufferSpec.Atachments[0].Blend.SourceColorFactor = BlendFactor::One;
			transparentGeometryFramebufferSpec.Atachments[0].Blend.DestinationColorFactor = BlendFactor::One;
			transparentGeometryFramebufferSpec.Atachments[1].Blend.SourceColorFactor = BlendFactor::Zero;
			transparentGeometryFramebufferSpec.Atachments[1].Blend.DestinationColorFactor = BlendFactor::InverseSourceColor;
			m_TransparentGeometryFrameBuffer = FrameBuffer::Create(transparentGeometryFramebufferSpec);


			FrameBufferSpecification depthOnlyFramebufferSpec;
			depthOnlyFramebufferSpec.Width = framebuffer->GetWidth();
			depthOnlyFramebufferSpec.Height = framebuffer->GetHeight();
			depthOnlyFramebufferSpec.Atachments = { ImageFormat::Depth32 };
			m_TransparentDepthBuffer = FrameBuffer::Create(depthOnlyFramebufferSpec);

			depthOnlyFramebufferSpec.Atachments[0].Image = framebuffer->GetDepthImage();
			m_DepthFrameBuffer = FrameBuffer::Create(depthOnlyFramebufferSpec);

		}

		VertexLayout quadVertexLayout = {
				{ VertexDataType::Float3, "Position" },
				{ VertexDataType::Float4, "Color" },
				{ VertexDataType::Float2, "TexCoord" },
				{ VertexDataType::Int, "TextureIndex" },
				{ VertexDataType::Float, "TilingFactor" },
				{ VertexDataType::Int, "ID" }
		};

		VertexLayout circleVertexLayout = {
			{ VertexDataType::Float3, "WorldPosition" },
			{ VertexDataType::Float2, "LocalPosition" },
			{ VertexDataType::Float4, "Color" },
			{ VertexDataType::Float, "Thickness" },
			{ VertexDataType::Float, "Fade" },
			{ VertexDataType::Int, "ID" }
		};

		VertexLayout lineVertexLayout = {
				{ VertexDataType::Float3, "Position" },
				{ VertexDataType::Float4, "Color" },
				{ VertexDataType::Int, "ID" }
		};

		// Depth Only Pass
		{
			PipelineSpecification pipelineSpec;
			pipelineSpec.TargetFrameBuffer = m_DepthFrameBuffer;
			pipelineSpec.WriteDepth = true;
			pipelineSpec.DepthOperator = DepthCompareOperator::Less;

			pipelineSpec.Shader = Renderer::GetShaderLib()->Get("Renderer2D_QuadDepthPass");
			pipelineSpec.Layout = quadVertexLayout;
			pipelineSpec.DebugName = "Renderer2D - Quad Depth Only Pass";
			m_QuadDepthPassPipeline = Pipeline::Create(pipelineSpec);
			m_QuadDepthPassMaterial = Material::Create(pipelineSpec.Shader);

			pipelineSpec.Shader = Renderer::GetShaderLib()->Get("Renderer2D_CircleDepthPass");
			pipelineSpec.Layout = circleVertexLayout;
			pipelineSpec.DebugName = "Renderer2D - Circle Depth Only Pass";
			m_CircleDepthPassPipeline = Pipeline::Create(pipelineSpec);
			m_CircleDepthPassMaterial = Material::Create(pipelineSpec.Shader);

			pipelineSpec.Shader = Renderer::GetShaderLib()->Get("Renderer2D_LineDepthPass");
			pipelineSpec.Layout = lineVertexLayout;
			pipelineSpec.DebugName = "Renderer2D - Line Depth Only Pass";
			m_LineDepthPassPipeline = Pipeline::Create(pipelineSpec);
			m_LineDepthPassMaterial = Material::Create(pipelineSpec.Shader);

			pipelineSpec.TargetFrameBuffer = m_TransparentDepthBuffer;
			pipelineSpec.Shader = Renderer::GetShaderLib()->Get("Renderer2D_QuadDepthPass");
			pipelineSpec.Layout = quadVertexLayout;
			pipelineSpec.DebugName = "Renderer2D - Transparent Quad Depth Only Pass";
			m_TransparentQuadDepthPassPipeline = Pipeline::Create(pipelineSpec);
			m_TransparentQuadDepthPassMaterial = Material::Create(pipelineSpec.Shader);

			pipelineSpec.Shader = Renderer::GetShaderLib()->Get("Renderer2D_CircleDepthPass");
			pipelineSpec.Layout = circleVertexLayout;
			pipelineSpec.DebugName = "Renderer2D - Transparent Circle Depth Only Pass";
			m_TransparentCircleDepthPassPipeline = Pipeline::Create(pipelineSpec);
			m_TransparentCircleDepthPassMaterial = Material::Create(pipelineSpec.Shader);
		}


		// Quad
		{
			PipelineSpecification quadPipelineSpecs;
			quadPipelineSpecs.TargetFrameBuffer = framebuffer;
			quadPipelineSpecs.Shader = Renderer::GetShaderLib()->Get("Renderer2D_Quad");
			quadPipelineSpecs.Layout = quadVertexLayout;
			quadPipelineSpecs.DebugName = "Renderer2D-Quad";
			quadPipelineSpecs.DepthEnabled = specifications.UseDepthTesting;
			quadPipelineSpecs.WriteDepth = false;
			quadPipelineSpecs.DepthOperator = DepthCompareOperator::Equal;
			m_QuadPipeline = Pipeline::Create(quadPipelineSpecs);
			m_QuadMaterial = Material::Create(quadPipelineSpecs.Shader);

			m_QuadVertexBuffer = VertexBuffer::Create(quadPipelineSpecs.Layout, DefaultQuadVertices * sizeof(QuadVertex), true, nullptr);

			uint32_t* quadIndices = sknew uint32_t[DefaultQuadIndices];
			for (uint32_t i = 0, j = 0; i < DefaultQuadIndices; i += 6, j += 4)
			{
				quadIndices[i + 0] = j + 0;
				quadIndices[i + 1] = j + 1;
				quadIndices[i + 2] = j + 2;

				quadIndices[i + 3] = j + 2;
				quadIndices[i + 4] = j + 3;
				quadIndices[i + 5] = j + 0;
			}
			m_QuadIndexBuffer = IndexBuffer::Create(DefaultQuadIndices, false, Buffer::FromArray(quadIndices, DefaultQuadIndices));
			skdelete[] quadIndices;

			m_QuadVertexData.Allocate(DefaultQuadVertices * sizeof QuadVertex);
		}
		
		// Circle
		{
			PipelineSpecification circlePipelineSpecs;
			circlePipelineSpecs.TargetFrameBuffer = framebuffer;
			circlePipelineSpecs.Shader = Renderer::GetShaderLib()->Get("Renderer2D_Circle");
			circlePipelineSpecs.Layout = circleVertexLayout;
			circlePipelineSpecs.DebugName = "Renderer2D-Circle";
			circlePipelineSpecs.DepthEnabled = specifications.UseDepthTesting;
			circlePipelineSpecs.WriteDepth = false;
			circlePipelineSpecs.DepthOperator = DepthCompareOperator::Equal;
			m_CirclePipeline = Pipeline::Create(circlePipelineSpecs);
			m_CircleMaterial = Material::Create(circlePipelineSpecs.Shader);

			m_CircleVertexBuffer = VertexBuffer::Create(circlePipelineSpecs.Layout, DefaultCircleVertices * sizeof CircleVertex, true, nullptr);
			m_CircleVertexData.Allocate(DefaultCircleVertices * sizeof CircleVertex);
			m_CircleIndexBuffer = m_QuadIndexBuffer;
		}
		
		// Line
		{
			PipelineSpecification linePipelineSpecs;
			linePipelineSpecs.TargetFrameBuffer = framebuffer;
			linePipelineSpecs.Shader = Renderer::GetShaderLib()->Get("Renderer2D_Line");
			linePipelineSpecs.Layout = lineVertexLayout;
			linePipelineSpecs.DebugName = "Renderer2D-Line";
			linePipelineSpecs.Primitve = PrimitveType::Line;
			linePipelineSpecs.DepthEnabled = specifications.UseDepthTesting;
			linePipelineSpecs.WriteDepth = false;
			linePipelineSpecs.DepthOperator = DepthCompareOperator::Equal;
			m_LinePipeline = Pipeline::Create(linePipelineSpecs);
			m_LineMaterial = Material::Create(linePipelineSpecs.Shader);
			
			m_LineVertexBuffer = VertexBuffer::Create(linePipelineSpecs.Layout, DefaultLineVertices * sizeof LineVertex, true, nullptr);
			m_LineVertexData.Allocate(DefaultLineVertices * sizeof LineVertex);
		}


		// Transparent Quad
		{
			PipelineSpecification pipelineSpec;
			pipelineSpec.TargetFrameBuffer = m_TransparentGeometryFrameBuffer;
			pipelineSpec.Shader = Renderer::GetShaderLib()->Get("Renderer2D_QuadTransparent");
			pipelineSpec.Layout = quadVertexLayout;
			pipelineSpec.DebugName = "Renderer2D Quad Transparent";
			pipelineSpec.DepthEnabled = specifications.UseDepthTesting;
			pipelineSpec.WriteDepth = false;
			pipelineSpec.DepthOperator = DepthCompareOperator::Less;
			m_TransparentQuadPipeline = Pipeline::Create(pipelineSpec);
			m_TransparentQuadMaterial = Material::Create(pipelineSpec.Shader);

			m_TransparentQuadVertexBuffer = VertexBuffer::Create(pipelineSpec.Layout, DefaultQuadVertices * sizeof QuadVertex, true, nullptr);
			m_TransparentQuadVertexData.Allocate(DefaultQuadVertices * sizeof QuadVertex);
			m_TransparentQuadIndexBuffer = m_QuadIndexBuffer;
		}

		// Transparent Circle
		{
			PipelineSpecification pipelineSpec;
			pipelineSpec.TargetFrameBuffer = m_TransparentGeometryFrameBuffer;
			pipelineSpec.Shader = Renderer::GetShaderLib()->Get("Renderer2D_CircleTransparent");
			pipelineSpec.Layout = circleVertexLayout;
			pipelineSpec.DebugName = "Renderer2D Circle Transparent";
			pipelineSpec.DepthEnabled = specifications.UseDepthTesting;
			pipelineSpec.WriteDepth = false;
			pipelineSpec.DepthOperator = DepthCompareOperator::Less;
			m_TransparentCirclePipeline = Pipeline::Create(pipelineSpec);
			m_TransparentCircleMaterial = Material::Create(pipelineSpec.Shader);

			m_TransparentCircleVertexBuffer = VertexBuffer::Create(pipelineSpec.Layout, DefaultCircleVertices * sizeof CircleVertex, true, nullptr);
			m_TransparentCircleVertexData.Allocate(DefaultCircleVertices * sizeof CircleVertex);
			m_TransparentCircleIndexBuffer = m_TransparentQuadIndexBuffer;
		}

		// Text
		{
			PipelineSpecification pipelineSpec;
			pipelineSpec.TargetFrameBuffer = framebuffer;
			pipelineSpec.Shader = Renderer::GetShaderLib()->Get("Renderer2D_Text");
			pipelineSpec.Layout = {
				{ VertexDataType::Float3, "Position" },
				{ VertexDataType::Float4, "Color" },
				{ VertexDataType::Float2, "TexCoord" },
				{ VertexDataType::Int, "ID" }
			};
			pipelineSpec.DebugName = "Renderer2D-Text";
			pipelineSpec.DepthEnabled = specifications.UseDepthTesting;
			pipelineSpec.WriteDepth = true;
			pipelineSpec.DepthOperator = DepthCompareOperator::Less;
			m_TextPipeline = Pipeline::Create(pipelineSpec);
			m_TextMaterial = Material::Create(pipelineSpec.Shader);

			m_TextVertexBuffer = VertexBuffer::Create(pipelineSpec.Layout, DefaultTextVertices * sizeof(TextVertex), true, nullptr);
			m_TextIndexBuffer = m_QuadIndexBuffer;

			m_TextVertexData.Allocate(DefaultTextVertices * sizeof TextVertex);
		}


		constexpr double delta = M_PI / 10.0f; // 0.31415
		glm::vec4 point = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		for (uint32_t i = 0; i < 20; i++)
		{
			const double r0 = (double)i * delta;
			point.x = (float)glm::sin(r0) * 0.5f;
			point.y = (float)glm::cos(r0) * 0.5f;

			m_CirlceVertexPositions[i] = point;
		}

	}

	void Renderer2D::ShutDown()
	{
		m_QuadVertexData.Release();
		m_CircleVertexData.Release();
		m_LineVertexData.Release();
		m_TextVertexData.Release();
		m_TransparentQuadVertexData.Release();
		m_TransparentCircleVertexData.Release();
	}

	void Renderer2D::Resize(uint32_t width, uint32_t height)
	{
		m_DepthFrameBuffer->Resize(width, height);
		m_TransparentDepthBuffer->Resize(width, height);
		m_TransparentGeometryFrameBuffer->Resize(width, height);
	}

	void Renderer2D::BeginScene(const glm::mat4& viewProj)
	{
		SK_PROFILE_FUNCTION();
		
		SK_CORE_VERIFY(!m_Active);

		m_Active = true;

		m_ViewProj = viewProj;
		CBCamera cam{ viewProj };
		m_ConstantBufferSet->Get(0)->Set(&cam, sizeof(CBCamera));

		// Quad
		m_QuadBatches.clear();
		m_QuadIndexCount = 0;
		m_QuadBatch = &m_QuadBatches.emplace_back(0);
		
		// Transparent Quad
		m_TransparentQuadBatches.clear();
		m_TransparentQuadIndexCount = 0;
		m_TransparentQuadBatch = &m_TransparentQuadBatches.emplace_back(0);
		
		// Circle
		m_CircleIndexCount = 0;
		m_CircleVertexCount = 0;

		// Transparent Circle
		m_TransparentCircleIndexCount = 0;
		m_TransparentCircleVertexCount = 0;

		// Line
		m_LineVertexCount = 0;

		// Text
		m_TextIndexCount = 0;
		m_TextVertexCount = 0;

		memset(&m_Statistics, 0, sizeof(m_Statistics));
	}

	void Renderer2D::EndScene()
	{
		SK_PROFILE_FUNCTION();

		SK_CORE_VERIFY(m_Active);

		m_CommandBuffer->Begin();
		m_CommandBuffer->BeginTimeQuery(m_GeometryPassTimer);

		ClearPass();
		OpaqueGeometryPass();
		OITGeometryPass();

		m_CommandBuffer->EndTimeQuery(m_GeometryPassTimer);
		m_CommandBuffer->End();
		m_CommandBuffer->Execute();

		m_Statistics.GeometryPassTime += m_GeometryPassTimer->GetTime();

		m_Active = false;
	}

	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& scaling, const glm::vec4& color, int id)
	{
		DrawQuad({ position.x, position.y, 0.0f }, { scaling.x, scaling.y, 1.0f }, color, id);
	}

	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec3& scaling, const glm::vec4& color, int id)
	{
		DrawQuad(position, scaling, m_WhiteTexture, 1.0f, color, id);
	}

	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& scaling, const Ref<Texture2D>& texture, float tilingfactor, const glm::vec4& tintcolor, int id)
	{
		DrawQuad({ position.x, position.y, 0.0f }, { scaling.x, scaling.y, 1.0f }, texture, tilingfactor, tintcolor, id);
	}

	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec3& scaling, const Ref<Texture2D>& texture, float tilingfactor, const glm::vec4& tintcolor, int id)
	{
		const glm::mat4 transform =
			glm::translate(glm::mat4(1), position) *
			glm::scale(glm::mat4(1), scaling);

		DrawQuad(transform, texture, tilingfactor, tintcolor, id);
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec2& position, float rotation, const glm::vec2& scaling, const glm::vec4& color, int id)
	{
		DrawRotatedQuad({ position.x, position.y, 0.0f }, { 0.0f, 0.0f, rotation }, { scaling.x, scaling.y, 1.0f }, color, id);
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scaling, const glm::vec4& color, int id)
	{
		DrawRotatedQuad(position, rotation, scaling, m_WhiteTexture, 1.0f, color, id);
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec2& position, float rotation, const glm::vec2& scaling, const Ref<Texture2D>& texture, float tilingfactor, const glm::vec4& tintcolor, int id)
	{
		DrawRotatedQuad({ position.x, position.y, 0.0f }, { 0.0f, 0.0f, rotation }, { scaling.x, scaling.y, 1.0f }, texture, tilingfactor, tintcolor, id);
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scaling, const Ref<Texture2D>& texture, float tilingfactor, const glm::vec4& tintcolor, int id)
	{
		const glm::mat4 transform =
			glm::translate(glm::mat4(1), position) *
			glm::toMat4(glm::quat(rotation)) *
			glm::scale(glm::mat4(1), scaling);

		DrawQuad(transform, texture, tilingfactor, tintcolor, id);
	}

	void Renderer2D::DrawQuad(const glm::mat4& transform, const glm::vec4& color, int id)
	{
		DrawQuad(transform, m_WhiteTexture, 1.0f, color, id);
	}

	void Renderer2D::DrawQuad(const glm::mat4& transform, Ref<Texture2D> texture, float tilingfactor, const glm::vec4& color, int id)
	{
		SK_CORE_VERIFY(m_Active);

		if (m_QuadBatch->Textures.size() > MaxTextureSlots)
			BeginQaudBatch();

		auto& batch = *m_QuadBatch;
		uint32_t textureSlot = AddTexture(&batch, texture ? texture : m_WhiteTexture);

		AssureQuadVertexDataSize();

		QuadVertex* memory = m_QuadVertexData.Offset<QuadVertex>(batch.VertexOffset + batch.VertexCount);
		for (uint32_t i = 0; i < 4; i++)
		{
			memory->WorldPosition = transform * m_QuadVertexPositions[i];
			memory->Color = color;
			memory->Tex = m_TextureCoords[i];
			memory->TextureSlot = textureSlot;
			memory->TilingFactor = tilingfactor;
			memory->ID = id;
			memory++;
		}

		batch.VertexCount += 4;
		batch.IndexCount += 6;
		m_QuadIndexCount += 6;

		m_Statistics.QuadCount++;
		m_Statistics.VertexCount += 4;
		m_Statistics.IndexCount += 6;
	}

	void Renderer2D::DrawQuadTransparent(const glm::mat4& transform, Ref<Texture2D> texture, float tilingfactor, const glm::vec4& color, int id)
	{
		SK_CORE_VERIFY(m_Active);

		if (m_TransparentQuadBatch->Textures.size() > MaxTextureSlots)
			BeginQaudBatch();

		auto& batch = *m_TransparentQuadBatch;
		uint32_t textureSlot = AddTexture(&batch, texture ? texture : m_WhiteTexture);

		AssureTransparentQuadVertexDataSize();

		QuadVertex* memory = m_TransparentQuadVertexData.Offset<QuadVertex>(batch.VertexOffset + batch.VertexCount);
		for (uint32_t i = 0; i < 4; i++)
		{
			memory->WorldPosition = transform * m_QuadVertexPositions[i];
			memory->Color = color;
			memory->Tex = m_TextureCoords[i];
			memory->TextureSlot = textureSlot;
			memory->TilingFactor = tilingfactor;
			memory->ID = id;
			memory++;
		}

		batch.VertexCount += 4;
		batch.IndexCount += 6;
		m_TransparentQuadIndexCount += 6;

		m_Statistics.QuadCount++;
		m_Statistics.VertexCount += 4;
		m_Statistics.IndexCount += 6;
	}

	void Renderer2D::DrawFilledCircle(const glm::vec2& position, const glm::vec2& scaling, const glm::vec4& color, float thickness, float fade, int id)
	{
		DrawFilledCircle({ position.x, position.y, 0.0f }, { scaling.x, scaling.y, 1.0f }, color, thickness, fade, id);
	}

	void Renderer2D::DrawFilledCircle(const glm::vec3& position, const glm::vec3& scaling, const glm::vec4& color, float thickness, float fade, int id)
	{
		const glm::mat4 transform =
			glm::translate(glm::mat4(1), position) *
			glm::scale(glm::mat4(1), scaling);

		DrawFilledCircle(transform, color, thickness, fade, id);
	}

	void Renderer2D::DrawFilledCircle(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scaling, const glm::vec4& color, float thickness, float fade, int id)
	{
		const glm::mat4 transform =
			glm::translate(glm::mat4(1), position) *
			glm::toMat4(glm::quat(rotation)) /*glm::eulerAngleXYZ(rotation.x, rotation.y, rotation.z)*/ *
			glm::scale(glm::mat4(1), scaling);

		DrawFilledCircle(transform, color, thickness, fade, id);
	}

	void Renderer2D::DrawFilledCircle(const glm::mat4& transform, const glm::vec4& color, float thickness, float fade, int id)
	{
		SK_CORE_VERIFY(m_Active);

		AssureCircleVertexDataSize();

		CircleVertex* memory = m_CircleVertexData.Offset<CircleVertex>(m_CircleVertexCount);
		for (uint32_t i = 0; i < 4; i++)
		{
			memory->WorldPosition = transform * m_QuadVertexPositions[i];
			memory->LocalPosition = m_QuadVertexPositions[i] * 2.0f;
			memory->Color = color;
			memory->Thickness = thickness;
			memory->Fade = fade;
			memory->ID = id;
			memory++;
		}

		m_CircleVertexCount += 4;
		m_CircleIndexCount += 6;

		m_Statistics.CircleCount++;
		m_Statistics.VertexCount += 4;
		m_Statistics.IndexCount += 6;
	}

	void Renderer2D::DrawFilledCircleTransparent(const glm::mat4& transform, const glm::vec4& color, float thickness, float fade, int id)
	{
		SK_CORE_VERIFY(m_Active);

		AssureTransparentCircleVertexDataSize();

		CircleVertex* memory = m_TransparentCircleVertexData.Offset<CircleVertex>(m_TransparentCircleVertexCount);
		for (uint32_t i = 0; i < 4; i++)
		{
			memory->WorldPosition = transform * m_QuadVertexPositions[i];
			memory->LocalPosition = m_QuadVertexPositions[i] * 2.0f;
			memory->Color = color;
			memory->Thickness = thickness;
			memory->Fade = fade;
			memory->ID = id;
			memory++;
		}

		m_TransparentCircleVertexCount += 4;
		m_TransparentCircleIndexCount += 6;

		m_Statistics.CircleCount++;
		m_Statistics.VertexCount += 4;
		m_Statistics.IndexCount += 6;
	}

	void Renderer2D::DrawCircle(const glm::vec2& position, float radius, const glm::vec4& color, int id)
	{
		const glm::mat4 transform =
			glm::translate(glm::mat4(1), glm::vec3(position, 0.0f)) *
			glm::scale(glm::mat4(1), glm::vec3(radius));

		DrawCircle(transform, color, id);
	}

	void Renderer2D::DrawCircle(const glm::vec3& position, const glm::vec3& rotation, float radius, const glm::vec4& color, int id)
	{
		const glm::mat4 transform =
			glm::translate(glm::mat4(1), position) *
			glm::toMat4(glm::quat(rotation)) /*glm::eulerAngleXYZ(rotation.x, rotation.y, rotation.z)*/ *
			glm::scale(glm::mat4(1), glm::vec3(radius));

		DrawCircle(transform, color, id);
	}

	void Renderer2D::DrawCircle(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale, const glm::vec4& color, int id)
	{
		const glm::mat4 transform =
			glm::translate(glm::mat4(1), position) *
			glm::toMat4(glm::quat(rotation)) /*glm::eulerAngleXYZ(rotation.x, rotation.y, rotation.z)*/ *
			glm::scale(glm::mat4(1), scale);

		DrawCircle(transform, color, id);
	}

	void Renderer2D::DrawCircle(const glm::mat4& transform, const glm::vec4& color, int id)
	{
		SK_CORE_VERIFY(m_Active);
		for (uint32_t i = 0; i < m_CirlceVertexPositions.size() - 1; i++)
		{
			glm::vec3 p0 = (transform * m_CirlceVertexPositions[i + 0]).xyz;
			glm::vec3 p1 = (transform * m_CirlceVertexPositions[i + 1]).xyz;
			DrawLine(p0, p1, color, id);
		}
		glm::vec3 p0 = (transform * m_CirlceVertexPositions.back()).xyz;
		glm::vec3 p1 = (transform * m_CirlceVertexPositions.front()).xyz;
		DrawLine(p0, p1, color, id);
	}

	void Renderer2D::DrawLine(const glm::vec2& pos0, const glm::vec2& pos1, const glm::vec4& color, int id)
	{
		DrawLine({ pos0.x, pos0.y, 0.0f }, { pos1.x, pos1.y, 0.0f }, color, id);
	}

	void Renderer2D::DrawLine(const glm::vec3& pos0, const glm::vec3& pos1, const glm::vec4& color, int id)
	{
		SK_CORE_VERIFY(m_Active);

		AssureLineVertexDataSize();

		LineVertex* memory = m_LineVertexData.Offset<LineVertex>(m_LineVertexCount);
		memory->WorldPosition = pos0;
		memory->Color = color;
		memory->ID = id;

		memory++;
		memory->WorldPosition = pos1;
		memory->Color = color;
		memory->ID = id;

		m_LineVertexCount += 2;
		
		m_Statistics.LineCount++;
		m_Statistics.VertexCount += 2;
	}


	void Renderer2D::DrawRect(const glm::vec2& position, const glm::vec2& scaling, const glm::vec4& color, int id)
	{
		DrawRect({ position.x, position.y, 0.0f }, { scaling.x, scaling.y, 1.0f }, color, id);
	}

	void Renderer2D::DrawRect(const glm::vec3& position, const glm::vec3& scaling, const glm::vec4& color, int id)
	{
		const glm::mat4 transform =
			glm::translate(glm::mat4(1), position) *
			glm::scale(glm::mat4(1), scaling);

		DrawRect(transform, color, id);
	}

	void Renderer2D::DrawRect(const glm::vec2& position, float rotation, const glm::vec2& scaling, const glm::vec4& color, int id)
	{
		DrawRect({ position.x, position.y, 0.0f }, { 0.0f, 0.0f, rotation }, { scaling.x, scaling.y, 1.0f }, color, id);
	}

	void Renderer2D::DrawRect(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scaling, const glm::vec4& color, int id)
	{
		const glm::mat4 transform =
			glm::translate(glm::mat4(1), position) *
			glm::toMat4(glm::quat(rotation)) /*glm::eulerAngleXYZ(rotation.x, rotation.y, rotation.z)*/ *
			glm::scale(glm::mat4(1), scaling);

		DrawRect(transform, color, id);
	}

	void Renderer2D::DrawRect(const glm::mat4& transform, const glm::vec4& color, int id)
	{
		SK_CORE_VERIFY(m_Active);

		glm::vec3 p0 = transform * m_QuadVertexPositions[0];
		glm::vec3 p1 = transform * m_QuadVertexPositions[1];
		glm::vec3 p2 = transform * m_QuadVertexPositions[2];
		glm::vec3 p3 = transform * m_QuadVertexPositions[3];

		DrawLine(p0, p1, color, id);
		DrawLine(p1, p2, color, id);
		DrawLine(p2, p3, color, id);
		DrawLine(p3, p0, color, id);
	}

	void Renderer2D::DrawString(const std::string& string, Ref<Font> font, const glm::mat4& transform, float kerning, float lineSpacing, const glm::vec4& color, int id)
	{
		SK_CORE_VERIFY(m_Active);

		const auto& fontGeometry = font->GetMSDFData()->FontGeometry;
		const auto& metrics = fontGeometry.getMetrics();
		Ref<Texture2D> fontAtlas = font->GetFontAtlas();

		float x = 0.0f;
		float y = 0.0f;
		float fsScale = 1.0f / (metrics.ascenderY - metrics.descenderY);
		
		std::vector<uint32_t> unicodeString;
		msdf_atlas::utf8Decode(unicodeString, string.c_str());

		AssureTextVertexDataSize(unicodeString.size());
		m_TextMaterial->SetTexture("g_FontAtlas", fontAtlas);

		for (size_t index = 0; index < unicodeString.size(); index++)
		{
			const char32_t character = unicodeString[index];
			const char32_t nextCharacter = (index + 1) >= unicodeString.size() ? ' ' : unicodeString[index + 1];

			if (character == '\n')
			{
				x = 0.0f;
				y -= metrics.lineHeight + lineSpacing;
				continue;
			}

			auto glyph = fontGeometry.getGlyph(character);
			if (!glyph)
				glyph = fontGeometry.getGlyph('?');
			if (!glyph)
				continue;

			double al, ab, ar, at;
			glyph->getQuadAtlasBounds(al, ab, ar, at);
			glm::vec2 textCoordMin = glm::vec2(al, ab);
			glm::vec2 textCoordMax = glm::vec2(ar, at);

			double pl, pb, pr, pt;
			glyph->getQuadPlaneBounds(pl, pb, pr, pt);
			glm::vec2 quadMin = glm::vec2(pl, pb);
			glm::vec2 quadMax = glm::vec2(pr, pt);

			quadMin *= fsScale;
			quadMax *= fsScale;
			quadMin += glm::vec2(x, y);
			quadMax += glm::vec2(x, y);

			float texelWidth = 1.0f / fontAtlas->GetWidth();
			float texelHeight = 1.0f / fontAtlas->GetHeight();
			textCoordMin *= glm::vec2(texelWidth, texelHeight);
			textCoordMax *= glm::vec2(texelWidth, texelHeight);

			double advance = glyph->getAdvance();
			fontGeometry.getAdvance(advance, character, nextCharacter);

			x += fsScale * advance + kerning;

			// Render
			const std::array<glm::vec2, 4> quadPositions = { glm::vec2(quadMin.x, quadMax.y), quadMax, glm::vec2(quadMax.x,quadMin.y), quadMin };
			const std::array<glm::vec2, 4> textCoords = { glm::vec2(textCoordMin.x, textCoordMax.y), textCoordMax, glm::vec2(textCoordMax.x,textCoordMin.y), textCoordMin };

			TextVertex* vertex = m_TextVertexData.Offset<TextVertex>(m_TextVertexCount);
			for (uint32_t i = 0; i < 4; i++)
			{
				vertex->WorldPosition = transform * glm::vec4(quadPositions[i], 0.0f, 1.0f);
				vertex->Color = color;
				vertex->TexCoord = textCoords[i];
				vertex->ID = id;
				vertex++;
			}

			m_TextVertexCount += 4;
			m_TextIndexCount += 6;

			m_Statistics.GlyphCount++;
			m_Statistics.VertexCount += 4;
			m_Statistics.IndexCount += 6;
		}

	}

	void Renderer2D::ClearPass()
	{
		//m_DepthFrameBuffer->Clear(m_CommandBuffer);
		m_TransparentDepthBuffer->Clear(m_CommandBuffer);
		m_TransparentGeometryFrameBuffer->ClearColorAtachments(m_CommandBuffer);
	}

	void Renderer2D::OpaqueGeometryPass()
	{
		m_CommandBuffer->BeginTimeQuery(m_OpaqueGeometryPassTimer);

		if (m_QuadIndexCount)
		{
			if (m_QuadIndexBuffer->GetCount() < m_QuadIndexCount)
				ResizeQuadIndexBuffer(m_QuadIndexCount);

			m_QuadVertexBuffer->SetData(m_QuadVertexData, true);
			Renderer::RenderGeometry(m_CommandBuffer, m_QuadDepthPassPipeline, m_QuadDepthPassMaterial, m_ConstantBufferSet, m_QuadVertexBuffer, m_QuadIndexBuffer, m_QuadIndexCount);
			m_Statistics.DrawCalls++;
		}

		if (m_CircleIndexCount)
		{
			if (m_CircleIndexBuffer->GetCount() < m_CircleIndexCount)
				ResizeQuadIndexBuffer(m_CircleIndexCount);

			m_CircleVertexBuffer->SetData(m_CircleVertexData, true);
			Renderer::RenderGeometry(m_CommandBuffer, m_CircleDepthPassPipeline, m_CircleDepthPassMaterial, m_ConstantBufferSet, m_CircleVertexBuffer, m_CircleIndexBuffer, m_CircleIndexCount);
			m_Statistics.DrawCalls++;
		}

		if (m_LineVertexCount)
		{
			m_LineVertexBuffer->SetData(m_LineVertexData, true);
			Renderer::RenderGeometry(m_CommandBuffer, m_LineDepthPassPipeline, m_LineDepthPassMaterial, m_ConstantBufferSet, m_LineVertexBuffer, m_LineVertexCount);
			m_Statistics.DrawCalls++;
		}


		if (m_QuadIndexCount)
		{
			Renderer::BeginBatch(m_CommandBuffer, m_QuadPipeline, m_QuadVertexBuffer, m_QuadIndexBuffer);
			uint32_t indexOffset = 0;
			for (const auto& batch : m_QuadBatches)
			{
				PrepareMaterial(m_QuadMaterial, batch);
				Renderer::RenderBatch(m_CommandBuffer, m_QuadMaterial, m_ConstantBufferSet, batch.IndexCount, indexOffset);
				indexOffset += batch.IndexCount;
				m_Statistics.DrawCalls++;
			}
			Renderer::EndBatch(m_CommandBuffer);
		}

		if (m_CircleIndexCount)
		{
			Renderer::RenderGeometry(m_CommandBuffer, m_CirclePipeline, m_CircleMaterial, m_ConstantBufferSet, m_CircleVertexBuffer, m_CircleIndexBuffer, m_CircleIndexCount);
			m_Statistics.DrawCalls++;
		}

		if (m_LineVertexCount)
		{
			Renderer::RenderGeometry(m_CommandBuffer, m_LinePipeline, m_LineMaterial, m_ConstantBufferSet, m_LineVertexBuffer, m_LineVertexCount);
			m_Statistics.DrawCalls++;
		}

		if (m_TextIndexCount)
		{
			if (m_TextIndexBuffer->GetCount() < m_TextIndexCount)
				ResizeQuadIndexBuffer(m_TextIndexCount);

			m_TextVertexBuffer->SetData(m_TextVertexData, true);
			Renderer::RenderGeometry(m_CommandBuffer, m_TextPipeline, m_TextMaterial, m_ConstantBufferSet, m_TextVertexBuffer, m_TextIndexBuffer, m_TextIndexCount);
			m_Statistics.DrawCalls++;
		}

		m_CommandBuffer->EndTimeQuery(m_OpaqueGeometryPassTimer);
	}

	void Renderer2D::OITGeometryPass()
	{
		m_CommandBuffer->BeginTimeQuery(m_OITGeoemtryPassTimer);

		if (m_TransparentQuadIndexCount)
		{
			if (m_TransparentQuadIndexBuffer->GetCount() < m_TransparentQuadIndexCount)
				ResizeQuadIndexBuffer(m_TransparentQuadIndexCount);

			m_TransparentQuadVertexBuffer->SetData(m_TransparentQuadVertexData, true);
			Renderer::RenderGeometry(m_CommandBuffer, m_TransparentQuadDepthPassPipeline, m_TransparentQuadDepthPassMaterial, m_ConstantBufferSet, m_TransparentQuadVertexBuffer, m_TransparentQuadIndexBuffer, m_TransparentQuadIndexCount);
			m_Statistics.DrawCalls++;
		}

		if (m_TransparentCircleIndexCount)
		{
			if (m_TransparentCircleIndexBuffer->GetCount() < m_TransparentCircleIndexCount)
				ResizeQuadIndexBuffer(m_TransparentCircleIndexCount);

			m_TransparentCircleVertexBuffer->SetData(m_TransparentCircleVertexData, true);
			Renderer::RenderGeometry(m_CommandBuffer, m_TransparentCircleDepthPassPipeline, m_TransparentCircleDepthPassMaterial, m_ConstantBufferSet, m_TransparentCircleVertexBuffer, m_TransparentCircleIndexBuffer, m_TransparentCircleIndexCount);
			m_Statistics.DrawCalls++;
		}

		if (m_TransparentQuadIndexCount)
		{
			Renderer::BeginBatch(m_CommandBuffer, m_TransparentQuadPipeline, m_TransparentQuadVertexBuffer, m_TransparentQuadIndexBuffer);
			uint32_t indexOffset = 0;
			for (const auto& batch : m_TransparentQuadBatches)
			{
				PrepareMaterial(m_TransparentQuadMaterial, batch);
				Renderer::RenderBatch(m_CommandBuffer, m_TransparentQuadMaterial, m_ConstantBufferSet, batch.IndexCount, indexOffset);
				indexOffset += batch.IndexCount;
				m_Statistics.DrawCalls++;
			}
			Renderer::EndBatch(m_CommandBuffer);
		}

		if (m_TransparentCircleIndexCount)
		{
			Renderer::RenderGeometry(m_CommandBuffer, m_TransparentCirclePipeline, m_TransparentCircleMaterial, m_ConstantBufferSet, m_TransparentCircleVertexBuffer, m_TransparentCircleIndexBuffer, m_TransparentCircleIndexCount);
			m_Statistics.DrawCalls++;
		}

		m_CompositeMaterial->SetImage("AccumulationImage", m_TransparentGeometryFrameBuffer->GetImage(0), 0);
		m_CompositeMaterial->SetImage("RevealImage", m_TransparentGeometryFrameBuffer->GetImage(1), 1);
		m_CompositeMaterial->SetImage("IDImage", m_TransparentGeometryFrameBuffer->GetImage(2), 2);
		m_CompositeMaterial->SetImage("DepthImage", m_TransparentDepthBuffer->GetDepthImage(), 3);
		Renderer::RenderFullScreenQuad(m_CommandBuffer, m_CompositePipeline, m_CompositeMaterial);
		m_Statistics.DrawCalls++;

		m_CommandBuffer->EndTimeQuery(m_OITGeoemtryPassTimer);
	}

	void Renderer2D::AssureQuadVertexDataSize()
	{
		if (m_QuadBatch->VertexOffset + m_QuadBatch->VertexCount >= m_QuadVertexData.Count<QuadVertex>())
			m_QuadVertexData.Resize(m_QuadVertexData.Size * 2);
	}

	void Renderer2D::AssureTransparentQuadVertexDataSize()
	{
		if (m_TransparentQuadBatch->VertexOffset + m_TransparentQuadBatch->VertexCount >= m_TransparentQuadVertexData.Count<QuadVertex>())
			m_TransparentQuadVertexData.Resize(m_TransparentQuadVertexData.Size * 2);
	}

	void Renderer2D::AssureCircleVertexDataSize()
	{
		if (m_CircleVertexCount >= m_CircleVertexData.Count<CircleVertex>())
			m_CircleVertexData.Resize(m_CircleVertexData.Size * 2);
	}

	void Renderer2D::AssureTransparentCircleVertexDataSize()
	{
		if (m_TransparentCircleVertexCount >= m_TransparentCircleVertexData.Count<CircleVertex>())
			m_TransparentCircleVertexData.Resize(m_TransparentCircleVertexData.Size * 2);
	}

	void Renderer2D::AssureLineVertexDataSize()
	{
		if (m_LineVertexCount >= m_LineVertexData.Count<LineVertex>())
			m_LineVertexData.Resize(m_LineVertexData.Size * 2);
	}

	void Renderer2D::AssureTextVertexDataSize(uint32_t glyphCount)
	{
		if ((m_TextVertexCount + glyphCount * 4) >= m_TextVertexData.Count<TextVertex>())
			m_TextVertexData.Resize(std::max<uint64_t>(m_TextVertexData.Size * 2, m_TextVertexCount + glyphCount * sizeof TextVertex * 4));
	}

	void Renderer2D::BeginQaudBatch()
	{
		m_QuadBatch = &m_QuadBatches.emplace_back(m_QuadBatch->VertexOffset + m_QuadBatch->VertexCount);
	}

	uint32_t Renderer2D::AddTexture(QuadBatch* batch, Ref<Texture2D> texture)
	{
		SK_CORE_VERIFY(texture);

		uint32_t index = 0;
		for (const auto& tex : batch->Textures)
		{
			if (tex == texture)
				return index;

			index++;
		}

		SK_CORE_ASSERT(batch->Textures.size() == index);
		SK_CORE_ASSERT(batch->Textures.size() < 16);
		batch->Textures.push_back(texture);
		m_Statistics.TextureCount++;
		return index;
	}

	void Renderer2D::PrepareMaterial(Ref<Material> material, const QuadBatch& batch)
	{
		if (!material->HasResource("g_Textures"))
		{
			SK_CORE_ERROR_TAG("Renderer", "Material has no Resource Named g_Textures!");
			return;
		}

		uint32_t index = 0;
		auto array = material->GetTextureArray("g_Textures");
		for (const auto& texture : batch.Textures)
			array->Set(index++, texture);

		for (; index < array->Count(); index++)
			array->Set(index, m_WhiteTexture);
	}

	void Renderer2D::ResizeQuadIndexBuffer(uint64_t indexCount)
	{
		SK_CORE_VERIFY((indexCount % 6) == 0);
		Renderer::Submit([indexBuffer = m_QuadIndexBuffer, indexCount]()
		{
			uint32_t* quadIndices = sknew uint32_t[indexCount];
			for (uint32_t i = 0, j = 0; i < indexCount; i += 6, j += 4)
			{
				quadIndices[i + 0] = j + 0;
				quadIndices[i + 1] = j + 1;
				quadIndices[i + 2] = j + 2;

				quadIndices[i + 3] = j + 2;
				quadIndices[i + 4] = j + 3;
				quadIndices[i + 5] = j + 0;
			}
			indexBuffer->RT_Resize(indexCount, { quadIndices, indexCount * sizeof uint32_t });
			skdelete[] quadIndices;
		});
	}

}
