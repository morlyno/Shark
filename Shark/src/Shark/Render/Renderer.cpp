#include "skpch.h"
#include "Renderer.h"

#include "Shark/Utility/Utility.h"

namespace Shark {

	static Ref<RendererAPI> s_RendererAPI = nullptr;

	void Renderer::Init()
	{
		s_RendererAPI = RendererAPI::Create();
	}

	void Renderer::ShutDown()
	{
		s_RendererAPI = nullptr;
	}

	void Renderer::NewFrame()
	{
		s_RendererAPI->NewFrame();
	}

	void Renderer::RenderFullScreenQuad(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Image2D> image)
	{
		s_RendererAPI->RenderFullScreenQuad(commandBuffer, pipeline, image);
	}

	void Renderer::RenderFullScreenQuadWidthDepth(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Image2D> image, Ref<Image2D> depthImage)
	{
		s_RendererAPI->RenderFullScreenQuadWidthDepth(commandBuffer, pipeline, image, depthImage);
	}

	void Renderer::RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<FrameBuffer> frameBuffer, Ref<Shader> shaders, Ref<ConstantBufferSet> constantBufferSet, Ref<Texture2DArray> textureArray, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, uint32_t indexCount, PrimitveTopology topology)
	{
		s_RendererAPI->RenderGeometry(renderCommandBuffer, frameBuffer, shaders, constantBufferSet, textureArray, vertexBuffer, indexBuffer, indexCount, topology);
	}

	void Renderer::RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<ConstantBufferSet> constantBufferSet, Ref<Texture2DArray> textureArray, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, uint32_t indexCount, PrimitveTopology topology)
	{
		s_RendererAPI->RenderGeometry(renderCommandBuffer, pipeline, constantBufferSet, textureArray, vertexBuffer, indexBuffer, indexCount, topology);
	}

	void Renderer::RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<ConstantBufferSet> constantBufferSet, Ref<Texture2DArray> textureArray, Ref<VertexBuffer> vertexBuffer, uint32_t vertexCount, PrimitveTopology topology)
	{
		s_RendererAPI->RenderGeometry(renderCommandBuffer, pipeline, constantBufferSet, textureArray, vertexBuffer, vertexCount, topology);
	}

	Ref<ShaderLibrary> Renderer::GetShaderLib()
	{
		return s_RendererAPI->GetShaderLib();
	}

	Ref<Texture2D> Renderer::GetWhiteTexture()
	{
		return s_RendererAPI->GetWhiteTexture();
	}

	Ref<GPUTimer> Renderer::GetPresentTimer()
	{
		return s_RendererAPI->GetPresentTimer();
	}

	Ref<FrameBuffer> Renderer::GetFinaleCompositFrameBuffer()
	{
		return s_RendererAPI->GetFinaleCompositFrameBuffer();
	}

	Ref<RendererAPI> Renderer::GetRendererAPI()
	{
		return s_RendererAPI;
	}

}