#include "skpch.h"
#include "Renderer.h"

#include "Platform/DirectX11/DirectXRenderer.h"

namespace Shark {

	static Ref<RendererAPI> s_RendererAPI = nullptr;
	static RendererAPIType s_API = RendererAPIType::None;

	void Renderer::Init()
	{
		switch (s_API)
		{
			case RendererAPIType::None:        SK_CORE_ASSERT(false, "RendererAPI not specified"); break;
			case RendererAPIType::DirectX11:   s_RendererAPI = Ref<DirectXRenderer>::Create(); break;
			default:
				SK_CORE_ASSERT(false, "Unkonw RendererAPI");
		}
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

	void Renderer::SetAPI(RendererAPIType api)
	{
		s_API = api;
	}

	RendererAPIType Renderer::GetAPI()
	{
		return s_API;
	}

}