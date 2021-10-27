#include "skpch.h"
#include "Renderer2D.h"

#include "Shark/Render/Renderer.h"

#include "Shark/Utility/Math.h"

#include "Platform/DirectX11/DirectXRenderCommandBuffer.h"

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
		m_RenderCommandBuffer = RenderCommandBuffer::Create();

		m_ConstantBufferSet = ConstantBufferSet::Create();
		m_ConstantBufferSet->Create(sizeof(CBCamera), 0);

		m_WhiteTexture = Renderer::GetWhiteTexture();

		// Quad
		m_QuadShader = Renderer::GetShaderLib().Get("Renderer2D_Quad");
		m_QuadVertexBuffer = VertexBuffer::Create(m_QuadShader->GetVertexLayout(), nullptr, MaxQuadVertices * sizeof(QuadVertex), true);
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

		// Circle
		m_CircleShader = Renderer::GetShaderLib().Get("Renderer2D_Circle");
		m_CircleVertexBuffer = VertexBuffer::Create(m_CircleShader->GetVertexLayout(), nullptr, MaxCircleVertices * sizeof(CircleVertex), true);
		m_CircleVertexBasePtr = new CircleVertex[MaxCircleVertices];

		// Line
		m_LineShader = Renderer::GetShaderLib().Get("Renderer2D_Line");
		m_LineVertexBuffer = VertexBuffer::Create(m_LineShader->GetVertexLayout(), nullptr, MaxLineVertices * sizeof(LineVertex), true);
		Index* lineIndices = new Index[MaxLineIndices];
		for (uint32_t i = 0; i < MaxLineIndices; i++)
			lineIndices[i] = i;
		m_LineIndexBuffer = IndexBuffer::Create(lineIndices, MaxLineIndices);
		delete[] lineIndices;

		m_LineVertexBasePtr = new LineVertex[MaxLineVertices];

#if SK_DEBUG
		for (uint32_t i = 0; i < MaxTextureSlots; i++)
			m_WhiteTexture->Bind(m_RenderCommandBuffer, i);
#endif

	}

	void Renderer2D::ShutDown()
	{

	}

	void Renderer2D::BeginScene(const DirectX::XMMATRIX& viewProj)
	{
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
		m_LineIndexCount = 0;
		m_LineVertexIndexPtr = m_LineVertexBasePtr;

		memset(&m_Stats, 0, sizeof(Statistics));
		m_Stats.TextureCount = 1;
	}

	void Renderer2D::EndScene()
	{
		m_RenderCommandBuffer->Begin();

		m_RenderTarget->ClearAtachment(m_RenderCommandBuffer, 0);
		m_RenderTarget->ClearAtachment(m_RenderCommandBuffer, 1, { -1.0f, -1.0f, -1.0f, -1.0f });
		m_RenderTarget->ClearDepth(m_RenderCommandBuffer);

		Renderer::BeginRenderPass(m_RenderCommandBuffer, m_RenderTarget);

#if SK_DEBUG
		for (uint32_t i = 0; i < MaxTextureSlots; i++)
			m_WhiteTexture->Bind(m_RenderCommandBuffer, i);
#endif

		// Quad
		{
			uint32_t dataSize = (uint32_t)((uint8_t*)m_QuadVertexIndexPtr - (uint8_t*)m_QuadVertexBasePtr);
			if (dataSize)
			{
				m_QuadVertexBuffer->SetData(m_QuadVertexBasePtr, dataSize);

				Renderer::RenderGeometry(m_RenderCommandBuffer, m_RenderTarget, m_QuadShader, m_ConstantBufferSet, m_QuadTextureArray, m_QuadVertexBuffer, m_QuadIndexBuffer, m_QuadIndexCount, PrimitveTopology::Triangle);
				m_Stats.DrawCalls++;
			}
		}

		// Circle
		{
			uint32_t dataSize = (uint32_t)((uint8_t*)m_CircleVertexIndexPtr - (uint8_t*)m_CircleVertexBasePtr);
			if (dataSize)
			{
				m_CircleVertexBuffer->SetData(m_CircleVertexBasePtr, dataSize);

				Renderer::RenderGeometry(m_RenderCommandBuffer, m_RenderTarget, m_CircleShader, m_ConstantBufferSet, nullptr, m_CircleVertexBuffer, m_QuadIndexBuffer, m_CircleIndexCount, PrimitveTopology::Triangle);
				m_Stats.DrawCalls++;
			}
		}

		// Line
		{
			uint32_t dataSize = (uint32_t)((uint8_t*)m_LineVertexIndexPtr - (uint8_t*)m_LineVertexBasePtr);
			if (dataSize)
			{
				m_LineVertexBuffer->SetData(m_LineVertexBasePtr, dataSize);

				Renderer::RenderGeometry(m_RenderCommandBuffer, m_RenderTarget, m_LineShader, m_ConstantBufferSet, nullptr, m_LineVertexBuffer, m_LineIndexBuffer, m_LineIndexCount, PrimitveTopology::Line);
				m_Stats.DrawCalls++;
			}
		}

		Renderer::EndRenderPass(m_RenderCommandBuffer);
		m_RenderCommandBuffer->End();
		m_RenderCommandBuffer->Execute();
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
				m_Stats.TextureCount++;
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

		m_Stats.QuadCount++;
		m_Stats.VertexCount += 4;
		m_Stats.IndexCount += 6;

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
				m_Stats.TextureCount++;
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

		m_Stats.QuadCount++;
		m_Stats.VertexCount += 4;
		m_Stats.IndexCount += 6;
	}


	void Renderer2D::DrawFilledCircle(const DirectX::XMFLOAT2& position, const DirectX::XMFLOAT2& scaling, float thickness, const DirectX::XMFLOAT4& color, int id)
	{
		DrawFilledCircle({ position.x, position.y, 0.0f }, { scaling.x, scaling.y, 1.0f }, thickness, color, id);
	}

	void Renderer2D::DrawFilledCircle(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& scaling, float thickness, const DirectX::XMFLOAT4& color, int id)
	{
		if (m_CircleIndexCount >= MaxCircleIndices)
			FlushAndResetCircle();

		const auto translation = DirectX::XMMatrixScaling(scaling.x, scaling.y, scaling.x) * DirectX::XMMatrixTranslation(position.x, position.y, position.z);

		for (uint32_t i = 0; i < 4; i++)
		{
			CircleVertex* vtx = m_CircleVertexIndexPtr++;
			vtx->WorldPosition = DXMath::Store3(DirectX::XMVector3Transform(DXMath::Load(QuadVertexPositions[i]), translation));
			vtx->LocalPosition = { QuadVertexPositions[i].x, QuadVertexPositions[i].y };
			vtx->Color = color;
			vtx->Thickness = thickness;
			vtx->ID = id;
		}

		m_CircleIndexCount += 6;

		m_Stats.CircleCount++;
		m_Stats.VertexCount += 4;
		m_Stats.IndexCount += 6;
	}

	void Renderer2D::DrawFilledCircle(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& rotation, const DirectX::XMFLOAT3& scaling, float thickness, const DirectX::XMFLOAT4& color, int id)
	{
		if (m_CircleIndexCount >= MaxCircleIndices)
			FlushAndResetCircle();

		const auto translation = DirectX::XMMatrixScaling(scaling.x, scaling.y, scaling.z) * DirectX::XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z) * DirectX::XMMatrixTranslation(position.x, position.y, position.z);

		for (uint32_t i = 0; i < 4; i++)
		{
			CircleVertex* vtx = m_CircleVertexIndexPtr++;
			vtx->WorldPosition = DXMath::Store3(DirectX::XMVector3Transform(DXMath::Load(QuadVertexPositions[i]), translation));
			vtx->LocalPosition = { QuadVertexPositions[i].x, QuadVertexPositions[i].y };
			vtx->Color = color;
			vtx->Thickness = thickness;
			vtx->ID = id;
		}

		m_CircleIndexCount += 6;

		m_Stats.CircleCount++;
		m_Stats.VertexCount += 4;
		m_Stats.IndexCount += 6;
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
		if (m_LineIndexCount >= MaxLineIndices)
			FlushAndResetLine();

		LineVertex* vtx = m_LineVertexIndexPtr++;
		vtx->WorldPosition = pos0;
		vtx->Color = color;
		vtx->ID = id;

		vtx = m_LineVertexIndexPtr++;
		vtx->WorldPosition = pos1;
		vtx->Color = color;
		vtx->ID = id;

		m_LineIndexCount += 2;
		
		m_Stats.LineCount++;
		m_Stats.VertexCount += 2;
		m_Stats.IndexCount += 2;
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

	void Renderer2D::DrawRect(const DirectX::XMFLOAT2& position, float rotation, const DirectX::XMFLOAT3& scaling, const DirectX::XMFLOAT4& color, int id)
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

	void Renderer2D::FlushAndResetQuad()
	{
		m_RenderCommandBuffer->Begin();

		uint32_t dataSize = (uint32_t)((uint8_t*)m_QuadVertexIndexPtr - (uint8_t*)m_QuadVertexBasePtr);
		if (dataSize)
		{
			m_QuadVertexBuffer->SetData(m_QuadVertexBasePtr, dataSize);

			Renderer::RenderGeometry(m_RenderCommandBuffer, m_RenderTarget, m_QuadShader, m_ConstantBufferSet, m_QuadTextureArray, m_QuadVertexBuffer, m_QuadIndexBuffer, m_QuadIndexCount, PrimitveTopology::Triangle);
			m_Stats.DrawCalls++;
		}

		m_RenderCommandBuffer->End();

		m_QuadIndexCount = 0;
		m_QuadVertexIndexPtr = m_QuadVertexBasePtr;
		m_QuadTextureSlotIndex = 1;
		m_QuadTextureArray->Set(0, m_WhiteTexture);
		for (uint32_t i = 1; i < MaxTextureSlots; i++)
			m_QuadTextureArray->Set(i, nullptr);

	}

	void Renderer2D::FlushAndResetCircle()
	{
		m_RenderCommandBuffer->Begin();

		uint32_t dataSize = (uint32_t)((uint8_t*)m_CircleVertexIndexPtr - (uint8_t*)m_CircleVertexBasePtr);
		if (dataSize)
		{
			m_CircleVertexBuffer->SetData(m_CircleVertexBasePtr, dataSize);

			Renderer::RenderGeometry(m_RenderCommandBuffer, m_RenderTarget, m_CircleShader, m_ConstantBufferSet, nullptr, m_CircleVertexBuffer, m_QuadIndexBuffer, m_CircleIndexCount, PrimitveTopology::Triangle);
			m_Stats.DrawCalls++;
		}

		m_RenderCommandBuffer->End();

		m_CircleIndexCount = 0;
		m_CircleVertexIndexPtr = m_CircleVertexBasePtr;
	}

	void Renderer2D::FlushAndResetLine()
	{
		m_RenderCommandBuffer->Begin();

		uint32_t dataSize = (uint32_t)((uint8_t*)m_LineVertexIndexPtr - (uint8_t*)m_LineVertexBasePtr);
		if (dataSize)
		{
			m_LineVertexBuffer->SetData(m_LineVertexBasePtr, dataSize);

			Renderer::RenderGeometry(m_RenderCommandBuffer, m_RenderTarget, m_LineShader, m_ConstantBufferSet, nullptr, m_LineVertexBuffer, m_LineIndexBuffer, m_LineIndexCount, PrimitveTopology::Line);
			m_Stats.DrawCalls++;
		}

		m_RenderCommandBuffer->End();

		m_LineIndexCount = 0;
		m_LineVertexIndexPtr = m_LineVertexBasePtr;
	}

}
