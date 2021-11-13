#include "skpch.h"
#include "Renderer2D.h"

#include "Shark/Render/Renderer.h"

#include "Shark/Utility/Math.h"
#include "Shark/Debug/Profiler.h"

#include "Platform/DirectX11/DirectXRenderCommandBuffer.h"
#include "Platform/DirectX11/DirectXTexture.h"

namespace Shark {

	Renderer2D::Renderer2D(Ref<FrameBuffer> renderTarget)
	{
		Init(renderTarget);
	}

	Renderer2D::~Renderer2D()
	{
		if (m_QuadVertexBasePtr)
			delete[] m_QuadVertexBasePtr;
		if (m_CircleVertexBasePtr)
			delete[] m_CircleVertexBasePtr;
		if (m_LineVertexBasePtr)
			delete[] m_LineVertexBasePtr;
	}

	void Renderer2D::Init(Ref<FrameBuffer> renderTarget)
	{
		m_RenderTarget = renderTarget;
		m_WhiteTexture = Renderer::GetWhiteTexture();

		m_ConstantBufferSet = ConstantBufferSet::Create();
		m_ConstantBufferSet->Create(sizeof(CBCamera), 0);

		m_CommandBuffer = RenderCommandBuffer::Create();

		m_Statistics.GeometryPassTimer = GPUTimer::Create();

		// Quad
		{
			PipelineSpecification quadPipelineSpecs;
			quadPipelineSpecs.TargetFrameBuffer = renderTarget;
			quadPipelineSpecs.Shader = Renderer::GetShaderLib()->Get("Renderer2D_Quad");
			quadPipelineSpecs.DebugName = "Renderer2D-Quad";
			m_QuadPipeline = Pipeline::Create(quadPipelineSpecs);

			m_QuadVertexBuffer = VertexBuffer::Create(quadPipelineSpecs.Shader->GetVertexLayout(), nullptr, MaxQuadVertices * sizeof(QuadVertex), true);

			Index* quadIndices = new Index[MaxQuadIndices];
			for (uint32_t i = 0, j = 0; i < MaxQuadIndices; i += 6, j += 4)
			{
				quadIndices[i + 0] = j + 0;
				quadIndices[i + 1] = j + 1;
				quadIndices[i + 2] = j + 2;

				quadIndices[i + 3] = j + 2;
				quadIndices[i + 4] = j + 3;
				quadIndices[i + 5] = j + 0;
			}
			m_QuadIndexBuffer = IndexBuffer::Create(quadIndices, MaxQuadIndices);
			delete[] quadIndices;

			m_QuadVertexBasePtr = new QuadVertex[MaxQuadVertices];
			m_QuadTextureArray = Texture2DArray::Create(MaxTextureSlots);
		}

		// Circle
		{
			PipelineSpecification cirlcePipelineSpecs;
			cirlcePipelineSpecs.TargetFrameBuffer = renderTarget;
			cirlcePipelineSpecs.Shader = Renderer::GetShaderLib()->Get("Renderer2D_Circle");
			cirlcePipelineSpecs.DebugName = "Renderer2D-Circle";
			m_CirlcePipeline = Pipeline::Create(cirlcePipelineSpecs);

			m_CircleVertexBuffer = VertexBuffer::Create(cirlcePipelineSpecs.Shader->GetVertexLayout(), nullptr, MaxCircleVertices * sizeof(CircleVertex), true);
			m_CircleVertexBasePtr = new CircleVertex[MaxCircleVertices];
		}


		// Line
		{
			PipelineSpecification linePipelineSpecs;
			linePipelineSpecs.TargetFrameBuffer = renderTarget;
			linePipelineSpecs.Shader = Renderer::GetShaderLib()->Get("Renderer2D_Line");
			linePipelineSpecs.DebugName = "Renderer2D-Line";
			m_LinePipeline = Pipeline::Create(linePipelineSpecs);
			
			m_LineVertexBuffer = VertexBuffer::Create(linePipelineSpecs.Shader->GetVertexLayout(), nullptr, MaxLineVertices * sizeof(LineVertex), true);
			m_LineVertexBasePtr = new LineVertex[MaxLineVertices];
		}

		// Line On Top
		{
			PipelineSpecification lineOnTopPipelineSpecs;
			lineOnTopPipelineSpecs.TargetFrameBuffer = renderTarget;
			lineOnTopPipelineSpecs.Shader = Renderer::GetShaderLib()->Get("Renderer2D_Line");
			lineOnTopPipelineSpecs.DebugName = "Renderer2D-LineOnTop";
			lineOnTopPipelineSpecs.DepthEnabled = false;
			lineOnTopPipelineSpecs.WriteDepth = false;
			m_LineOnTopPipeline = Pipeline::Create(lineOnTopPipelineSpecs);

			m_LineOnTopVertexBuffer = VertexBuffer::Create(lineOnTopPipelineSpecs.Shader->GetVertexLayout(), nullptr, MaxLineOnTopVertices * sizeof(LineVertex), true);
			m_LineOnTopVertexBasePtr = new LineVertex[MaxLineOnTopVertices];
		}

	}

	void Renderer2D::ShutDown()
	{

	}

	void Renderer2D::SetRenderTarget(Ref<FrameBuffer> renderTarget)
	{
		SK_CORE_ASSERT(!m_Active);
		m_RenderTarget = renderTarget;
	}

	void Renderer2D::BeginScene(const DirectX::XMMATRIX& viewProj)
	{
		m_Active = true;

		CBCamera cam{ viewProj };
		m_ConstantBufferSet->Get(0)->Set(&cam, sizeof(CBCamera));

		// Quad
		m_QuadIndexCount = 0;
		m_QuadVertexIndexPtr = m_QuadVertexBasePtr;

		m_QuadTextureSlotIndex = 1;
		m_QuadTextureArray->Set(0, m_WhiteTexture);
		for (uint32_t i = 1; i < MaxTextureSlots; i++)
			m_QuadTextureArray->Set(i, nullptr);

		// Circle
		m_CircleIndexCount = 0;
		m_CircleVertexIndexPtr = m_CircleVertexBasePtr;

		// Line
		m_LineVertexCount = 0;
		m_LineVertexIndexPtr = m_LineVertexBasePtr;

		// Line On Top
		m_LineOnTopVertexCount = 0;
		m_LineOnTopVertexIndexPtr = m_LineOnTopVertexBasePtr;

		m_Statistics.DrawCalls = 0;
		m_Statistics.QuadCount = 0;
		m_Statistics.CircleCount = 0;
		m_Statistics.LineCount = 0;
		m_Statistics.LineOnTopCount = 0;
		m_Statistics.VertexCount = 0;
		m_Statistics.IndexCount = 0;
		m_Statistics.TextureCount = 1;
	}

	void Renderer2D::EndScene()
	{
		SK_PERF_SCOPED("Renderer2D::EndScene");

		m_CommandBuffer->Begin();

#if SK_DEBUG
		{
			SK_CORE_ASSERT(RendererAPI::GetAPI() == RendererAPI::API::DirectX11);
			Ref<DirectXTexture2D> dxTexture = m_WhiteTexture.As<DirectXTexture2D>();
			Ref<DirectXRenderCommandBuffer> dxCommandBuffer = m_CommandBuffer.As<DirectXRenderCommandBuffer>();

			for (uint32_t i = 0; i < MaxTextureSlots; i++)
				dxTexture->Bind(dxCommandBuffer->GetContext(), i);
		}
#endif

		m_CommandBuffer->BeginTimeQuery(m_Statistics.GeometryPassTimer);

		// Quad
		{
			uint32_t dataSize = (uint32_t)((uint8_t*)m_QuadVertexIndexPtr - (uint8_t*)m_QuadVertexBasePtr);
			if (dataSize)
			{
				m_QuadVertexBuffer->SetData(m_QuadVertexBasePtr, dataSize);

				Renderer::RenderGeometry(m_CommandBuffer, m_QuadPipeline, m_ConstantBufferSet, m_QuadTextureArray, m_QuadVertexBuffer, m_QuadIndexBuffer, m_QuadIndexCount, PrimitveTopology::Triangle);
				m_Statistics.DrawCalls++;
			}
		}


		// Circle
		{
			uint32_t dataSize = (uint32_t)((uint8_t*)m_CircleVertexIndexPtr - (uint8_t*)m_CircleVertexBasePtr);
			if (dataSize)
			{
				m_CircleVertexBuffer->SetData(m_CircleVertexBasePtr, dataSize);

				Renderer::RenderGeometry(m_CommandBuffer, m_CirlcePipeline, m_ConstantBufferSet, nullptr, m_CircleVertexBuffer, m_QuadIndexBuffer, m_CircleIndexCount, PrimitveTopology::Triangle);
				m_Statistics.DrawCalls++;
			}
		}


		// Line
		{
			uint32_t dataSize = (uint32_t)((uint8_t*)m_LineVertexIndexPtr - (uint8_t*)m_LineVertexBasePtr);
			if (dataSize)
			{
				m_LineVertexBuffer->SetData(m_LineVertexBasePtr, dataSize);

				Renderer::RenderGeometry(m_CommandBuffer, m_LinePipeline, m_ConstantBufferSet, nullptr, m_LineVertexBuffer, m_LineVertexCount, PrimitveTopology::Line);
				m_Statistics.DrawCalls++;
			}
		}


		// Line On Top
		{
			uint32_t dataSize = (uint32_t)((uint8_t*)m_LineOnTopVertexIndexPtr - (uint8_t*)m_LineOnTopVertexBasePtr);
			if (dataSize)
			{
				m_LineOnTopVertexBuffer->SetData(m_LineOnTopVertexBasePtr, dataSize);

				Renderer::RenderGeometry(m_CommandBuffer, m_LineOnTopPipeline, m_ConstantBufferSet, nullptr, m_LineOnTopVertexBuffer, m_LineOnTopVertexCount, PrimitveTopology::Line);
				m_Statistics.DrawCalls++;
			}
		}

		m_CommandBuffer->EndTimeQuery(m_Statistics.GeometryPassTimer);


		m_CommandBuffer->End();
		m_CommandBuffer->Execute();

		m_Active = false;
	}

	void Renderer2D::DrawQuad(const DirectX::XMFLOAT2& position, const DirectX::XMFLOAT2& scaling, const DirectX::XMFLOAT4& color, int id)
	{
		DrawQuad({ position.x, position.y, 0.0f }, { scaling.x, scaling.y, 1.0f }, color, id);
	}

	void Renderer2D::DrawQuad(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& scaling, const DirectX::XMFLOAT4& color, int id)
	{
		DrawQuad(position, scaling, m_WhiteTexture, 1.0f, color, id);
	}

	void Renderer2D::DrawQuad(const DirectX::XMFLOAT2& position, const DirectX::XMFLOAT2& scaling, const Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tintcolor, int id)
	{
		DrawQuad({ position.x, position.y, 0.0f }, { scaling.x, scaling.y, 1.0f }, texture, tilingfactor, tintcolor, id);
	}

	void Renderer2D::DrawQuad(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& scaling, const Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tintcolor, int id)
	{
		static constexpr DirectX::XMFLOAT3 VertexPositions[4] = { { -0.5f, 0.5f, 0.0f }, { 0.5f, 0.5f, 0.0f }, { 0.5f, -0.5f, 0.0f }, { -0.5f, -0.5f, 0.0f } };
		static constexpr DirectX::XMFLOAT2 TextureCoords[4] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

		if (m_QuadIndexCount >= MaxQuadIndices)
			FlushAndResetQuad();

		const auto translation = DirectX::XMMatrixScaling(scaling.x, scaling.y, scaling.x) * DirectX::XMMatrixTranslation(position.x, position.y, position.z);

		uint32_t textureSlot = 0;
		if (texture != m_WhiteTexture && texture != nullptr)
		{
			for (uint32_t i = 0; i < m_QuadTextureSlotIndex; i++)
				if (m_QuadTextureArray->Get(i) == texture)
					textureSlot = i;
		
			if (textureSlot == 0)
			{
				textureSlot = m_QuadTextureSlotIndex++;
				m_QuadTextureArray->Set(textureSlot, texture);
				m_Statistics.TextureCount++;
			}
		}

		for (uint32_t i = 0; i < 4; i++)
		{
			QuadVertex* vtx = m_QuadVertexIndexPtr++;
			vtx->WorldPosition = DXMath::Store3(DirectX::XMVector3Transform(DXMath::Load(VertexPositions[i]), translation));
			vtx->Color = tintcolor;
			vtx->Tex = TextureCoords[i];
			vtx->TextureSlot = textureSlot;
			vtx->TilingFactor = tilingfactor;
			vtx->ID = id;
		}

		m_QuadIndexCount += 6;

		m_Statistics.QuadCount++;
		m_Statistics.VertexCount += 4;
		m_Statistics.IndexCount += 6;

	}

	void Renderer2D::DrawRotatedQuad(const DirectX::XMFLOAT2& position, float rotation, const DirectX::XMFLOAT2& scaling, const DirectX::XMFLOAT4& color, int id)
	{
		DrawRotatedQuad({ position.x, position.y, 0.0f }, { 0.0f, 0.0f, rotation }, { scaling.x, scaling.y, 1.0f }, color, id);
	}

	void Renderer2D::DrawRotatedQuad(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& rotation, const DirectX::XMFLOAT3& scaling, const DirectX::XMFLOAT4& color, int id)
	{
		DrawRotatedQuad(position, rotation, scaling, m_WhiteTexture, 1.0f, color, id);
	}

	void Renderer2D::DrawRotatedQuad(const DirectX::XMFLOAT2& position, float rotation, const DirectX::XMFLOAT2& scaling, const Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tintcolor, int id)
	{
		DrawRotatedQuad({ position.x, position.y, 0.0f }, { 0.0f, 0.0f, rotation }, { scaling.x, scaling.y, 1.0f }, texture, tilingfactor, tintcolor, id);
	}

	void Renderer2D::DrawRotatedQuad(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& rotation, const DirectX::XMFLOAT3& scaling, const Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tintcolor, int id)
	{
		if (m_QuadIndexCount >= MaxQuadIndices)
			FlushAndResetQuad();

		const auto translation = DirectX::XMMatrixScaling(scaling.x, scaling.y, 1.0f) * DirectX::XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z) * DirectX::XMMatrixTranslation(position.x, position.y, position.z);

		uint32_t textureSlot = 0;
		if (texture != m_WhiteTexture && texture != nullptr)
		{
			for (uint32_t i = 0; i < m_QuadTextureSlotIndex; i++)
				if (m_QuadTextureArray->Get(i) == texture)
					textureSlot = i;

			if (textureSlot == 0)
			{
				textureSlot = m_QuadTextureSlotIndex++;
				m_QuadTextureArray->Set(textureSlot, texture);
				m_Statistics.TextureCount++;
			}
		}

		for (uint32_t i = 0; i < 4; i++)
		{
			QuadVertex* vtx = m_QuadVertexIndexPtr++;
			vtx->WorldPosition = DXMath::Store3(DirectX::XMVector3Transform(DXMath::Load(QuadVertexPositions[i]), translation));
			vtx->Color = tintcolor;
			vtx->Tex = TextureCoords[i];
			vtx->TextureSlot = textureSlot;
			vtx->TilingFactor = tilingfactor;
			vtx->ID = id;
		}

		m_QuadIndexCount += 6;

		m_Statistics.QuadCount++;
		m_Statistics.VertexCount += 4;
		m_Statistics.IndexCount += 6;
	}

	void Renderer2D::DrawFilledCircle(const DirectX::XMFLOAT2& position, const DirectX::XMFLOAT2& scaling, const DirectX::XMFLOAT4& color, float thickness, float fade, int id)
	{
		DrawFilledCircle({ position.x, position.y, 0.0f }, { scaling.x, scaling.y, 1.0f }, color, thickness, fade, id);
	}

	void Renderer2D::DrawFilledCircle(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& scaling, const DirectX::XMFLOAT4& color, float thickness, float fade, int id)
	{
		if (m_CircleIndexCount >= MaxCircleIndices)
			FlushAndResetCircle();

		const auto translation = DirectX::XMMatrixScaling(scaling.x, scaling.y, scaling.x) * DirectX::XMMatrixTranslation(position.x, position.y, position.z);

		for (uint32_t i = 0; i < 4; i++)
		{
			CircleVertex* vtx = m_CircleVertexIndexPtr++;
			vtx->WorldPosition = DXMath::Store3(DirectX::XMVector3Transform(DXMath::Load(QuadVertexPositions[i]), translation));
			vtx->LocalPosition = { QuadVertexPositions[i].x * 2.0f, QuadVertexPositions[i].y * 2.0f };
			vtx->Color = color;
			vtx->Thickness = thickness;
			vtx->Fade = fade;
			vtx->ID = id;
		}

		m_CircleIndexCount += 6;

		m_Statistics.CircleCount++;
		m_Statistics.VertexCount += 4;
		m_Statistics.IndexCount += 6;
	}

	void Renderer2D::DrawFilledCircle(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& rotation, const DirectX::XMFLOAT3& scaling, const DirectX::XMFLOAT4& color, float thickness, float fade, int id)
	{
		if (m_CircleIndexCount >= MaxCircleIndices)
			FlushAndResetCircle();

		const auto translation = DirectX::XMMatrixScaling(scaling.x, scaling.y, scaling.z) * DirectX::XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z) * DirectX::XMMatrixTranslation(position.x, position.y, position.z);

		for (uint32_t i = 0; i < 4; i++)
		{
			CircleVertex* vtx = m_CircleVertexIndexPtr++;
			vtx->WorldPosition = DXMath::Store3(DirectX::XMVector3Transform(DXMath::Load(QuadVertexPositions[i]), translation));
			vtx->LocalPosition = { QuadVertexPositions[i].x * 2.0f, QuadVertexPositions[i].y * 2.0f };
			vtx->Color = color;
			vtx->Thickness = thickness;
			vtx->Fade = fade;
			vtx->ID = id;
		}

		m_CircleIndexCount += 6;

		m_Statistics.CircleCount++;
		m_Statistics.VertexCount += 4;
		m_Statistics.IndexCount += 6;
	}

	void Renderer2D::DrawCircle(const DirectX::XMFLOAT2& position, float radius, const DirectX::XMFLOAT4& color, float delta, int id)
	{
		DrawCircle({ position.x, position.y, 0.0f }, { 0.0f, 0.0f, 0.0f }, radius, color, delta, id);
	}

	void Renderer2D::DrawCircle(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& rotation, float radius, const DirectX::XMFLOAT4& color, float delta, int id)
	{
		const auto translation = DirectX::XMMatrixScaling(radius, radius, radius) * DirectX::XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z) * DirectX::XMMatrixTranslation(position.x, position.y, position.z);

		if (delta < 0.01f)
			delta = 0.01f;

		for (float i = 0; i < DirectX::XM_2PI; i += delta)
		{
			float r0 = i;
			float r1 = i + delta;

			DirectX::XMFLOAT3 p0, p1;
			p0.x = sinf(r0);
			p0.y = cosf(r0);
			p0.z = 0.0f;

			p1.x = sinf(r1);
			p1.y = cosf(r1);
			p1.z = 0.0f;

			p0 = DXMath::Store3(DirectX::XMVector3Transform(DXMath::Load(p0), translation));
			p1 = DXMath::Store3(DirectX::XMVector3Transform(DXMath::Load(p1), translation));

			DrawLine(p0, p1, color, id);
		}

	}

	void Renderer2D::DrawLine(const DirectX::XMFLOAT2& pos0, const DirectX::XMFLOAT2& pos1, const DirectX::XMFLOAT4& color, int id)
	{
		DrawLine({ pos0.x, pos0.y, 0.0f }, { pos1.x, pos1.y, 0.0f }, color, id);
	}

	void Renderer2D::DrawLine(const DirectX::XMFLOAT3& pos0, const DirectX::XMFLOAT3& pos1, const DirectX::XMFLOAT4& color, int id)
	{
		if (m_LineVertexCount >= MaxLineVertices)
			FlushAndResetLine();

		LineVertex* vtx = m_LineVertexIndexPtr++;
		vtx->WorldPosition = pos0;
		vtx->Color = color;
		vtx->ID = id;

		vtx = m_LineVertexIndexPtr++;
		vtx->WorldPosition = pos1;
		vtx->Color = color;
		vtx->ID = id;

		m_LineVertexCount += 2;
		
		m_Statistics.LineCount++;
		m_Statistics.VertexCount += 2;
	}


	void Renderer2D::DrawRect(const DirectX::XMFLOAT2& position, const DirectX::XMFLOAT2& scaling, const DirectX::XMFLOAT4& color, int id)
	{
		DrawRect({ position.x, position.y, 0.0f }, { scaling.x, scaling.y, 1.0f }, color, id);
	}

	void Renderer2D::DrawRect(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& scaling, const DirectX::XMFLOAT4& color, int id)
	{
		const auto translation = DirectX::XMMatrixScaling(scaling.x, scaling.y, scaling.x) * DirectX::XMMatrixTranslation(position.x, position.y, position.z);

		auto p0 = DXMath::Store3(DirectX::XMVector3Transform(DXMath::Load(QuadVertexPositions[0]), translation));
		auto p1 = DXMath::Store3(DirectX::XMVector3Transform(DXMath::Load(QuadVertexPositions[1]), translation));
		auto p2 = DXMath::Store3(DirectX::XMVector3Transform(DXMath::Load(QuadVertexPositions[2]), translation));
		auto p3 = DXMath::Store3(DirectX::XMVector3Transform(DXMath::Load(QuadVertexPositions[3]), translation));

		DrawLine(p0, p1, color, id);
		DrawLine(p1, p2, color, id);
		DrawLine(p2, p3, color, id);
		DrawLine(p3, p0, color, id);
	}

	void Renderer2D::DrawRect(const DirectX::XMFLOAT2& position, float rotation, const DirectX::XMFLOAT2& scaling, const DirectX::XMFLOAT4& color, int id)
	{
		DrawRect({ position.x, position.y, 0.0f }, { 0.0f, 0.0f, rotation }, { scaling.x, scaling.y, 1.0f }, color, id);
	}

	void Renderer2D::DrawRect(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& rotation, const DirectX::XMFLOAT3& scaling, const DirectX::XMFLOAT4& color, int id)
	{
		const auto translation = DirectX::XMMatrixScaling(scaling.x, scaling.y, scaling.z) * DirectX::XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z) * DirectX::XMMatrixTranslation(position.x, position.y, position.z);

		auto p0 = DXMath::Store3(DirectX::XMVector3Transform(DXMath::Load(QuadVertexPositions[0]), translation));
		auto p1 = DXMath::Store3(DirectX::XMVector3Transform(DXMath::Load(QuadVertexPositions[1]), translation));
		auto p2 = DXMath::Store3(DirectX::XMVector3Transform(DXMath::Load(QuadVertexPositions[2]), translation));
		auto p3 = DXMath::Store3(DirectX::XMVector3Transform(DXMath::Load(QuadVertexPositions[3]), translation));

		DrawLine(p0, p1, color, id);
		DrawLine(p1, p2, color, id);
		DrawLine(p2, p3, color, id);
		DrawLine(p3, p0, color, id);
	}

	void Renderer2D::DrawLineOnTop(const DirectX::XMFLOAT2& pos0, const DirectX::XMFLOAT2& pos1, const DirectX::XMFLOAT4& color, int id)
	{
		DrawLineOnTop({ pos0.x, pos0.y, 0.0f }, { pos1.x, pos1.y, 0.0f }, color, id);
	}

	void Renderer2D::DrawLineOnTop(const DirectX::XMFLOAT3& pos0, const DirectX::XMFLOAT3& pos1, const DirectX::XMFLOAT4& color, int id)
	{
		if (m_LineOnTopVertexCount >= MaxLineOnTopVertices)
			FlushAndResetLineOnTop();

		LineVertex* vtx = m_LineOnTopVertexIndexPtr++;
		vtx->WorldPosition = pos0;
		vtx->Color = color;
		vtx->ID = id;

		vtx = m_LineOnTopVertexIndexPtr++;
		vtx->WorldPosition = pos1;
		vtx->Color = color;
		vtx->ID = id;

		m_LineOnTopVertexCount += 2;

		m_Statistics.LineOnTopCount++;
		m_Statistics.VertexCount += 2;
		m_Statistics.IndexCount += 2;
	}

	void Renderer2D::DrawRectOnTop(const DirectX::XMFLOAT2& position, const DirectX::XMFLOAT2& scaling, const DirectX::XMFLOAT4& color, int id)
	{
		DrawRectOnTop({ position.x, position.y, 0.0f }, { scaling.x, scaling.y, 1.0f }, color, id);
	}

	void Renderer2D::DrawRectOnTop(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& scaling, const DirectX::XMFLOAT4& color, int id)
	{
		const auto translation = DirectX::XMMatrixScaling(scaling.x, scaling.y, scaling.x) * DirectX::XMMatrixTranslation(position.x, position.y, position.z);

		auto p0 = DXMath::Store3(DirectX::XMVector3Transform(DXMath::Load(QuadVertexPositions[0]), translation));
		auto p1 = DXMath::Store3(DirectX::XMVector3Transform(DXMath::Load(QuadVertexPositions[1]), translation));
		auto p2 = DXMath::Store3(DirectX::XMVector3Transform(DXMath::Load(QuadVertexPositions[2]), translation));
		auto p3 = DXMath::Store3(DirectX::XMVector3Transform(DXMath::Load(QuadVertexPositions[3]), translation));

		DrawLineOnTop(p0, p1, color, id);
		DrawLineOnTop(p1, p2, color, id);
		DrawLineOnTop(p2, p3, color, id);
		DrawLineOnTop(p3, p0, color, id);
	}

	void Renderer2D::DrawRectOnTop(const DirectX::XMFLOAT2& position, float rotation, const DirectX::XMFLOAT2& scaling, const DirectX::XMFLOAT4& color, int id)
	{
		DrawRectOnTop({ position.x, position.y, 0.0f }, { 0.0f, 0.0f, rotation }, { scaling.x, scaling.y, 1.0f }, color, id);
	}

	void Renderer2D::DrawRectOnTop(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& rotation, const DirectX::XMFLOAT3& scaling, const DirectX::XMFLOAT4& color, int id)
	{
		const auto translation = DirectX::XMMatrixScaling(scaling.x, scaling.y, scaling.z) * DirectX::XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z) * DirectX::XMMatrixTranslation(position.x, position.y, position.z);

		auto p0 = DXMath::Store3(DirectX::XMVector3Transform(DXMath::Load(QuadVertexPositions[0]), translation));
		auto p1 = DXMath::Store3(DirectX::XMVector3Transform(DXMath::Load(QuadVertexPositions[1]), translation));
		auto p2 = DXMath::Store3(DirectX::XMVector3Transform(DXMath::Load(QuadVertexPositions[2]), translation));
		auto p3 = DXMath::Store3(DirectX::XMVector3Transform(DXMath::Load(QuadVertexPositions[3]), translation));

		DrawLineOnTop(p0, p1, color, id);
		DrawLineOnTop(p1, p2, color, id);
		DrawLineOnTop(p2, p3, color, id);
		DrawLineOnTop(p3, p0, color, id);
	}

	void Renderer2D::DrawCircleOnTop(const DirectX::XMFLOAT2& position, float radius, const DirectX::XMFLOAT4& color, float delta, int id)
	{
		DrawCircleOnTop({ position.x, position.y, 0.0f }, { 0.0f, 0.0f, 0.0f }, radius, color, delta, id);
	}

	void Renderer2D::DrawCircleOnTop(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& rotation, float radius, const DirectX::XMFLOAT4& color, float delta, int id)
	{
		const auto translation = DirectX::XMMatrixScaling(radius, radius, radius) * DirectX::XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z) * DirectX::XMMatrixTranslation(position.x, position.y, position.z);

		if (delta < 0.01f)
			delta = 0.01f;

		for (float i = 0; i < DirectX::XM_2PI; i += delta)
		{
			float r0 = i;
			float r1 = i + delta;

			DirectX::XMFLOAT3 p0, p1;
			p0.x = sinf(r0);
			p0.y = cosf(r0);
			p0.z = 0.0f;

			p1.x = sinf(r1);
			p1.y = cosf(r1);
			p1.z = 0.0f;

			p0 = DXMath::Store3(DirectX::XMVector3Transform(DXMath::Load(p0), translation));
			p1 = DXMath::Store3(DirectX::XMVector3Transform(DXMath::Load(p1), translation));

			DrawLineOnTop(p0, p1, color, id);
		}
	}

	void Renderer2D::FlushAndResetQuad()
	{
		m_CommandBuffer->Begin();

		uint32_t dataSize = (uint32_t)((uint8_t*)m_QuadVertexIndexPtr - (uint8_t*)m_QuadVertexBasePtr);
		if (dataSize)
		{
			m_QuadVertexBuffer->SetData(m_QuadVertexBasePtr, dataSize);

			Renderer::RenderGeometry(m_CommandBuffer, m_QuadPipeline, m_ConstantBufferSet, m_QuadTextureArray, m_QuadVertexBuffer, m_QuadIndexBuffer, m_QuadIndexCount, PrimitveTopology::Triangle);
			m_Statistics.DrawCalls++;
		}

		m_CommandBuffer->End();

		m_QuadIndexCount = 0;
		m_QuadVertexIndexPtr = m_QuadVertexBasePtr;
		m_QuadTextureSlotIndex = 1;
		m_QuadTextureArray->Set(0, m_WhiteTexture);
		for (uint32_t i = 1; i < MaxTextureSlots; i++)
			m_QuadTextureArray->Set(i, nullptr);

	}

	void Renderer2D::FlushAndResetCircle()
	{
		m_CommandBuffer->Begin();

		uint32_t dataSize = (uint32_t)((uint8_t*)m_CircleVertexIndexPtr - (uint8_t*)m_CircleVertexBasePtr);
		if (dataSize)
		{
			m_CircleVertexBuffer->SetData(m_CircleVertexBasePtr, dataSize);

			Renderer::RenderGeometry(m_CommandBuffer, m_CirlcePipeline, m_ConstantBufferSet, nullptr, m_CircleVertexBuffer, m_QuadIndexBuffer, m_CircleIndexCount, PrimitveTopology::Triangle);
			m_Statistics.DrawCalls++;
		}

		m_CommandBuffer->End();

		m_CircleIndexCount = 0;
		m_CircleVertexIndexPtr = m_CircleVertexBasePtr;
	}

	void Renderer2D::FlushAndResetLine()
	{
		m_CommandBuffer->Begin();

		uint32_t dataSize = (uint32_t)((uint8_t*)m_LineVertexIndexPtr - (uint8_t*)m_LineVertexBasePtr);
		if (dataSize)
		{
			m_LineVertexBuffer->SetData(m_LineVertexBasePtr, dataSize);

			Renderer::RenderGeometry(m_CommandBuffer, m_LinePipeline, m_ConstantBufferSet, nullptr, m_LineVertexBuffer, m_LineVertexCount, PrimitveTopology::Line);
			m_Statistics.DrawCalls++;
		}

		m_CommandBuffer->End();

		m_LineVertexCount = 0;
		m_LineVertexIndexPtr = m_LineVertexBasePtr;
	}

	void Renderer2D::FlushAndResetLineOnTop()
	{
		m_CommandBuffer->Begin();

		uint32_t dataSize = (uint32_t)((uint8_t*)m_LineOnTopVertexIndexPtr - (uint8_t*)m_LineOnTopVertexBasePtr);
		if (dataSize)
		{
			m_LineOnTopVertexBuffer->SetData(m_LineOnTopVertexBasePtr, dataSize);

			Renderer::RenderGeometry(m_CommandBuffer, m_LineOnTopPipeline, m_ConstantBufferSet, nullptr, m_LineOnTopVertexBuffer, m_LineOnTopVertexCount, PrimitveTopology::Line);
			m_Statistics.DrawCalls++;
		}

		m_CommandBuffer->End();

		m_LineOnTopVertexCount = 0;
		m_LineOnTopVertexIndexPtr = m_LineOnTopVertexBasePtr;
	}

}
