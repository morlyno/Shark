#include "skpch.h"
#include "Renderer.h"

#include "Platform/DirectX11/DirectXRenderer.h"
#include "Shark/Debug/Profiler.h"
#include <future>

namespace Shark {

	Ref<RendererAPI> Renderer::s_RendererAPI = nullptr;

	struct RendererData
	{
		Ref<ShaderLibrary> m_ShaderLibrary;

		Ref<Texture2D> m_WhiteTexture;
		Ref<Texture2D> m_BlackTexture;
	};

	static RendererData* s_Data = nullptr;
	static constexpr uint32_t s_CommandQueueCount = 2;
	static RenderCommandQueue* s_CommandQueue[s_CommandQueueCount];
	static uint32_t s_CommandQueueSubmissionIndex = 0;

	static bool s_SingleThreadedIsExecuting = false;

	Ref<RendererAPI> RendererAPI::Create()
	{
		switch (RendererAPI::GetCurrentAPI())
		{
			case RendererAPIType::None:        SK_CORE_ASSERT(false, "RendererAPI not specified"); break;
			case RendererAPIType::DirectX11:   return Ref<DirectXRenderer>::Create(); break;
		}

		SK_CORE_ASSERT(false, "Unkown RendererAPI");
		return nullptr;
	}

	void Renderer::Init()
	{
		SK_PROFILE_FUNCTION();

		s_CommandQueue[0] = sknew RenderCommandQueue();
		s_CommandQueue[1] = sknew RenderCommandQueue();

		s_RendererAPI = RendererAPI::Create();

		s_Data = sknew RendererData;
		s_Data->m_ShaderLibrary = Ref<ShaderLibrary>::Create();

		// 3D
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/DefaultMeshShader.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/SharkPBR.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/FlatColor.glsl");

		// 2D
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/Renderer2D_Quad.hlsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/Renderer2D_QuadTransparent.hlsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/Renderer2D_QuadDepthPass.hlsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/Renderer2D_Circle.hlsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/Renderer2D_CircleTransparent.hlsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/Renderer2D_CircleDepthPass.hlsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/Renderer2D_Line.hlsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/Renderer2D_LineDepthPass.hlsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/Renderer2D_Composite.hlsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/Renderer2D_Text.hlsl");

		// Misc
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/FullScreen.hlsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/CompositWidthDepth.hlsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/NegativeEffect.hlsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/BlurEffect.hlsl");

		// Compile Shaders
		Renderer::WaitAndRender();

		{
			TextureSpecification spec;
			spec.Format = ImageFormat::RGBA8;
			spec.Width = 1;
			spec.Height = 1;
			spec.GenerateMips = false;

			spec.DebugName = "White Texture";
			s_Data->m_WhiteTexture = Texture2D::Create(spec, Buffer::FromValue(0xFFFFFFFF));

			spec.DebugName = "Balck Texture";
			s_Data->m_BlackTexture = Texture2D::Create(spec, Buffer::FromValue(0x00000000));
		}
	}

	void Renderer::ShutDown()
	{
		SK_PROFILE_FUNCTION();

		s_Data->m_ShaderLibrary = nullptr;
		s_Data->m_WhiteTexture = nullptr;
		skdelete s_Data;

		s_RendererAPI = nullptr;
		Renderer::WaitAndRender();

		skdelete s_CommandQueue[0];
		skdelete s_CommandQueue[1];
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
		SK_PERF_SCOPED("Renderer::WaitAndRender");

		const uint32_t executeIndex = s_CommandQueueSubmissionIndex;
		s_CommandQueueSubmissionIndex = (s_CommandQueueSubmissionIndex + 1) & s_CommandQueueCount;

		s_SingleThreadedIsExecuting = true;
		std::async(std::launch::async, [executeIndex]()
		{
			s_CommandQueue[executeIndex]->Execute();
		});
		s_SingleThreadedIsExecuting = false;
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

	void Renderer::RenderSubmesh(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Mesh> mesh, uint32_t submeshIndex, Ref<Pipeline> pipeline)
	{
		s_RendererAPI->RenderSubmesh(renderCommandBuffer, mesh, submeshIndex, pipeline);
	}

	void Renderer::RenderSubmesh(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Mesh> mesh, uint32_t submeshIndex, Ref<ConstantBuffer> sceneData, Ref<ConstantBuffer> meshData, Ref<ConstantBuffer> lightData)
	{
		s_RendererAPI->RenderSubmesh(commandBuffer, pipeline, mesh, submeshIndex, sceneData, meshData, lightData);
	}

	void Renderer::RenderSubmeshWithMaterial(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Mesh> mesh, uint32_t submeshIndex, Ref<Material> material, Ref<ConstantBuffer> sceneData)
	{
		s_RendererAPI->RenderSubmeshWithMaterial(commandBuffer, pipeline, mesh, submeshIndex, material, sceneData);
	}

	void Renderer::GenerateMips(Ref<Image2D> image)
	{
		s_RendererAPI->GenerateMips(image);
	}

	void Renderer::RT_GenerateMips(Ref<Image2D> image)
	{
		s_RendererAPI->RT_GenerateMips(image);
	}

	void Renderer::ReportLiveObejcts()
	{
		switch (RendererAPI::GetCurrentAPI())
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

	Ref<ShaderLibrary> Renderer::GetShaderLibrary()
	{
		return s_Data->m_ShaderLibrary;
	}

	Ref<Texture2D> Renderer::GetWhiteTexture()
	{
		return s_Data->m_WhiteTexture;
	}

	Ref<Texture2D> Renderer::GetBlackTexture()
	{
		return s_Data->m_BlackTexture;
	}

	const RendererCapabilities& Renderer::GetCapabilities()
	{
		return s_RendererAPI->GetCapabilities();
	}

	bool Renderer::IsOnRenderThread()
	{
		return s_SingleThreadedIsExecuting;
	}

	RenderCommandQueue& Renderer::GetCommandQueue()
	{
		return *s_CommandQueue[s_CommandQueueSubmissionIndex];
	}

}