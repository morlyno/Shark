#include "skpch.h"
#include "Renderer.h"

#include "Shark/Utility/Utility.h"

namespace Shark {

	static Ref<RendererAPI> s_RendererAPI = nullptr;

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
		s_RendererAPI = RendererAPI::Create();

		s_BaseData = new RendererBaseData;

		s_BaseData->ShaderLib.Load("assets/Shaders/Renderer2D_Quad.hlsl");
		s_BaseData->ShaderLib.Load("assets/Shaders/Renderer2D_Circle.hlsl");
		s_BaseData->ShaderLib.Load("assets/Shaders/Renderer2D_Line.hlsl");

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
	}

	void Renderer::ShutDown()
	{
		delete s_BaseData;
		s_RendererAPI = nullptr;
	}

	void Renderer::SubmitFullScreenQuad()
	{
		SK_CORE_ASSERT(false, "Not implemented");
	}

	void Renderer::RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<FrameBuffer> frameBuffer, Ref<Shader> shaders, Ref<ConstantBufferSet> constantBufferSet, Ref<Texture2DArray> textureArray, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, uint32_t indexCount, PrimitveTopology topology)
	{
		s_RendererAPI->RenderGeometry(renderCommandBuffer, frameBuffer, shaders, constantBufferSet, textureArray, vertexBuffer, indexBuffer, indexCount, topology);
	}

	void Renderer::RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<ConstantBufferSet> constantBufferSet, Ref<Texture2DArray> textureArray, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, uint32_t indexCount, PrimitveTopology topology)
	{
		s_RendererAPI->RenderGeometry(renderCommandBuffer, pipeline, constantBufferSet, textureArray, vertexBuffer, indexBuffer, indexCount, topology);
	}

	Ref<FrameBuffer> Renderer::GetFinaleCompositFrameBuffer()
	{
		return s_RendererAPI->GetFinaleCompositFrameBuffer();
	}

	Ref<RendererAPI> Renderer::GetRendererAPI()
	{
		return s_RendererAPI;
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