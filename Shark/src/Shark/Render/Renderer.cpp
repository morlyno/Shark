#include "skpch.h"
#include "Renderer.h"

#include "Platform/DirectX11/DirectXRenderer.h"
#include "Shark/Debug/Profiler.h"

namespace Shark {

	Ref<RendererAPI> Renderer::s_RendererAPI = nullptr;
	static RendererAPIType s_API = RendererAPIType::None;

	static CommandQueue* s_RenderCommandQueue[2];

	void Renderer::Init()
	{
		s_RenderCommandQueue[0] = sknew CommandQueue(1024 * 10);
		s_RenderCommandQueue[1] = sknew CommandQueue(1024 * 10);

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
		skdelete s_RenderCommandQueue[0];
		skdelete s_RenderCommandQueue[1];
	}

	void Renderer::BeginFrame()
	{
		s_RendererAPI->BeginFrame();
	}

	void Renderer::EndFrame()
	{
		s_RendererAPI->EndFrame();
	}

	void Renderer::WaitAndRender()
	{
		SK_PROFILE_FUNCTION();
		SK_PERF_FUNCTION();

		std::swap(s_RenderCommandQueue[0], s_RenderCommandQueue[1]);
		s_RenderCommandQueue[1]->Execute();
	}

	bool Renderer::IsOnRenderThread()
	{
		return true;
	}

	void Renderer::RenderFullScreenQuad(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Material> material)
	{
		s_RendererAPI->RenderFullScreenQuad(commandBuffer, pipeline, material);
	}

	void Renderer::BeginBatch(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer)
	{
		s_RendererAPI->BeginBatch(renderCommandBuffer, pipeline, vertexBuffer, indexBuffer);
	}

	void Renderer::RenderBatch(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Material> material, uint32_t indexCount, uint32_t startIndex)
	{
		s_RendererAPI->RenderBatch(renderCommandBuffer, material, indexCount, startIndex);
	}

	void Renderer::EndBatch(Ref<RenderCommandBuffer> renderCommandBuffer)
	{
		s_RendererAPI->EndBatch(renderCommandBuffer);
	}

	void Renderer::RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, uint32_t indexCount)
	{
		s_RendererAPI->RenderGeometry(renderCommandBuffer, pipeline, material, vertexBuffer, indexBuffer, indexCount);
	}

	void Renderer::RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, Ref<VertexBuffer> vertexBuffer, uint32_t vertexCount)
	{
		s_RendererAPI->RenderGeometry(renderCommandBuffer, pipeline, material, vertexBuffer, vertexCount);
	}

	void Renderer::RenderMesh(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Mesh> mesh, Ref<Pipeline> pipeline)
	{
		s_RendererAPI->RenderMesh(renderCommandBuffer, mesh, pipeline);
	}

	void Renderer::RenderSubmesh(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Mesh> mesh, uint32_t submeshIndex, Ref<Pipeline> pipeline)
	{
		s_RendererAPI->RenderSubmesh(renderCommandBuffer, mesh, submeshIndex, pipeline);
	}

	void Renderer::GenerateMips(Ref<Image2D> image)
	{
		s_RendererAPI->GenerateMips(image);
	}

	void Renderer::RT_GenerateMips(Ref<Image2D> image)
	{
		s_RendererAPI->RT_GenerateMips(image);
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

	bool Renderer::ResourcesCreated()
	{
		return s_RendererAPI->ResourcesCreated();
	}

	bool Renderer::IsInsideFrame()
	{
		return s_RendererAPI ? s_RendererAPI->IsInsideFrame() : true;
	}

	void Renderer::ReportLiveObejcts()
	{
		switch (s_API)
		{
			case RendererAPIType::None: return;
			case RendererAPIType::DirectX11: DirectXRenderer::ReportLiveObejcts(); return;
		}

		SK_CORE_ASSERT(false, "Unkown Renderer API");
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

	CommandQueue& Renderer::GetCommandQueue()
	{
		return *s_RenderCommandQueue[0];
	}

	CommandQueue& Renderer::GetResourceFreeQueue()
	{
		return *s_RenderCommandQueue[0];
	}

	bool Renderer::IsDuringStartup()
	{
		return Application::Get().GetApplicationState() == ApplicationState::Startup;
	}

	bool Renderer::IsDuringShutdown()
	{
		return Application::Get().GetApplicationState() == ApplicationState::Shutdown;
	}

}