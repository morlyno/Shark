#include "skpch.h"
#include "Renderer.h"

#include "Shark/Asset/AssetManager.h"
#include "Shark/File/FileSystem.h"
#include "Shark/Debug/Profiler.h"

#include "Platform/DirectX11/DirectXRenderer.h"

#include <future>

#define SK_SIMULTE_MULTITHREADED 0

namespace Shark {

	Ref<RendererContext> Renderer::s_RendererContext = nullptr;
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
		Ref<Environment> m_EmptyEnvironment;
		Ref<Texture2D> m_BRDFLUTTexture;

		Ref<DirectXVertexBuffer> m_QuadVertexBuffer;
		Ref<DirectXIndexBuffer> m_QuadIndexBuffer;
		Ref<DirectXVertexBuffer> m_CubeVertexBuffer;
		Ref<DirectXIndexBuffer> m_CubeIndexBuffer;

		std::unordered_map<uint64_t, ShaderDependencies> m_ShaderDependencies;
	};

	static RendererConfig s_Config = {};
	static RendererCapabilities s_Capabilities;
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

		s_RendererContext = RendererContext::Create();
		s_RendererAPI = RendererAPI::Create();
		s_RendererAPI->Init();

		s_Data = sknew RendererData;
		s_Data->m_ShaderLibrary = Ref<ShaderLibrary>::Create();

		// 3D
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/SharkPBR.hlsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/Skybox.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/EnvironmentIrradiance.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/EquirectangularToCubeMap.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/EnvironmentMipFilter.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/BRDF_LUT.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/Tonemap.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/Composite.glsl");

		// 2D
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/2D/Renderer2D_Quad.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/2D/Renderer2D_Circle.hlsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/2D/Renderer2D_Line.hlsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/2D/Renderer2D_Text.hlsl");

		// Jump Flood
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/JumpFlood/SelectedGeometry.hlsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/JumpFlood/JumpFloodInit.hlsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/JumpFlood/JumpFloodPass.hlsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/JumpFlood/JumpFloodComposite.hlsl");


		// Commands
		Renderer::GetShaderLibrary()->Load("resources/Shaders/Commands/BlitImage.glsl");

		// Misc
#if TODO
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/FullScreen.hlsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/CompositWidthDepth.hlsl");
#endif

		// Compile Shaders
		Renderer::WaitAndRender();

		{
			float vertices[4 * 5] = {
				-1.0f,  1.0f, 0.0f, 0.0f, 0.0f,
				 1.0f,  1.0f, 0.0f, 1.0f, 0.0f,
				 1.0f, -1.0f, 0.0f, 1.0f, 1.0f,
				-1.0f, -1.0f, 0.0f, 0.0f, 1.0f
			};

			uint32_t indices[3 * 2] = {
				0, 1, 2,
				2, 3, 0
			};

			s_Data->m_QuadVertexBuffer = Ref<DirectXVertexBuffer>::Create((uint32_t)sizeof(vertices), false, Buffer::FromArray(vertices));
			s_Data->m_QuadIndexBuffer = Ref<DirectXIndexBuffer>::Create((uint32_t)std::size(indices), false, Buffer::FromArray(indices));
		}

		{
			glm::vec3 vertices[] = {
				{ -1, -1, -1 },
				{ 1, -1, -1 },
				{ -1, 1, -1 },
				{ 1, 1, -1 },
				{ -1, -1, 1 },
				{ 1, -1, 1 },
				{ -1, 1, 1 },
				{ 1, 1, 1 },
			};

			uint32_t indices[] = {
				0, 2, 1, 2, 3, 1,
				1, 3, 5, 3, 7, 5,
				2, 6, 3, 3, 6, 7,
				4, 5, 7, 4, 7, 6,
				0, 4, 2, 2, 4, 6,
				0, 1, 4, 1, 5, 4
			};

			s_Data->m_CubeVertexBuffer = VertexBuffer::Create(Buffer::FromArray(vertices)).As<DirectXVertexBuffer>();
			s_Data->m_CubeIndexBuffer = IndexBuffer::Create(Buffer::FromArray(indices)).As<DirectXIndexBuffer>();
		}

		s_Data->m_BRDFLUTTexture = s_RendererAPI->CreateBRDFLUT();

		{
			TextureSpecification spec;
			spec.Format = ImageFormat::RGBA8UNorm;
			spec.Width = 1;
			spec.Height = 1;
			spec.GenerateMips = false;

			spec.DebugName = "White Texture";
			s_Data->m_WhiteTexture = Texture2D::Create(spec, Buffer::FromValue(0xFFFFFFFF));

			spec.DebugName = "Black Texture";
			s_Data->m_BlackTexture = Texture2D::Create(spec, Buffer::FromValue(0x00000000));

			spec.Format = ImageFormat::RGBA32Float;
			spec.DebugName = "Black Texture Cube";

			glm::vec4 imageData[6];
			memset(imageData, 0, sizeof(imageData));
			spec.Format = ImageFormat::RGBA32Float;
			s_Data->m_BlackTextureCube = TextureCube::Create(spec, Buffer::FromArray(imageData));

			s_Data->m_EmptyEnvironment = Ref<Environment>::Create(s_Data->m_BlackTextureCube, s_Data->m_BlackTextureCube);
		}
	}

	void Renderer::ShutDown()
	{
		SK_PROFILE_FUNCTION();

		s_Data->m_QuadVertexBuffer = nullptr;
		s_Data->m_QuadIndexBuffer = nullptr;
		s_Data->m_CubeVertexBuffer = nullptr;
		s_Data->m_CubeIndexBuffer = nullptr;

		s_Data->m_ShaderLibrary = nullptr;
		s_Data->m_WhiteTexture = nullptr;
		skdelete s_Data;

		// Execute all (resource free) commands submitted  before the shutdown happens
		// The RendererAPI and RendererContext will not submit any commands to the command queue after this point
		Renderer::WaitAndRender();

		s_RendererAPI->ShutDown();
		s_RendererAPI = nullptr;
		s_RendererContext->DestroyDevice();
		s_RendererContext->ReportLiveObjects();
		s_RendererContext = nullptr;

		skdelete s_CommandQueue[0];
		skdelete s_CommandQueue[1];
	}

	void Renderer::BeginEventMarker(Ref<RenderCommandBuffer> commandBuffer, const std::string& name)
	{
		s_RendererAPI->BeginEventMarker(commandBuffer, name);
	}

	void Renderer::EndEventMarker(Ref<RenderCommandBuffer> commandBuffer)
	{
		s_RendererAPI->EndEventMarker(commandBuffer);
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

	void Renderer::BeginRenderPass(Ref<RenderCommandBuffer> commandBuffer, Ref<RenderPass> renderPass, bool expliciteClear)
	{
		s_RendererAPI->BeginRenderPass(commandBuffer, renderPass, expliciteClear);
	}

	void Renderer::EndRenderPass(Ref<RenderCommandBuffer> commandBuffer, Ref<RenderPass> renderPass)
	{
		s_RendererAPI->EndRenderPass(commandBuffer, renderPass);
	}

	void Renderer::RenderFullScreenQuad(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Material> material)
	{
		Renderer::RenderGeometry(commandBuffer, pipeline, material, s_Data->m_QuadVertexBuffer, s_Data->m_QuadIndexBuffer, s_Data->m_QuadIndexBuffer->GetCount());
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
		RenderGeometry(commandBuffer, pipeline, material, s_Data->m_CubeVertexBuffer, s_Data->m_CubeIndexBuffer, s_Data->m_CubeIndexBuffer->GetCount());
	}

	void Renderer::RenderSubmesh(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Mesh> mesh, uint32_t submeshIndex, Ref<MaterialTable> materialTable)
	{
		s_RendererAPI->RenderSubmesh(commandBuffer, pipeline, mesh, submeshIndex, materialTable);
	}

	void Renderer::RenderSubmeshWithMaterial(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Mesh> mesh, Ref<MeshSource> meshSource, uint32_t submeshIndex, Ref<Material> material)
	{
		s_RendererAPI->RenderSubmeshWithMaterial(commandBuffer, pipeline, mesh, meshSource, submeshIndex, material);
	}

	void Renderer::CopyImage(Ref<Image2D> sourceImage, Ref<Image2D> destinationImage)
	{
		s_RendererAPI->CopyImage(sourceImage, destinationImage);
	}

	void Renderer::CopyImage(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> sourceImage, Ref<Image2D> destinationImage)
	{
		s_RendererAPI->CopyImage(commandBuffer, sourceImage, destinationImage);
	}

	void Renderer::CopyMip(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> sourceImage, uint32_t sourceMip, Ref<Image2D> destinationImage, uint32_t destinationMip)
	{
		s_RendererAPI->CopyMip(commandBuffer, sourceImage, sourceMip, destinationImage, destinationMip);
	}

	void Renderer::BlitImage(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> sourceImage, Ref<Image2D> destinationImage)
	{
		s_RendererAPI->BlitImage(commandBuffer, sourceImage, destinationImage);
	}

	void Renderer::GenerateMips(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> image)
	{
		s_RendererAPI->GenerateMips(commandBuffer, image);
	}

	std::pair<Ref<TextureCube>, Ref<TextureCube>> Renderer::CreateEnvironmentMap(const std::filesystem::path& filepath)
	{
		return s_RendererAPI->CreateEnvironmentMap(filepath);
	}

	std::pair<Ref<TextureCube>, Ref<TextureCube>> Renderer::RT_CreateEnvironmentMap(const std::filesystem::path& filepath)
	{
		return s_RendererAPI->RT_CreateEnvironmentMap(filepath);
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

		std::erase_if(dependencies.Materials, [](Weak<Material> material) { return material.Expired(); });
		std::erase_if(dependencies.RenderPasses, [](Weak<RenderPass> renderPass) { return renderPass.Expired(); });

		for (const auto& m : dependencies.Materials)
		{
			Ref<Material> material = m.GetRef();
			SK_CORE_VERIFY(material->Validate());
			material->Prepare();
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
		s_RendererContext->ReportLiveObjects();
	}

	uint32_t Renderer::GetCurrentFrameIndex()
	{
		return s_RendererAPI->GetCurrentFrameIndex();
	}

	uint32_t Renderer::RT_GetCurrentFrameIndex()
	{
		return s_RendererAPI->RT_GetCurrentFrameIndex();
	}

	Ref<RendererContext> Renderer::GetRendererContext()
	{
		return s_RendererContext;
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

	Ref<Environment> Renderer::GetEmptyEnvironment()
	{
		return s_Data->m_EmptyEnvironment;
	}

	Ref<Texture2D> Renderer::GetBRDFLUTTexture()
	{
		return s_Data->m_BRDFLUTTexture;
	}

	RendererCapabilities& Renderer::GetCapabilities()
	{
		return s_Capabilities;
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
		SK_CORE_VERIFY(std::this_thread::get_id() == Application::Get().GetMainThreadID());
		return *s_CommandQueue[s_CommandQueueSubmissionIndex];
	}

	RenderCommandQueue& Renderer::GetResourceFreeQueue()
	{
		return *s_CommandQueue[s_CommandQueueSubmissionIndex];
	}

}