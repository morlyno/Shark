#include "skpch.h"
#include "Renderer.h"

#include "Shark/Render/Renderer2D.h"
#include "Shark/Render/Buffers.h"
#include "Shark/Utility/Utility.h"

#include "Shark/Debug/Instrumentor.h"

namespace Shark {

	struct RendererBaseData
	{
		ShaderLibrary ShaderLib;
		Ref<Texture2D> WhiteTexture;

		Ref<VertexBuffer> QuadVB;
		Ref<IndexBuffer> QuadIB;
	};
	static RendererBaseData* s_BaseData = nullptr;

	void Renderer::Init()
	{
		SK_PROFILE_FUNCTION();

		RendererCommand::Init();

		s_BaseData = new RendererBaseData;

		s_BaseData->ShaderLib.Load("assets/Shaders/BatchShader2D_Quad.hlsl");
		s_BaseData->ShaderLib.Load("assets/Shaders/BatchShader2D_Circle.hlsl");
		s_BaseData->ShaderLib.Load("assets/Shaders/BatchShader2D_Line.hlsl");

		s_BaseData->ShaderLib.Load("assets/Shaders/FullScreen.hlsl");
		s_BaseData->ShaderLib.Load("assets/Shaders/NegativeEffect.hlsl");
		s_BaseData->ShaderLib.Load("assets/Shaders/BlurEffect.hlsl");
		
		uint32_t color = 0xFFFFFFFF;
		s_BaseData->WhiteTexture = Texture2D::Create(1, 1, &color);

		float vertices[4 * 2] = {
			-1.0f,  1.0f,
			 1.0f,  1.0f,
			 1.0f, -1.0f,
			-1.0f, -1.0f
		};
		uint32_t indices[3 * 2] = {
			0, 1, 2,
			2, 3, 0
		};
		VertexLayout layout = {
			{ VertexDataType::Float2, "Position" }
		};

		s_BaseData->QuadVB = VertexBuffer::Create(layout, vertices, sizeof(vertices));
		s_BaseData->QuadIB = IndexBuffer::Create(indices, Utility::ArraySize(indices));
		Renderer2D::Init();
	}

	void Renderer::ShutDown()
	{
		SK_PROFILE_FUNCTION();

		Renderer2D::ShutDown();

		delete s_BaseData;

		RendererCommand::ShutDown();
	}

	void Renderer::SubmitFullScreenQuad()
	{
		s_BaseData->QuadIB->Bind();
		s_BaseData->QuadVB->Bind();
		RendererCommand::DrawIndexed(s_BaseData->QuadIB->GetCount(), PrimitveTopology::Triangle);
		s_BaseData->QuadVB->UnBind();
		s_BaseData->QuadIB->UnBind();
	}

	void Renderer::SubmitGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Shaders> shaders, Ref<ConstantBufferSet> constantBufferSet, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, uint32_t indexCount, PrimitveTopology topology)
	{
		shaders->Bind();
		constantBufferSet->Bind();
		vertexBuffer->Bind();
		indexBuffer->Bind();
		RendererCommand::DrawIndexed(indexCount, topology);
	}

	ShaderLibrary& Renderer::GetShaderLib()
	{
		return s_BaseData->ShaderLib;
	}

	Ref<Texture2D> Renderer::GetWhiteTexture()
	{
		return s_BaseData->WhiteTexture;
	}

}