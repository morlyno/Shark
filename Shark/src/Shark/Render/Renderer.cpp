#include "skpch.h"
#include "Renderer.h"

#include "Shark/File/FileSystem.h"
#include "Shark/Debug/Profiler.h"

#include "Platform/DirectX11/DirectXRenderer.h"

#include <future>

#define SK_SIMULTE_MULTITHREADED 0

namespace Shark {

	Ref<RendererAPI> Renderer::s_RendererAPI = nullptr;

	struct ShaderDependencies
	{
		std::vector<Weak<Material>> Materials;
		std::vector<Weak<RenderPass>> RenderPasses;
	};

	struct RendererData
	{
		Ref<ShaderLibrary> m_ShaderLibrary;

		Ref<Texture2D> m_WhiteTexture;
		Ref<Texture2D> m_BlackTexture;
		Ref<TextureCube> m_BlackTextureCube;
		Ref<SamplerWrapper> m_ClampLinearSampler;

		std::unordered_map<uint64_t, ShaderDependencies> m_ShaderDependencies;
	};

	static RendererConfig s_Config = {};
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
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/SharkPBR.hlsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/Skybox.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/EnvironmentIrradiance.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/EquirectangularToCubeMap.glsl");

		// 2D
		//Renderer::GetShaderLibrary()->Load("Resources/Shaders/2D/Renderer2D_Quad.hlsl");
		//Renderer::GetShaderLibrary()->Load("Resources/Shaders/2D/Renderer2D_QuadTransparent.hlsl");
		//Renderer::GetShaderLibrary()->Load("Resources/Shaders/2D/Renderer2D_QuadDepthPass.hlsl");
		//Renderer::GetShaderLibrary()->Load("Resources/Shaders/2D/Renderer2D_Circle.hlsl");
		//Renderer::GetShaderLibrary()->Load("Resources/Shaders/2D/Renderer2D_CircleTransparent.hlsl");
		//Renderer::GetShaderLibrary()->Load("Resources/Shaders/2D/Renderer2D_CircleDepthPass.hlsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/2D/Renderer2D_Line.hlsl");
		//Renderer::GetShaderLibrary()->Load("Resources/Shaders/2D/Renderer2D_LineDepthPass.hlsl");
		//Renderer::GetShaderLibrary()->Load("Resources/Shaders/2D/Renderer2D_Composite.hlsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/2D/Renderer2D_Text.hlsl");

		// Misc
#if TODO
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/FullScreen.hlsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/CompositWidthDepth.hlsl");
#endif

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

			spec.Format = ImageFormat::RGBA32F;
			spec.DebugName = "Black Texture Cube";

			glm::vec4 imageData[6];
			memset(imageData, 0, sizeof(imageData));
			s_Data->m_BlackTextureCube = TextureCube::Create(spec, Buffer::FromArray(imageData));

			SamplerSpecification samplerSpec;
			samplerSpec.Filter = FilterMode::Linear;
			samplerSpec.Wrap = WrapMode::Clamp;
			s_Data->m_ClampLinearSampler = SamplerWrapper::Create(samplerSpec);
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

#if SK_SIMULTE_MULTITHREADED
		std::async(std::launch::async, [executeIndex]()
		{
			s_CommandQueue[executeIndex]->Execute();
		});
#else
		s_CommandQueue[executeIndex]->Execute();
#endif
		s_SingleThreadedIsExecuting = false;
	}

	void Renderer::BeginRenderPass(Ref<RenderCommandBuffer> commandBuffer, Ref<RenderPass> renderPass)
	{
		s_RendererAPI->BeginRenderPass(commandBuffer, renderPass);
	}

	void Renderer::EndRenderPass(Ref<RenderCommandBuffer> commandBuffer, Ref<RenderPass> renderPass)
	{
		s_RendererAPI->EndRenderPass(commandBuffer, renderPass);
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

	void Renderer::RenderCube(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Material> material)
	{
		s_RendererAPI->RenderCube(commandBuffer, pipeline, material);
	}

	void Renderer::RenderSubmesh(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Mesh> mesh, uint32_t submeshIndex)
	{
		s_RendererAPI->RenderSubmesh(commandBuffer, pipeline, mesh, submeshIndex);
	}

	void Renderer::RenderSubmeshWithMaterial(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Mesh> mesh, uint32_t submeshIndex, Ref<Material> material)
	{
		s_RendererAPI->RenderSubmeshWithMaterial(commandBuffer, pipeline, mesh, submeshIndex, material);
	}

	void Renderer::CopyImage(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> sourceImage, Ref<Image2D> destinationImage)
	{
		s_RendererAPI->CopyImage(commandBuffer, sourceImage, destinationImage);
	}

	void Renderer::RT_CopyImage(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> sourceImage, Ref<Image2D> destinationImage)
	{
		s_RendererAPI->RT_CopyImage(commandBuffer, sourceImage, destinationImage);
	}

	std::pair<Ref<TextureCube>, Ref<TextureCube>> Renderer::CreateEnvironmentMap(const std::filesystem::path& filepath)
	{
		return s_RendererAPI->CreateEnvironmentMap(filepath);
	}

	void Renderer::GenerateMips(Ref<Image2D> image)
	{
		s_RendererAPI->GenerateMips(image);
	}

	void Renderer::RT_GenerateMips(Ref<Image2D> image)
	{
		s_RendererAPI->RT_GenerateMips(image);
	}

	void Renderer::ShaderReloaded(Ref<Shader> shader)
	{
		auto& dependencies = s_Data->m_ShaderDependencies[shader->GetHash()];

		if (dependencies.Materials.empty() && dependencies.RenderPasses.empty())
			return;

		std::ranges::remove_if(dependencies.Materials, [](Weak<Material> material) { return material.Expired(); });
		std::ranges::remove_if(dependencies.RenderPasses, [](Weak<RenderPass> material) { return material.Expired(); });

		for (const auto& m : dependencies.Materials)
		{
			Ref<Material> material = m.GetRef();
			SK_CORE_VERIFY(material->Validate());
		}

		for (const auto& rp : dependencies.RenderPasses)
		{
			Ref<RenderPass> renderPass = rp.GetRef();
			SK_CORE_VERIFY(renderPass->Validate());
			renderPass->Bake();
		}

	}

	void Renderer::AcknowledgeShaderDependency(Ref<Shader> shader, Weak<Material> material)
	{
		s_Data->m_ShaderDependencies[shader->GetHash()].Materials.push_back(material);
	}

	void Renderer::AcknowledgeShaderDependency(Ref<Shader> shader, Weak<RenderPass> renderPass)
	{
		s_Data->m_ShaderDependencies[shader->GetHash()].RenderPasses.push_back(renderPass);
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

	uint32_t Renderer::GetCurrentFrameIndex()
	{
		return s_RendererAPI->GetCurrentFrameIndex();
	}

	uint32_t Renderer::RT_GetCurrentFrameIndex()
	{
		return s_RendererAPI->RT_GetCurrentFrameIndex();
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

	Ref<TextureCube> Renderer::GetBlackTextureCube()
	{
		return s_Data->m_BlackTextureCube;
	}

	Ref<SamplerWrapper> Renderer::GetClampLinearSampler()
	{
		return s_Data->m_ClampLinearSampler;
	}

	Ref<RenderCommandBuffer> Renderer::GetCommandBuffer()
	{
		return s_RendererAPI->GetCommandBuffer();
	}

	const RendererCapabilities& Renderer::GetCapabilities()
	{
		return s_RendererAPI->GetCapabilities();
	}

	bool Renderer::IsOnRenderThread()
	{
		return s_SingleThreadedIsExecuting;
	}

	RendererConfig& Renderer::GetConfig()
	{
		return s_Config;
	}

	void Renderer::SetConfig(const RendererConfig& config)
	{
		s_Config = config;
	}

	RenderCommandQueue& Renderer::GetCommandQueue()
	{
		return *s_CommandQueue[s_CommandQueueSubmissionIndex];
	}

}