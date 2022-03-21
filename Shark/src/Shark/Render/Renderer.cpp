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

	void Renderer::RenderFullScreenQuad(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Material> material)
	{
		s_RendererAPI->RenderFullScreenQuad(commandBuffer, pipeline, material);
	}

	void Renderer::RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, Ref<ConstantBufferSet> constantBufferSet, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, uint32_t indexCount)
	{
		s_RendererAPI->RenderGeometry(renderCommandBuffer, pipeline, material, constantBufferSet, vertexBuffer, indexBuffer, indexCount);
	}

	void Renderer::RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, Ref<ConstantBufferSet> constantBufferSet, Ref<VertexBuffer> vertexBuffer, uint32_t vertexCount)
	{
		s_RendererAPI->RenderGeometry(renderCommandBuffer, pipeline, material, constantBufferSet, vertexBuffer, vertexCount);
	}

	void Renderer::GenerateMips(Ref<Image2D> image)
	{
		s_RendererAPI->GenerateMips(image);
	}

	void Renderer::ClearAllCommandBuffers()
{
		s_RendererAPI->ClearAllCommandBuffers();
	}

	const RendererCapabilities& Renderer::GetCapabilities()
	{
		return s_RendererAPI->GetCapabilities();
	}

	Ref<ShaderLibrary> Renderer::GetShaderLib()
	{
		return s_RendererAPI->GetShaderLib();
	}

	Ref<Texture2D> Renderer::GetWhiteTexture()
	{
		return s_RendererAPI->GetWhiteTexture();
	}

	Ref<RendererAPI> Renderer::GetRendererAPI()
	{
		return s_RendererAPI;
	}

}