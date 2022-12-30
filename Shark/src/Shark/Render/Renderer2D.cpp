#include "skpch.h"
#include "Renderer2D.h"

#include "Shark/Render/Renderer.h"

#include "Shark/Math/Math.h"
#include "Shark/Debug/Profiler.h"

#include "Platform/DirectX11/DirectXRenderCommandBuffer.h"
#include "Platform/DirectX11/DirectXTexture.h"

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

		m_WhiteTexture = Renderer::GetWhiteTexture();

		m_ConstantBufferSet = ConstantBufferSet::Create();
		m_ConstantBufferSet->Create(sizeof(CBCamera), 0);

		m_CommandBuffer = RenderCommandBuffer::Create();

		m_GeometryPassTimer = GPUTimer::Create();
		m_QuadFlushQuery = GPUTimer::Create();

		// Depth Only Pass
		{
			FrameBufferSpecification depthOnlyFramebufferSpec;
			depthOnlyFramebufferSpec.Width = framebuffer->GetWidth();
			depthOnlyFramebufferSpec.Height = framebuffer->GetHeight();
			depthOnlyFramebufferSpec.Atachments = { ImageFormat::Depth32 };
			depthOnlyFramebufferSpec.Atachments[0].Image = framebuffer->GetDepthImage();
			m_DepthFrameBuffer = FrameBuffer::Create(depthOnlyFramebufferSpec);

			PipelineSpecification pipelineSpec;
			pipelineSpec.TargetFrameBuffer = m_DepthFrameBuffer;
			pipelineSpec.WriteDepth = true;
			pipelineSpec.DepthOperator = DepthCompareOperator::LessEqual;

			pipelineSpec.Shader = Renderer::GetShaderLib()->Get("Renderer2D_QuadDepthPass");
			pipelineSpec.DebugName = "Renderer2D - Quad Depth Only Pass";
			m_QuadDepthPassPipeline = Pipeline::Create(pipelineSpec);
			m_QuadDepthPassMaterial = Material::Create(pipelineSpec.Shader);

			pipelineSpec.Shader = Renderer::GetShaderLib()->Get("Renderer2D_CircleDepthPass");
			pipelineSpec.DebugName = "Renderer2D - Circle Depth Only Pass";
			m_CircleDepthPassPipeline = Pipeline::Create(pipelineSpec);
			m_CircleDepthPassMaterial = Material::Create(pipelineSpec.Shader);

			pipelineSpec.Shader = Renderer::GetShaderLib()->Get("Renderer2D_LineDepthPass");
			pipelineSpec.DebugName = "Renderer2D - Line Depth Only Pass";
			m_LineDepthPassPipeline = Pipeline::Create(pipelineSpec);
			m_LineDepthPassMaterial = Material::Create(pipelineSpec.Shader);
		}

		// Quad
		{
			PipelineSpecification quadPipelineSpecs;
			quadPipelineSpecs.TargetFrameBuffer = framebuffer;
			quadPipelineSpecs.Shader = Renderer::GetShaderLib()->Get("Renderer2D_Quad");
			quadPipelineSpecs.DebugName = "Renderer2D-Quad";
			quadPipelineSpecs.DepthEnabled = specifications.UseDepthTesting;
			quadPipelineSpecs.WriteDepth = false;
			quadPipelineSpecs.DepthOperator = DepthCompareOperator::Equal;
			m_QuadPipeline = Pipeline::Create(quadPipelineSpecs);
			m_QuadMaterial = Material::Create(quadPipelineSpecs.Shader);

			m_QuadVertexBuffer = VertexBuffer::Create(quadPipelineSpecs.Shader->GetVertexLayout(), DefaultQuadVertices * sizeof(QuadVertex), true, nullptr);

			uint32_t* quadIndices = new uint32_t[DefaultQuadIndices];
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
			delete[] quadIndices;

			m_QuadVertexData.Allocate(DefaultQuadVertices * sizeof QuadVertex);
		}

		// Circle
		{
			PipelineSpecification circlePipelineSpecs;
			circlePipelineSpecs.TargetFrameBuffer = framebuffer;
			circlePipelineSpecs.Shader = Renderer::GetShaderLib()->Get("Renderer2D_Circle");
			circlePipelineSpecs.DebugName = "Renderer2D-Circle";
			circlePipelineSpecs.DepthEnabled = specifications.UseDepthTesting;
			circlePipelineSpecs.WriteDepth = false;
			circlePipelineSpecs.DepthOperator = DepthCompareOperator::Equal;
			m_CirclePipeline = Pipeline::Create(circlePipelineSpecs);
			m_CircleMaterial = Material::Create(circlePipelineSpecs.Shader);

			m_CircleVertexBuffer = VertexBuffer::Create(circlePipelineSpecs.Shader->GetVertexLayout(), DefaultCircleVertices * sizeof CircleVertex, true, nullptr);
			m_CircleVertexData.Allocate(DefaultCircleVertices * sizeof CircleVertex);
			m_CircleIndexBuffer = m_QuadIndexBuffer;
		}


		// Line
		{
			PipelineSpecification linePipelineSpecs;
			linePipelineSpecs.TargetFrameBuffer = framebuffer;
			linePipelineSpecs.Shader = Renderer::GetShaderLib()->Get("Renderer2D_Line");
			linePipelineSpecs.DebugName = "Renderer2D-Line";
			linePipelineSpecs.Primitve = PrimitveType::Line;
			linePipelineSpecs.DepthEnabled = specifications.UseDepthTesting;
			linePipelineSpecs.WriteDepth = false;
			linePipelineSpecs.DepthOperator = DepthCompareOperator::Equal;
			m_LinePipeline = Pipeline::Create(linePipelineSpecs);
			m_LineMaterial = Material::Create(linePipelineSpecs.Shader);
			
			m_LineVertexBuffer = VertexBuffer::Create(linePipelineSpecs.Shader->GetVertexLayout(), DefaultLineVertices * sizeof LineVertex, true, nullptr);
			m_LineVertexData.Allocate(DefaultLineVertices * sizeof LineVertex);
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
	}

	void Renderer2D::Resize(uint32_t width, uint32_t height)
	{
		m_DepthFrameBuffer->Resize(width, height);
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

		// Circle
		m_CircleIndexCount = 0;
		m_CircleVertexCount = 0;

		// Line
		m_LineVertexCount = 0;


		m_Statistics.DrawCalls = 0;
		m_Statistics.QuadCount = 0;
		m_Statistics.CircleCount = 0;
		m_Statistics.LineCount = 0;
		m_Statistics.LineOnTopCount = 0;
		m_Statistics.VertexCount = 0;
		m_Statistics.IndexCount = 0;
		m_Statistics.TextureCount = 1;
		m_Statistics.GeometryPassTime = 0.0f;
	}

	void Renderer2D::EndScene()
	{
		SK_PROFILE_FUNCTION();

		SK_CORE_VERIFY(m_Active);

		m_CommandBuffer->Begin();
		m_CommandBuffer->BeginTimeQuery(m_GeometryPassTimer);

		m_DepthFrameBuffer->Clear(m_CommandBuffer);

		// Quad Depth Pass
		if (m_QuadIndexCount)
		{
			if (m_QuadIndexBuffer->GetCount() < m_QuadIndexCount)
				ResizeQuadIndexBuffer(m_QuadIndexCount);

			m_QuadVertexBuffer->SetData(m_QuadVertexData, true);
			Renderer::RenderGeometry(m_CommandBuffer, m_QuadDepthPassPipeline, m_QuadDepthPassMaterial, m_ConstantBufferSet, m_QuadVertexBuffer, m_QuadIndexBuffer, m_QuadIndexCount);
		}

		// Circle Depth Pass
		if (m_CircleIndexCount)
		{
			if (m_CircleIndexBuffer->GetCount() < m_CircleIndexCount)
				ResizeQuadIndexBuffer(m_CircleIndexCount);

			m_CircleVertexBuffer->SetData(m_CircleVertexData, true);
			Renderer::RenderGeometry(m_CommandBuffer, m_CircleDepthPassPipeline, m_CircleDepthPassMaterial, m_ConstantBufferSet, m_CircleVertexBuffer, m_CircleIndexBuffer, m_CircleIndexCount);
		}

		// Line Depth Pass
		if (m_LineVertexCount)
		{
			m_LineVertexBuffer->SetData(m_LineVertexData, true);
			Renderer::RenderGeometry(m_CommandBuffer, m_LineDepthPassPipeline, m_LineDepthPassMaterial, m_ConstantBufferSet, m_LineVertexBuffer, m_LineVertexCount);
		}

		// Quad
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


		// Circle
		if (m_CircleIndexCount)
		{
			Renderer::RenderGeometry(m_CommandBuffer, m_CirclePipeline, m_CircleMaterial, m_ConstantBufferSet, m_CircleVertexBuffer, m_CircleIndexBuffer, m_CircleIndexCount);
			m_Statistics.DrawCalls++;
		}


		// Line
		if (m_LineVertexCount)
		{
			Renderer::RenderGeometry(m_CommandBuffer, m_LinePipeline, m_LineMaterial, m_ConstantBufferSet, m_LineVertexBuffer, m_LineVertexCount);
			m_Statistics.DrawCalls++;
		}


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

	void Renderer2D::AssureQuadVertexDataSize()
	{
		if (m_QuadBatch->VertexOffset + m_QuadBatch->VertexCount >= m_QuadVertexData.Count<QuadVertex>())
			m_QuadVertexData.Resize(m_QuadVertexData.Size * 2);
	}

	void Renderer2D::AssureCircleVertexDataSize()
	{
		if (m_CircleVertexCount >= m_CircleVertexData.Count<CircleVertex>())
			m_CircleVertexData.Resize(m_CircleVertexData.Size * 2);
	}

	void Renderer2D::AssureLineVertexDataSize()
	{
		if (m_LineVertexCount >= m_LineVertexData.Count<LineVertex>())
			m_LineVertexData.Resize(m_LineVertexData.Size * 2);
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
			uint32_t* quadIndices = new uint32_t[indexCount];
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
			delete[] quadIndices;
		});
	}

}
