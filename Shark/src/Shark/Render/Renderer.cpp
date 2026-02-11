#include "skpch.h"
#include "Renderer.h"

#include "Shark/Render/RendererRT.h"
#include "Shark/Serialization/Import/TextureImporter.h"

#include "Shark/Debug/Profiler.h"

#include <nvrhi/utils.h>

#define SK_SIMULTE_MULTITHREADED 0

namespace Shark {

	struct ShaderDependencies
	{
		std::vector<Weak<Material>> Materials;
		std::vector<Weak<RenderPass>> RenderPasses;
	};

	struct RendererData
	{
		Ref<ShaderLibrary> m_ShaderLibrary;
		ResourceCache m_ResourceCache;

		Ref<Texture2D> m_WhiteTexture;
		Ref<Texture2D> m_BlackTexture;
		Ref<TextureCube> m_BlackTextureCube;
		Ref<Environment> m_EmptyEnvironment;
		Ref<Image2D> m_BRDFLUTTexture;

		Ref<VertexBuffer> m_QuadVertexBuffer;
		Ref<IndexBuffer> m_QuadIndexBuffer;
		Ref<VertexBuffer> m_CubeVertexBuffer;
		Ref<IndexBuffer> m_CubeIndexBuffer;

		Samplers m_Samplers;

		// This is needed for GenerateMips but i want it gone
		std::array<Ref<Image2D>, 3> m_NullUAVs;
		std::array<Ref<Image2D>, 3> m_NullArrayUAVs;

		ShaderCache m_ShaderCache;
		std::unordered_map<uint64_t, ShaderDependencies> m_ShaderDependencies;

		uint32_t m_FrameIndex = (uint32_t)-1;
		uint32_t m_RTFrameIndex = (uint32_t)-1;
	};

	static RendererConfig s_Config = {};
	static RendererCapabilities s_Capabilities;
	static RendererData* s_Data = nullptr;

	static RenderCommandQueue* s_CommandQueue[2];
	static RenderCommandQueue* s_MTCommandQueue[2];
	static std::mutex s_MTCommandQueueMutex;


	static bool s_SingleThreadedIsExecuting = false;

	void Renderer::Init()
	{
		SK_PROFILE_FUNCTION();

		s_CommandQueue[0] = sknew RenderCommandQueue();
		s_CommandQueue[1] = sknew RenderCommandQueue();

		s_MTCommandQueue[0] = sknew RenderCommandQueue(1024 * 10);
		s_MTCommandQueue[1] = sknew RenderCommandQueue(1024 * 10);

		s_Data = sknew RendererData;
		s_Data->m_ShaderLibrary = Ref<ShaderLibrary>::Create();
		s_Data->m_ShaderCache.LoadRegistry();

		s_Data->m_Samplers.NearestRepeat       = Sampler::Create({ .Filter = FilterMode::Nearest, .Address = AddressMode::Repeat });
		s_Data->m_Samplers.NearestClamp        = Sampler::Create({ .Filter = FilterMode::Nearest, .Address = AddressMode::ClampToEdge });
		s_Data->m_Samplers.NearestMirrorRepeat = Sampler::Create({ .Filter = FilterMode::Nearest, .Address = AddressMode::MirrorRepeat });
		s_Data->m_Samplers.LinearRepeat        = Sampler::Create({ .Filter = FilterMode::Linear,  .Address = AddressMode::Repeat });
		s_Data->m_Samplers.LinearClamp         = Sampler::Create({ .Filter = FilterMode::Linear,  .Address = AddressMode::ClampToEdge });
		s_Data->m_Samplers.LinearMirrorRepeat  = Sampler::Create({ .Filter = FilterMode::Linear,  .Address = AddressMode::MirrorRepeat });

		Timer loadShadersTimer;
		SK_CORE_INFO_TAG("Renderer", "Loading Shaders...");

		auto shaderLibrary = GetShaderLibrary();
		shaderLibrary->SetCompilerOptions({
			.ForceCompile = false,
			.Optimize = true,
			.GenerateDebugInfo = true
		});

		// 3D
		shaderLibrary->Load("Resources/Shaders/SharkPBR.hlsl");
		shaderLibrary->Load("Resources/Shaders/Skybox.hlsl");
		shaderLibrary->Load("Resources/Shaders/BRDF_LUT.hlsl");
		shaderLibrary->Load("Resources/Shaders/Tonemap.hlsl");
		shaderLibrary->Load("Resources/Shaders/Composite.hlsl");

		// 2D
		shaderLibrary->Load("Resources/Shaders/2D/Quad.hlsl");
		shaderLibrary->Load("Resources/Shaders/2D/Circle.hlsl");
		shaderLibrary->Load("Resources/Shaders/2D/Line.hlsl");
		shaderLibrary->Load("Resources/Shaders/2D/Text.hlsl");

		// Jump Flood
		shaderLibrary->Load("Resources/Shaders/JumpFlood/SelectedGeometry.hlsl");
		shaderLibrary->Load("Resources/Shaders/JumpFlood/JumpFloodInit.hlsl");
		shaderLibrary->Load("Resources/Shaders/JumpFlood/JumpFloodPass.hlsl");
		shaderLibrary->Load("Resources/Shaders/JumpFlood/JumpFloodComposite.hlsl");

		// EnvMap
		shaderLibrary->Load("Resources/Shaders/EnvMap/EquirectangularToCubeMap.hlsl");
		shaderLibrary->Load("Resources/Shaders/EnvMap/EnvIrradiance.hlsl");
		shaderLibrary->Load("Resources/Shaders/EnvMap/EnvMipFilter.hlsl");

		// Commands
		shaderLibrary->Load("resources/Shaders/Commands/CmdBlitImage.hlsl");
		shaderLibrary->Load("resources/Shaders/Commands/CmdBlitImageArray.hlsl");
		shaderLibrary->Load("Resources/Shaders/Commands/LinearSample.hlsl");
		shaderLibrary->Load("Resources/Shaders/Commands/LinearSampleArray.hlsl");

		shaderLibrary->Load("Resources/Shaders/ImGui.hlsl");
		s_Data->m_ShaderCache.SaveRegistry();

		SK_CORE_INFO_TAG("Renderer", "Finished loading shaders in {}", loadShadersTimer.Elapsed());

		RT::SetupCache(GetGraphicsDevice(), &s_Data->m_ResourceCache);

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

			s_Data->m_QuadVertexBuffer = VertexBuffer::Create(Buffer::FromArray(vertices));
			s_Data->m_QuadIndexBuffer = IndexBuffer::Create(Buffer::FromArray(indices));
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

			s_Data->m_CubeVertexBuffer = VertexBuffer::Create(Buffer::FromArray(vertices));
			s_Data->m_CubeIndexBuffer = IndexBuffer::Create(Buffer::FromArray(indices));
		}

		s_Data->m_BRDFLUTTexture = CreateBRDFLUT();

		{
			TextureSpecification spec;
			spec.Format = ImageFormat::RGBA;
			spec.Width = 1;
			spec.Height = 1;
			spec.HasMips = false;

			spec.DebugName = "White Texture";
			s_Data->m_WhiteTexture = Texture2D::Create(spec, Buffer::FromValue(0xFFFFFFFF));

			spec.DebugName = "Black Texture";
			s_Data->m_BlackTexture = Texture2D::Create(spec, Buffer::FromValue(0x00000000));

			spec.Format = ImageFormat::RGBA32F;
			spec.DebugName = "Black Texture Cube";
			spec.Format = ImageFormat::RGBA32F;
			glm::vec4 imageData[6]{};

			s_Data->m_BlackTextureCube = TextureCube::Create(spec, Buffer::FromArray(imageData));
			s_Data->m_EmptyEnvironment = Ref<Environment>::Create(s_Data->m_BlackTextureCube, s_Data->m_BlackTextureCube);
		}

		ImageSpecification nullUAVSpec;
		nullUAVSpec.Width = 1;
		nullUAVSpec.Height = 1;
		nullUAVSpec.Format = ImageFormat::RGBA;
		nullUAVSpec.Usage = ImageUsage::Storage;
		nullUAVSpec.DebugName = "NULL-UAV";
		for (uint32_t i = 0; i < 3; i++)
		{
			nullUAVSpec.Layers = 1;
			s_Data->m_NullUAVs[i] = Image2D::Create(nullUAVSpec);
			nullUAVSpec.Layers = 2;
			s_Data->m_NullArrayUAVs[i] = Image2D::Create(nullUAVSpec);
		}

		Renderer::WaitAndRender();
	}

	void Renderer::ShutDown()
	{
		SK_PROFILE_FUNCTION();

		// execute any remaining commands (usually there are none)
		Renderer::WaitAndRender();

		s_Data->m_ShaderCache.SaveRegistry();

		s_Data->m_QuadVertexBuffer = nullptr;
		s_Data->m_QuadIndexBuffer = nullptr;
		s_Data->m_CubeVertexBuffer = nullptr;
		s_Data->m_CubeIndexBuffer = nullptr;

		s_Data->m_ShaderLibrary = nullptr;
		s_Data->m_WhiteTexture = nullptr;
		skdelete s_Data;

		skdelete s_CommandQueue[0];
		skdelete s_CommandQueue[1];

		skdelete s_MTCommandQueue[0];
		skdelete s_MTCommandQueue[1];
	}

	DeviceManager* Renderer::GetDeviceManager()
	{
		return Application::Get().GetDeviceManager();
	}

	nvrhi::IDevice* Renderer::GetGraphicsDevice()
	{
		return GetDeviceManager()->GetDevice();
	}

	void Renderer::BeginFrame()
	{
		s_Data->m_FrameIndex++;
		Renderer::Submit([]()
		{
			s_Data->m_RTFrameIndex++;

			auto device = GetGraphicsDevice();
			device->runGarbageCollection();
		});
	}

	void Renderer::EndFrame()
	{
	}

	void Renderer::WaitAndRender()
	{
		SK_PROFILE_FUNCTION();
		SK_PERF_SCOPED("Renderer::WaitAndRender");

		s_MTCommandQueueMutex.lock();
		std::swap(s_MTCommandQueue[0], s_MTCommandQueue[1]);
		s_MTCommandQueueMutex.unlock();
		s_MTCommandQueue[1]->Execute();

		std::swap(s_CommandQueue[0], s_CommandQueue[1]);

		s_SingleThreadedIsExecuting = true;
		s_CommandQueue[1]->Execute();
		s_SingleThreadedIsExecuting = false;

	}

	namespace Internal {
		static void GenerateMips(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> targetImage);
		static void RT_GenerateMips(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> targetImage);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////
	//// Main Thread
	///////////////////////////////////////////////////////////////////////////////////////////////////

	#pragma region Main Thread

	void Renderer::BeginRenderPass(Ref<RenderCommandBuffer> commandBuffer, Ref<RenderPass> renderPass, bool expliciteClear)
	{
		SK_CORE_TRACE_TAG("Renderer", "BeginRenderPass '{}'", renderPass->GetSpecification().DebugName);

		auto shader = renderPass->GetShader();
		auto framebuffer = renderPass->GetTargetFramebuffer();
		Submit([commandBuffer, renderPass, framebuffer, shader, expliciteClear]()
		{
			RT::BeginRenderPass(commandBuffer, renderPass, framebuffer, shader, expliciteClear);
		});
	}

	void Renderer::EndRenderPass(Ref<RenderCommandBuffer> commandBuffer, Ref<RenderPass> renderPass)
	{
		SK_CORE_TRACE_TAG("Renderer", "EndRenderPass '{}'", renderPass->GetSpecification().DebugName);

		Submit([commandBuffer, renderPass]()
		{
			RT::EndRenderPass(commandBuffer, renderPass);
		});
	}

	void Renderer::BeginComputePass(Ref<RenderCommandBuffer> commandBuffer, Ref<ComputePass> computePass)
	{
		SK_CORE_TRACE_TAG("Renderer", "BeginComputePass '{}'", computePass->GetSpecification().DebugName);

		Submit([commandBuffer, computePass, shader = computePass->GetShader()]()
		{
			RT::BeginComputePass(commandBuffer, computePass, shader);
		});
	}

	void Renderer::EndComputePass(Ref<RenderCommandBuffer> commandBuffer, Ref<ComputePass> computePass)
	{
		SK_CORE_TRACE_TAG("Renderer", "EndComputePass '{}'", computePass->GetSpecification().DebugName);

		Submit([commandBuffer, computePass]()
		{
			RT::EndComputePass(commandBuffer, computePass);
		});
	}

	void Renderer::Dispatch(Ref<RenderCommandBuffer> commandBuffer, Ref<ComputePipeline> pipeline, const glm::uvec3& workGroups, const Buffer pushConstantData)
	{
		SK_CORE_TRACE_TAG("Renderer", "Dispatch '{}' {}", pipeline->GetDebugName(), workGroups);

		Submit([commandBuffer, pipeline, workGroups, temp = Buffer::Copy(pushConstantData)]() mutable
		{
			RT::Dispatch(commandBuffer, pipeline, nullptr, workGroups, temp);
			temp.Release();
		});
	}

	void Renderer::Dispatch(Ref<RenderCommandBuffer> commandBuffer, Ref<ComputePipeline> pipeline, Ref<Material> material, const glm::uvec3& workGroups, const Buffer pushConstantData)
	{
		SK_CORE_TRACE_TAG("Renderer", "Dispatch '{}' '{}' {}", pipeline->GetDebugName(), material->GetName(), workGroups);
		SK_CORE_VERIFY(material);

		Submit([commandBuffer, pipeline, material, workGroups, temp = Buffer::Copy(pushConstantData)]() mutable
		{
			RT::Dispatch(commandBuffer, pipeline, material, workGroups, temp);
			temp.Release();
		});
	}

	void Renderer::RenderGeometry(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, uint32_t indexCount, Buffer pushConstant)
	{
		SK_CORE_TRACE_TAG("Renderer", "[RT] RenderGeometry '{}' '{}'", material->GetName(), pipeline->GetSpecification().DebugName);

		Submit([commandBuffer, pipeline, material, vertexBuffer, indexBuffer, indexCount, temp = Buffer::Copy(pushConstant)]() mutable
		{
			RT::RenderGeometry(commandBuffer, pipeline, material, vertexBuffer, indexBuffer, nvrhi::DrawArguments().setVertexCount(indexCount), temp);
			temp.Release();
		});
	}

	void Renderer::RenderGeometry(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, const nvrhi::DrawArguments& drawArguments, Buffer pushConstant)
	{
		SK_CORE_TRACE_TAG("Renderer", "[RT] RenderGeometry '{}' '{}'", material->GetName(), pipeline->GetSpecification().DebugName);

		Submit([commandBuffer, pipeline, material, vertexBuffer, indexBuffer, drawArguments, temp = Buffer::Copy(pushConstant)]() mutable
		{
			RT::RenderGeometry(commandBuffer, pipeline, material, vertexBuffer, indexBuffer, drawArguments, temp);
			temp.Release();
		});
	}

	void Renderer::RenderSubmesh(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Mesh> mesh, Ref<MeshSource> meshSource, uint32_t submeshIndex, Ref<Material> material, const Buffer pushConstantsData)
	{
		SK_CORE_TRACE_TAG("Renderer", "[RT] RenderSubmesh '{}':{} '{}'", meshSource->GetName(), submeshIndex, material->GetName());

		Submit([commandBuffer, pipeline, mesh, meshSource, submeshIndex, material, temp = Buffer::Copy(pushConstantsData)]() mutable
		{
			RT::RenderSubmesh(commandBuffer, pipeline, mesh, meshSource, submeshIndex, material, temp);
			temp.Release();
		});
	}

	void Renderer::RenderFullScreenQuad(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, Buffer pushConstantsData)
	{
		RenderGeometry(commandBuffer, pipeline, material, s_Data->m_QuadVertexBuffer, s_Data->m_QuadIndexBuffer, s_Data->m_QuadIndexBuffer->GetCount(), pushConstantsData);
	}

	void Renderer::RenderCube(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Material> material)
	{
		RenderGeometry(commandBuffer, pipeline, material, s_Data->m_CubeVertexBuffer, s_Data->m_CubeIndexBuffer, s_Data->m_CubeIndexBuffer->GetCount());
	}

	void Renderer::WriteBuffer(Ref<RenderCommandBuffer> commandBuffer, Ref<GpuBuffer> buffer, const Buffer bufferData)
	{
		Submit([commandBuffer, buffer, temp = Buffer::Copy(bufferData)]() mutable
		{
			RT::WriteBuffer(commandBuffer, buffer, temp);
			temp.Release();
		});
	}

	void Renderer::WriteImage(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> image, const ImageSlice& slice, const Buffer imageData)
	{
		Submit([commandBuffer, image, slice, temp = Buffer::Copy(imageData)]() mutable
		{
			RT::WriteImage(commandBuffer, image, slice, temp);
			temp.Release();
		});
	}

	void Renderer::CopySlice(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> sourceImage, const ImageSlice& sourceSlice, Ref<Image2D> destinationImage, const ImageSlice& destinationSlice)
	{
		SK_CORE_TRACE_TAG("Renderer", "CopySlice '{}':(Mip:{}, Level: {}) -> '{}':(Mip:{}, Level: {})", sourceImage->GetSpecification().DebugName, sourceSlice.Mip, sourceSlice.Layer, destinationImage->GetSpecification().DebugName, destinationSlice.Mip, destinationSlice.Layer);

		Submit([commandBuffer, sourceImage, sourceSlice, destinationImage, destinationSlice]()
		{
			RT::CopySlice(commandBuffer, sourceImage, sourceSlice, destinationImage, destinationSlice);
		});
	}

	void Renderer::CopySlice(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> sourceImage, const ImageSlice& sourceSlice, Ref<StagingImage2D> destinationImage, const ImageSlice& destinationSlice)
	{
		SK_CORE_TRACE_TAG("Renderer", "CopySlice '{}':(Mip:{}, Level: {}) -> '{}':(Mip:{}, Level: {})", sourceImage->GetSpecification().DebugName, sourceSlice.Mip, sourceSlice.Layer, destinationImage->GetSpecification().DebugName, destinationSlice.Mip, destinationSlice.Layer);

		Submit([commandBuffer, sourceImage, sourceSlice, destinationImage, destinationSlice]()
		{
			RT::CopySlice(commandBuffer, sourceImage, sourceSlice, destinationImage, destinationSlice);
		});
	}

	void Renderer::CopySlice(Ref<RenderCommandBuffer> commandBuffer, Ref<StagingImage2D> sourceImage, const ImageSlice& sourceSlice, Ref<Image2D> destinationImage, const ImageSlice& destinationSlice)
	{
		SK_CORE_TRACE_TAG("Renderer", "CopySlice '{}':(Mip:{}, Level: {}) -> '{}':(Mip:{}, Level: {})", sourceImage->GetSpecification().DebugName, sourceSlice.Mip, sourceSlice.Layer, destinationImage->GetSpecification().DebugName, destinationSlice.Mip, destinationSlice.Layer);

		Submit([commandBuffer, sourceImage, sourceSlice, destinationImage, destinationSlice]()
		{
			RT::CopySlice(commandBuffer, sourceImage, sourceSlice, destinationImage, destinationSlice);
		});
	}

	void Renderer::CopyMip(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> sourceImage, uint32_t sourceMip, Ref<Image2D> destinationImage, uint32_t destinationMip)
	{
		SK_CORE_TRACE_TAG("Renderer", "CopyMip '{}':{} -> '{}':{}", sourceImage->GetSpecification().DebugName, sourceMip, destinationImage->GetSpecification().DebugName, destinationMip);

		Submit([commandBuffer, sourceImage, sourceMip, destinationImage, destinationMip]()
		{
			RT::CopyMip(commandBuffer, sourceImage, sourceMip, destinationImage, destinationMip);
		});
	}

	void Renderer::CopyMip(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> sourceImage, uint32_t sourceMip, Ref<StagingImage2D> destinationImage, uint32_t destinationMip)
	{
		SK_CORE_TRACE_TAG("Renderer", "CopyMip '{}':{} -> '{}':{}", sourceImage->GetSpecification().DebugName, sourceMip, destinationImage->GetSpecification().DebugName, destinationMip);

		Submit([commandBuffer, sourceImage, sourceMip, destinationImage, destinationMip]()
		{
			RT::CopyMip(commandBuffer, sourceImage, sourceMip, destinationImage, destinationMip);
		});
	}

	void Renderer::CopyMip(Ref<RenderCommandBuffer> commandBuffer, Ref<StagingImage2D> sourceImage, uint32_t sourceMip, Ref<Image2D> destinationImage, uint32_t destinationMip)
	{
		SK_CORE_TRACE_TAG("Renderer", "CopyMip '{}':{} -> '{}':{}", sourceImage->GetSpecification().DebugName, sourceMip, destinationImage->GetSpecification().DebugName, destinationMip);

		Submit([commandBuffer, sourceImage, sourceMip, destinationImage, destinationMip]()
		{
			RT::CopyMip(commandBuffer, sourceImage, sourceMip, destinationImage, destinationMip);
		});
	}

	void Renderer::CopyImage(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> sourceImage, Ref<Image2D> destinationImage)
	{
		SK_CORE_TRACE_TAG("Renderer", "CopyImage '{}' -> '{}'", sourceImage->GetSpecification().DebugName, destinationImage->GetSpecification().DebugName);

		Submit([commandBuffer, sourceImage, destinationImage]()
		{
			RT::CopyImage(commandBuffer, sourceImage, destinationImage);
		});
	}

	void Renderer::CopyImage(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> sourceImage, Ref<StagingImage2D> destinationImage)
	{
		SK_CORE_TRACE_TAG("Renderer", "CopyImage '{}' -> '{}'", sourceImage->GetSpecification().DebugName, destinationImage->GetSpecification().DebugName);

		Submit([commandBuffer, sourceImage, destinationImage]()
		{
			RT::CopyImage(commandBuffer, sourceImage, destinationImage);
		});
	}

	void Renderer::CopyImage(Ref<RenderCommandBuffer> commandBuffer, Ref<StagingImage2D> sourceImage, Ref<Image2D> destinationImage)
	{
		SK_CORE_TRACE_TAG("Renderer", "CopyImage '{}' -> '{}'", sourceImage->GetSpecification().DebugName, destinationImage->GetSpecification().DebugName);

		Submit([commandBuffer, sourceImage, destinationImage]()
		{
			RT::CopyImage(commandBuffer, sourceImage, destinationImage);
		});
	}

	void Renderer::BlitImage(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> sourceImage, Ref<Image2D> destinationImage, uint32_t mipSlice, FilterMode filterMode)
	{
		BlitImageParams params;
		params.DestinationBaseSlice.Mip = mipSlice;
		params.DestinationBaseSlice.Layer = 0;
		params.LayerCount = destinationImage->GetSpecification().Layers;
		params.SourceBaseSlice.Mip = mipSlice;
		params.SourceBaseSlice.Layer = 0;
		params.SourceMin = glm::uvec2(0);
		params.SourceMax = glm::uvec2(sourceImage->GetWidth(), sourceImage->GetHeight());
		params.DestinationMin = glm::uvec2(0);
		params.DestinationMax = glm::uvec2(destinationImage->GetWidth(), destinationImage->GetHeight());
		BlitImage(commandBuffer, sourceImage, destinationImage, params, filterMode);
	}

	void Renderer::BlitImage(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> sourceImage, Ref<Image2D> destinationImage, const BlitImageParams& params, FilterMode filterMode)
	{
		SK_PROFILE_SCOPED("Renderer - BlitImage");
		SK_CORE_TRACE_TAG("Renderer", "[RT] BlitImage '{}' -> '{}'", sourceImage->GetSpecification().DebugName, destinationImage->GetSpecification().DebugName);

		SK_CORE_VERIFY(destinationImage->GetSpecification().Usage == ImageUsage::Storage);
		auto shader = Renderer::GetShaderLibrary()->Get(params.LayerCount == 1 ? "CmdBlitImage" : "CmdBlitImageArray");

		auto pipeline = ComputePipeline::Create(shader, "Cmd - Blit");
		auto pass = ComputePass::Create(shader, "Cmd - Blit");

		pass->SetInput("u_Input", sourceImage, nvrhi::TextureSubresourceSet(params.SourceBaseSlice.Mip, 1, params.SourceBaseSlice.Layer, params.LayerCount));
		pass->SetInput("o_Output", destinationImage, nvrhi::TextureSubresourceSet(params.DestinationBaseSlice.Mip, 1, params.DestinationBaseSlice.Layer, params.LayerCount));
		pass->SetInput("u_Sampler", filterMode == FilterMode::Linear ? s_Data->m_Samplers.LinearClamp : s_Data->m_Samplers.NearestClamp);
		SK_CORE_VERIFY(pass->Validate());
		pass->Bake(); // #Renderer #RT

		const auto& srcDesc = sourceImage->GetHandle()->getDesc();
		const auto& dstDesc = destinationImage->GetHandle()->getDesc();

		struct BlitParams
		{
			glm::vec2 src0;
			glm::vec2 src1;
			glm::vec2 dst0;
			glm::vec2 dst1;
		} push;

		push.src0 = params.SourceMin.value_or(glm::uvec2(0, 0));
		push.src1 = params.SourceMax.value_or(glm::uvec2(srcDesc.width, srcDesc.height));
		push.dst0 = params.DestinationMin.value_or(glm::uvec2(0, 0));
		push.dst1 = params.DestinationMax.value_or(glm::uvec2(dstDesc.width, dstDesc.height));

		glm::uvec2 destinationSize = push.dst1 - push.dst0;

		const glm::uvec3 workGroups = {
			(destinationSize.x + 7) / 8,
			(destinationSize.y + 7) / 8,
			params.LayerCount
		};

		BeginComputePass(commandBuffer, pass);
		Dispatch(commandBuffer, pipeline, workGroups, { &push, sizeof(push) });
		EndComputePass(commandBuffer, pass);
	}

	void Internal::GenerateMips(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> targetImage)
	{
		SK_CORE_VERIFY(!ImageUtils::IsIntegerBased(targetImage->GetSpecification().Format));
		SK_CORE_VERIFY(targetImage->GetSpecification().Usage == ImageUsage::Storage);

		SK_PROFILE_SCOPED("Renderer - GenerateMips");
		SK_CORE_TRACE_TAG("Renderer", "GenerateMips '{}':({}, {}):{}", targetImage->GetSpecification().DebugName, targetImage->GetWidth(), targetImage->GetHeight(), targetImage->GetSpecification().MipLevels);

		auto shader = targetImage->GetSpecification().Layers == 1 ? Renderer::GetShaderLibrary()->Get("LinearSample") : Renderer::GetShaderLibrary()->Get("LinearSampleArray");

		auto pipeline = ComputePipeline::Create(shader, "GenerateMips");
		auto pass = ComputePass::Create(shader, "GenerateMips");

		Renderer::BeginComputePass(commandBuffer, pass);

		const uint32_t layers = targetImage->GetSpecification().Layers;
		const uint32_t mipLevels = targetImage->GetSpecification().MipLevels;
		const nvrhi::TextureDimension textureDimension = layers == 1 ? nvrhi::TextureDimension::Texture2D : nvrhi::TextureDimension::Texture2DArray;

		float targetMipWidth = (float)targetImage->GetWidth();
		float targetMipHeight = (float)targetImage->GetHeight();

		auto dstSubresource = nvrhi::TextureSubresourceSet(0, 1, 0, 1);
		auto srcSubresource = nvrhi::TextureSubresourceSet(0, 1, 0, 1);

		for (uint32_t layer = 0; layer < layers; layer++)
		{
			static constexpr uint32_t NUM_LODS = 4;

			struct Settings
			{
				uint32_t Dispatch;
				uint32_t LODs;
			} settings;

			dstSubresource.baseArraySlice = layer;
			srcSubresource.baseArraySlice = layer;

			for (uint32_t baseMip = 1, dispatch = 0; baseMip < mipLevels; baseMip += NUM_LODS, dispatch++)
			{
				targetMipWidth /= 2;
				targetMipHeight /= 2;

				settings.Dispatch = dispatch;
				settings.LODs = std::min(mipLevels - baseMip, NUM_LODS);

				auto material = Material::Create(pipeline->GetShader(), fmt::format("Mip {}-{} Material", baseMip, baseMip + NUM_LODS - 1));

				srcSubresource.baseMipLevel = baseMip - 1;
				material->SetInput("u_Source", targetImage, { .Dimension = textureDimension, .SubresourceSet = srcSubresource });

				for (uint32_t i = 0; i < NUM_LODS; i++)
				{
					if ((baseMip + i) < (mipLevels))
					{
						dstSubresource.baseMipLevel = baseMip + i;
						material->SetInput("o_Mips", targetImage, { .Dimension = textureDimension, .SubresourceSet = dstSubresource }, i);
					}
					else
					{
						material->Set("o_Mips", targetImage->GetSpecification().Layers == 1 ? s_Data->m_NullUAVs[i - 1] : s_Data->m_NullArrayUAVs[i - 1], i);
					}
				}

				material->Bake();

				const auto workGroups = glm::max({ targetMipWidth / 8, targetMipHeight / 8, 1 }, glm::uvec3(1));
				Renderer::Dispatch(commandBuffer, pipeline, material, workGroups, Buffer::FromValue(settings));
			}
		}

		Renderer::EndComputePass(commandBuffer, pass);
	}

	void Renderer::GenerateMips(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> targetImage)
	{
		SK_PROFILE_SCOPED("Renderer - GenerateMips");
		SK_CORE_VERIFY(!ImageUtils::IsIntegerBased(targetImage->GetSpecification().Format));
		SK_CORE_TRACE_TAG("Renderer", "GenerateMips '{}':({}, {}):{}", targetImage->GetSpecification().DebugName, targetImage->GetWidth(), targetImage->GetHeight(), targetImage->GetSpecification().MipLevels);

		if (targetImage->GetSpecification().Usage == ImageUsage::Storage)
		{
			Internal::GenerateMips(commandBuffer, targetImage);
			return;
		}

		ImageSpecification specification = targetImage->GetSpecification();
		specification.Usage = ImageUsage::Storage;
		specification.Format = ImageUtils::ConvertToWritableFormat(specification.Format);
		specification.DebugName = fmt::format("TEMP - GenerateMips - '{}'", targetImage->GetSpecification().DebugName);
		Ref<Image2D> newTarget = Image2D::Create(specification);

		Renderer::CopyMip(commandBuffer, targetImage, 0, newTarget, 0);
		Internal::GenerateMips(commandBuffer, newTarget);

		for (uint32_t i = 1; i < targetImage->GetSpecification().MipLevels; i++)
		{
			CopyMip(commandBuffer, newTarget, i, targetImage, i);
		}
	}

	void Renderer::GenerateMips(Ref<Image2D> image)
	{
		auto commandBuffer = RenderCommandBuffer::Create(fmt::format("GenerateMips '{}'", image->GetSpecification().DebugName));
		commandBuffer->Begin();
		GenerateMips(commandBuffer, image);
		commandBuffer->End();
		commandBuffer->Execute();
	}

	std::pair<Ref<TextureCube>, Ref<TextureCube>> Renderer::CreateEnvironmentMap(Ref<RenderCommandBuffer> commandBuffer, const std::filesystem::path& filepath)
	{
		SK_PROFILE_SCOPED("Renderer - CreateEnvironmentMap");

		const uint32_t cubemapSize = Renderer::GetConfig().EnvironmentMapResolution;
		const uint32_t irradianceMapSize = 32;

		Ref<Texture2D> equirectangular = Texture2D::Create({ .HasMips = false }, filepath);
		SK_CORE_VERIFY(equirectangular->GetSpecification().Format == ImageFormat::RGBA32F, "Environment Texture is not HDR!");

		const uint32_t mipCount = ImageUtils::CalcMipLevels(cubemapSize, cubemapSize);

		TextureSpecification cubemapSpec;
		cubemapSpec.Format = ImageFormat::RGBA32F;
		cubemapSpec.Width = cubemapSize;
		cubemapSpec.Height = cubemapSize;
		cubemapSpec.HasMips = true;
		cubemapSpec.Storage = true;

		cubemapSpec.DebugName = fmt::format("EnvironmentMap Unfiltered {}", filepath);
		auto unfiltered = TextureCube::Create(cubemapSpec);
		cubemapSpec.DebugName = fmt::format("EnvironmentMap Filtered {}", filepath);
		auto filtered = TextureCube::Create(cubemapSpec);

		cubemapSpec.Width = irradianceMapSize;
		cubemapSpec.Height = irradianceMapSize;
		cubemapSpec.DebugName = fmt::format("IrradianceMap {}", filepath);
		auto irradianceMap = TextureCube::Create(cubemapSpec);

		auto equirectToCubeShader = GetShaderLibrary()->Get("EquirectangularToCubeMap");
		auto mipFilterShader = GetShaderLibrary()->Get("EnvMipFilter");
		auto irradianceShader = GetShaderLibrary()->Get("EnvIrradiance");

		{
			auto pipeline = ComputePipeline::Create(equirectToCubeShader);
			auto pass = ComputePass::Create(equirectToCubeShader, "EquirectangularToCube");
			pass->SetInput("u_Equirect", equirectangular);
			pass->SetInput("o_CubeMap", unfiltered);
			SK_CORE_VERIFY(pass->Validate());
			pass->Bake();

			BeginComputePass(commandBuffer, pass);
			Dispatch(commandBuffer, pipeline, { cubemapSize / 32, cubemapSize / 32, 6 });
			EndComputePass(commandBuffer, pass);

			GenerateMips(commandBuffer, unfiltered->GetImage());
		}

		{
			auto pipeline = ComputePipeline::Create(mipFilterShader);
			auto pass = ComputePass::Create(mipFilterShader, "EnvMipFilter");
			pass->SetInput("u_Unfiltered", unfiltered);
			SK_CORE_VERIFY(pass->Validate());
			pass->Bake();

			auto subresource = nvrhi::TextureSubresourceSet(0, 1, 0, nvrhi::TextureSubresourceSet::AllArraySlices);
			const float deltaRoughness = 1.0f / glm::max((float)filtered->GetMipLevelCount() - 1.0f, 1.0f);

			BeginComputePass(commandBuffer, pass);
			for (uint32_t i = 0, size = cubemapSize; i < mipCount; i++, size /= 2)
			{
				const uint32_t workGroups = glm::max(1u, size / 32);
				const float roughness = glm::max(i * deltaRoughness, 0.05f);

				auto material = Material::Create(mipFilterShader, fmt::format("EnvMipFilter Mip {}", i));
				material->SetInput("o_Filtered", filtered, { .SubresourceSet = subresource.setBaseMipLevel(i) });

				SK_CORE_VERIFY(material->Validate());
				material->Bake();

				Dispatch(commandBuffer, pipeline, material, { workGroups, workGroups, 6 }, Buffer::FromValue(roughness));
			}
			EndComputePass(commandBuffer, pass);
		}

		{
			auto pipeline = ComputePipeline::Create(irradianceShader);
			auto pass = ComputePass::Create(irradianceShader, "EnvIrradiance");
			pass->SetInput("o_Irradiance", irradianceMap);
			pass->SetInput("u_Radiance", filtered);
			SK_CORE_VERIFY(pass->Validate());
			pass->Bake();

			BeginComputePass(commandBuffer, pass);
			Dispatch(commandBuffer, pipeline, { irradianceMap->GetWidth() / 32, irradianceMap->GetHeight() / 32, 6 }, Buffer::FromValue(GetConfig().IrradianceMapComputeSamples));
			EndComputePass(commandBuffer, pass);


			GenerateMips(commandBuffer, irradianceMap->GetImage());
		}

		return { filtered, irradianceMap };
	}

	std::pair<Ref<TextureCube>, Ref<TextureCube>> Renderer::CreateEnvironmentMap(const std::filesystem::path& filepath)
	{
		auto commandBuffer = RenderCommandBuffer::Create("Environment");
		commandBuffer->Begin();
		auto result = CreateEnvironmentMap(commandBuffer, filepath);
		commandBuffer->End();
		commandBuffer->Execute();
		return result;
	}

	#pragma endregion

	///////////////////////////////////////////////////////////////////////////////////////////////////
	//// Render Thread
	///////////////////////////////////////////////////////////////////////////////////////////////////

	#pragma region Render Thread

	void Renderer::RT_BeginRenderPass(Ref<RenderCommandBuffer> commandBuffer, Ref<RenderPass> renderPass, bool expliciteClear)
	{
		RT::BeginRenderPass(commandBuffer, renderPass, renderPass->GetTargetFramebuffer(), renderPass->GetShader(), expliciteClear);
	}

	void Renderer::RT_EndRenderPass(Ref<RenderCommandBuffer> commandBuffer, Ref<RenderPass> renderPass)
	{
		RT::EndRenderPass(commandBuffer, renderPass);
	}

	void Renderer::RT_BeginComputePass(Ref<RenderCommandBuffer> commandBuffer, Ref<ComputePass> computePass)
	{
		RT::BeginComputePass(commandBuffer, computePass, computePass->GetShader());
	}

	void Renderer::RT_EndComputePass(Ref<RenderCommandBuffer> commandBuffer, Ref<ComputePass> computePass)
	{
		RT::EndComputePass(commandBuffer, computePass);
	}

	void Renderer::RT_Dispatch(Ref<RenderCommandBuffer> commandBuffer, Ref<ComputePipeline> pipeline, const glm::uvec3& workGroups, const Buffer pushConstantData)
	{
		RT::Dispatch(commandBuffer, pipeline, nullptr, workGroups, pushConstantData);
	}

	void Renderer::RT_Dispatch(Ref<RenderCommandBuffer> commandBuffer, Ref<ComputePipeline> pipeline, Ref<Material> material, const glm::uvec3& workGroups, const Buffer pushConstantData)
	{
		SK_CORE_VERIFY(material);
		RT::Dispatch(commandBuffer, pipeline, material, workGroups, pushConstantData);
	}

	void Renderer::RT_RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, uint32_t indexCount, Buffer pushConstant)
	{
		RT::RenderGeometry(renderCommandBuffer, pipeline, material, vertexBuffer, indexBuffer, nvrhi::DrawArguments().setVertexCount(indexCount), pushConstant);
	}

	void Renderer::RT_RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, const nvrhi::DrawArguments& drawArguments, Buffer pushConstant)
	{
		RT::RenderGeometry(renderCommandBuffer, pipeline, material, vertexBuffer, indexBuffer, drawArguments, pushConstant);
	}

	void Renderer::RT_RenderSubmesh(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Mesh> mesh, Ref<MeshSource> meshSource, uint32_t submeshIndex, Ref<Material> material, const Buffer pushConstantsData)
	{
		RT::RenderSubmesh(commandBuffer, pipeline, mesh, meshSource, submeshIndex, material, pushConstantsData);
	}

	void Renderer::RT_RenderFullScreenQuad(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, Buffer pushConstantsData)
	{
		RT_RenderGeometry(commandBuffer, pipeline, material, s_Data->m_QuadVertexBuffer, s_Data->m_QuadIndexBuffer, s_Data->m_QuadIndexBuffer->GetCount(), pushConstantsData);
	}

	void Renderer::RT_RenderCube(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Material> material)
	{
		RT_RenderGeometry(commandBuffer, pipeline, material, s_Data->m_CubeVertexBuffer, s_Data->m_CubeIndexBuffer, s_Data->m_CubeIndexBuffer->GetCount());
	}

	void Renderer::RT_WriteBuffer(Ref<RenderCommandBuffer> commandBuffer, Ref<GpuBuffer> buffer, const Buffer bufferData)
	{
		RT::WriteBuffer(commandBuffer, buffer, bufferData);
	}

	void Renderer::RT_WriteImage(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> image, const ImageSlice& slice, const Buffer imageData)
	{
		RT::WriteImage(commandBuffer, image, slice, imageData);
	}

	void Renderer::RT_CopySlice(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> sourceImage, const ImageSlice& sourceSlice, Ref<Image2D> destinationImage, const ImageSlice& destinationSlice)
	{
		RT::CopySlice(commandBuffer, sourceImage, sourceSlice, destinationImage, destinationSlice);
	}

	void Renderer::RT_CopySlice(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> sourceImage, const ImageSlice& sourceSlice, Ref<StagingImage2D> destinationImage, const ImageSlice& destinationSlice)
	{
		RT::CopySlice(commandBuffer, sourceImage, sourceSlice, destinationImage, destinationSlice);
	}

	void Renderer::RT_CopySlice(Ref<RenderCommandBuffer> commandBuffer, Ref<StagingImage2D> sourceImage, const ImageSlice& sourceSlice, Ref<Image2D> destinationImage, const ImageSlice& destinationSlice)
	{
		RT::CopySlice(commandBuffer, sourceImage, sourceSlice, destinationImage, destinationSlice);
	}

	void Renderer::RT_CopyMip(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> sourceImage, uint32_t sourceMip, Ref<Image2D> destinationImage, uint32_t destinationMip)
	{
		RT::CopyMip(commandBuffer, sourceImage, sourceMip, destinationImage, destinationMip);
	}

	void Renderer::RT_CopyMip(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> sourceImage, uint32_t sourceMip, Ref<StagingImage2D> destinationImage, uint32_t destinationMip)
	{
		RT::CopyMip(commandBuffer, sourceImage, sourceMip, destinationImage, destinationMip);
	}

	void Renderer::RT_CopyMip(Ref<RenderCommandBuffer> commandBuffer, Ref<StagingImage2D> sourceImage, uint32_t sourceMip, Ref<Image2D> destinationImage, uint32_t destinationMip)
	{
		RT::CopyMip(commandBuffer, sourceImage, sourceMip, destinationImage, destinationMip);
	}

	void Renderer::RT_CopyImage(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> sourceImage, Ref<Image2D> destinationImage)
	{
		RT::CopyImage(commandBuffer, sourceImage, destinationImage);
	}

	void Renderer::RT_CopyImage(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> sourceImage, Ref<StagingImage2D> destinationImage)
	{
		RT::CopyImage(commandBuffer, sourceImage, destinationImage);
	}

	void Renderer::RT_CopyImage(Ref<RenderCommandBuffer> commandBuffer, Ref<StagingImage2D> sourceImage, Ref<Image2D> destinationImage)
	{
		RT::CopyImage(commandBuffer, sourceImage, destinationImage);
	}

	void Internal::RT_GenerateMips(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> targetImage)
	{
		SK_CORE_VERIFY(!ImageUtils::IsIntegerBased(targetImage->GetSpecification().Format));
		SK_CORE_VERIFY(targetImage->GetSpecification().Usage == ImageUsage::Storage);

		SK_PROFILE_SCOPED("Renderer - GenerateMips");
		SK_CORE_TRACE_TAG("Renderer", "[RT] GenerateMips '{}':({}, {}):{}", targetImage->GetSpecification().DebugName, targetImage->GetWidth(), targetImage->GetHeight(), targetImage->GetSpecification().MipLevels);

		/////////////////////////////////////////////////
		/// Setupt
		/////////////////////////////////////////////////

		const auto& targetDesc = targetImage->GetHandle()->getDesc();
		const uint32_t layers = targetDesc.arraySize;
		const uint32_t mipLevels = targetDesc.mipLevels;
		const auto textureDimension = layers == 1 ? nvrhi::TextureDimension::Texture2D : nvrhi::TextureDimension::Texture2DArray;

		Ref<Shader> shader;
		nvrhi::ComputePipelineHandle pipeline;
		std::array<nvrhi::TextureHandle, 3> nullUAVs;

		auto* cache = Renderer::GetResourceCache();
		if (layers == 1)
		{
			shader = Renderer::GetShaderLibrary()->Get("LinearSample");
			pipeline = cache->Get<nvrhi::IComputePipeline>("Pipeline-LS");
			nullUAVs = {
				cache->Get<nvrhi::ITexture>("UAV-null-0"),
				cache->Get<nvrhi::ITexture>("UAV-null-1"),
				cache->Get<nvrhi::ITexture>("UAV-null-2")
			};
		}
		else
		{
			shader = Renderer::GetShaderLibrary()->Get("LinearSampleArray");
			pipeline = cache->Get<nvrhi::IComputePipeline>("Pipeline-LSA");
			nullUAVs = {
				cache->Get<nvrhi::ITexture>("UAV-Array-null-0"),
				cache->Get<nvrhi::ITexture>("UAV-Array-null-1"),
				cache->Get<nvrhi::ITexture>("UAV-Array-null-2")
			};
		}

		/////////////////////////////////////////////////
		/// Generate 
		/////////////////////////////////////////////////

		auto commandList = commandBuffer->GetHandle();
		commandList->beginMarker("GenerateMips");

		nvrhi::ComputeState state;
		state.pipeline = pipeline;

		for (uint32_t layer = 0; layer < layers; layer++)
		{
			static constexpr uint32_t NUM_LODS = 4;

			float targetMipWidth = static_cast<float>(targetDesc.width);
			float targetMipHeight = static_cast<float>(targetDesc.height);

			struct Settings
			{
				uint32_t Dispatch;
				uint32_t LODs;
			} settings;

			DescriptorSetManager manager(shader, 0);
			InputKey mipKey = manager.GetInputKey("o_Mips");
			InputKey sourceKey = manager.GetInputKey("u_Source");

			manager.SetInput(sourceKey, targetImage->GetHandle());
			manager.SetInput(mipKey.SetIndex(0), targetImage->GetHandle());
			manager.SetInput(mipKey.SetIndex(1), targetImage->GetHandle());
			manager.SetInput(mipKey.SetIndex(2), targetImage->GetHandle());
			manager.SetInput(mipKey.SetIndex(3), targetImage->GetHandle());

			for (uint32_t baseMip = 1, dispatch = 0; baseMip < mipLevels; baseMip += NUM_LODS, dispatch++)
			{
				targetMipWidth /= 2;
				targetMipHeight /= 2;

				settings.Dispatch = dispatch;
				settings.LODs = std::min(mipLevels - baseMip, NUM_LODS);

				manager.SetDescriptor(
					sourceKey,
					nvrhi::TextureSubresourceSet(baseMip - 1, 1, layer, 1),
					nvrhi::Format::UNKNOWN,
					textureDimension
				);

				for (uint32_t i = 0; i < NUM_LODS; i++)
				{
					mipKey.ArrayIndex = i;

					if ((baseMip + i) < mipLevels)
					{
						manager.SetDescriptor(
							mipKey,
							nvrhi::TextureSubresourceSet(baseMip + i, 1, layer, 1),
							nvrhi::Format::UNKNOWN,
							textureDimension
						);
					}
					else
					{
						manager.SetInput(mipKey, nullUAVs[i - 1]);
						manager.SetDescriptor(
							mipKey,
							nvrhi::TextureSubresourceSet(0, 1, 0, 1),
							nvrhi::Format::UNKNOWN,
							textureDimension
						);
					}
				}

				SK_CORE_VERIFY(manager.Validate());
				manager.Bake();

				state.bindings = {
					manager.GetHandle()
				};

				const auto workGroups = glm::max({ targetMipWidth / 8, targetMipHeight / 8 }, glm::uvec2(1));

				commandList->setComputeState(state);
				commandList->setPushConstants(&settings, sizeof(Settings));
				commandList->dispatch(workGroups.x, workGroups.y, 1);
			}
		}

		commandList->endMarker();

	};


	void Renderer::RT_GenerateMips(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> targetImage)
	{
		SK_PROFILE_SCOPED("Renderer - GenerateMips");
		SK_CORE_TRACE_TAG("Renderer", "[RT] GenerateMips '{}':({}, {}):{}", targetImage->GetSpecification().DebugName, targetImage->GetWidth(), targetImage->GetHeight(), targetImage->GetSpecification().MipLevels);

		const auto& targetDesc = targetImage->GetHandle()->getDesc();
		if (targetDesc.isUAV)
		{
			Internal::RT_GenerateMips(commandBuffer, targetImage);
			return;
		}

		ImageSpecification specification;
		specification.Width = targetDesc.width;
		specification.Height = targetDesc.height;
		specification.Format = ImageUtils::ConvertImageFormat(targetDesc.format);
		specification.MipLevels = targetDesc.mipLevels;
		specification.Layers = targetDesc.arraySize;
		specification.Usage = ImageUsage::Storage;
		Ref<Image2D> newTarget = Image2D::Create(specification);

		Renderer::RT_CopyMip(commandBuffer, targetImage, 0, newTarget, 0);
		Internal::RT_GenerateMips(commandBuffer, newTarget);

		for (uint32_t i = 1; i < targetImage->GetSpecification().MipLevels; i++)
		{
			Renderer::RT_CopyMip(commandBuffer, newTarget, i, targetImage, i);
		}
	}

	void Renderer::RT_GenerateMips(Ref<Image2D> targetImage)
	{
		auto commandBuffer = RenderCommandBuffer::Create(fmt::format("GenerateMips '{}'", targetImage->GetSpecification().DebugName));
		commandBuffer->RT_Begin();
		RT_GenerateMips(commandBuffer, targetImage);
		commandBuffer->RT_End();
		commandBuffer->RT_Execute();
	}

	void Renderer::RT_CreateEnvironmentMap(Ref<RenderCommandBuffer> commandBuffer, Ref<TextureCube> radianceTarget, Ref<TextureCube> irradianceTarget, const std::filesystem::path& filepath)
	{
		SK_CORE_VERIFY(radianceTarget->GetWidth() == radianceTarget->GetHeight());
		SK_CORE_VERIFY(irradianceTarget->GetWidth() == irradianceTarget->GetHeight());
		SK_CORE_VERIFY(radianceTarget->GetSpecification().Storage && irradianceTarget->GetSpecification().Storage);
		SK_CORE_VERIFY(radianceTarget->GetSpecification().Format == ImageFormat::RGBA32F && irradianceTarget->GetSpecification().Format == ImageFormat::RGBA32F);

		SK_PROFILE_SCOPED("Renderer - CreateEnvironmentMap");
		SK_CORE_TRACE_TAG("renderer", "[RT] CreateEnvironmentMap '{}'", filepath);

		/////////////////////////////////////////////////
		/// Setupt
		/////////////////////////////////////////////////

		const uint32_t cubemapSize = radianceTarget->GetWidth();
		const uint32_t irradianceSize = irradianceTarget->GetWidth();
		const uint32_t mipCount = ImageUtils::CalcMipLevels(cubemapSize, cubemapSize);

		const float delta = 1.0f / static_cast<float>(glm::max(mipCount - 1, 1u));
		const uint32_t samples = Renderer::GetConfig().IrradianceMapComputeSamples;

		TextureSpecification textureSpecification = { .HasMips = false };
		Buffer imageData = TextureImporter::ToBufferFromFile(filepath, textureSpecification.Format, textureSpecification.Width, textureSpecification.Height);

		Ref<Texture2D> equirectangular = Texture2D::Create(textureSpecification);
		SK_CORE_VERIFY(equirectangular->GetSpecification().Format == ImageFormat::RGBA32F, "Environment Texture is not HDR!");

		RT_WriteImage(commandBuffer, equirectangular->GetImage(), ImageSlice::Zero(), imageData);
		imageData.Release();

		TextureSpecification unfilteredSpecification = radianceTarget->GetSpecification();
		unfilteredSpecification.DebugName = fmt::format("TEMP - Env Unfiltered '{}'", filepath);
		Ref<TextureCube> unfiltered = TextureCube::Create(unfilteredSpecification);

		auto equirectToCubeShader = Renderer::GetShaderLibrary()->Get("EquirectangularToCubeMap");
		auto mipFilterShader = Renderer::GetShaderLibrary()->Get("EnvMipFilter");
		auto irradianceShader = Renderer::GetShaderLibrary()->Get("EnvIrradiance");

		auto cache = Renderer::GetResourceCache();
		auto device = Renderer::GetGraphicsDevice();
		auto commandlist = commandBuffer->GetHandle();

		/////////////////////////////////////////////////
		/// Passes
		/////////////////////////////////////////////////

		commandlist->beginMarker("ToCube");
		{
			DescriptorSetManager manager(equirectToCubeShader, 0);
			manager.SetInput("u_Equirect", equirectangular->GetHandle(), 0);
			manager.SetInput("o_CubeMap", unfiltered->GetHandle(), 0);
			SK_CORE_VERIFY(manager.Validate());
			manager.Bake();

			nvrhi::ComputeState state;
			state.pipeline = cache->Get<nvrhi::IComputePipeline>("Pipeline-Env-ToCube");
			state.bindings = {
				manager.GetHandle(),
				equirectToCubeShader->GetRequestedBindingSets().at(3)
			};

			commandlist->setComputeState(state);
			commandlist->dispatch(cubemapSize / 32, cubemapSize / 32, 6);
		}
		commandlist->endMarker();

		RT_GenerateMips(commandBuffer, unfiltered->GetImage());

		commandlist->beginMarker("MipFilter");
		{
			DescriptorSetManager set1(mipFilterShader, 1);
			set1.SetInput("u_Unfiltered", unfiltered->GetHandle(), 0);
			SK_CORE_VERIFY(set1.Validate());
			set1.Bake();

			nvrhi::ComputeState state;
			state.pipeline = cache->Get<nvrhi::IComputePipeline>("Pipeline-Env-Filter");
			state.bindings = {
				nullptr,
				set1.GetHandle(),
				mipFilterShader->GetRequestedBindingSets().at(3),
			};

			DescriptorSetManager set0(mipFilterShader, 0);
			set0.SetInput("o_Filtered", radianceTarget->GetHandle(), 0);

			for (uint32_t i = 0, size = cubemapSize; i < mipCount; i++, size /= 2)
			{
				const uint32_t workGroups = glm::max(1u, size / 32);
				const float roughness = glm::max(i * delta, 0.05f);

				set0.SetDescriptor("o_Filtered", nvrhi::TextureSubresourceSet(i, 1, 0, nvrhi::TextureSubresourceSet::AllArraySlices));
				SK_CORE_VERIFY(set0.Validate());
				set0.Bake();

				state.bindings[0] = set0.GetHandle();

				commandlist->setComputeState(state);
				commandlist->setPushConstants(&roughness, sizeof(float));
				commandlist->dispatch(workGroups, workGroups, 6);
			}
		}
		commandlist->endMarker();

		commandlist->beginMarker("Sample");
		{
			DescriptorSetManager manager(irradianceShader, 0);
			manager.SetInput("u_Radiance", radianceTarget->GetHandle(), 0);
			manager.SetInput("o_Irradiance", irradianceTarget->GetHandle(), 0);
			SK_CORE_VERIFY(manager.Validate());
			manager.Bake();

			nvrhi::ComputeState state;
			state.pipeline = cache->Get<nvrhi::IComputePipeline>("Pipeline-Env-Sample");
			state.bindings = {
				manager.GetHandle(),
				irradianceShader->GetRequestedBindingSets().at(3)
			};

			commandlist->setComputeState(state);
			commandlist->setPushConstants(&samples, sizeof samples);
			commandlist->dispatch(irradianceSize / 32, irradianceSize / 32, 6);
		}
		commandlist->endMarker();

		RT_GenerateMips(commandBuffer, irradianceTarget->GetImage());
	}

	std::pair<Ref<TextureCube>, Ref<TextureCube>> Renderer::RT_CreateEnvironmentMap(Ref<RenderCommandBuffer> commandBuffer, const std::filesystem::path& filepath)
	{
		SK_PROFILE_SCOPED("Renderer - CreateEnvironmentMap");
		SK_CORE_TRACE_TAG("renderer", "[RT] CreateEnvironmentMap '{}'", filepath);

		auto device = Renderer::GetGraphicsDevice();

		const uint32_t cubemapSize = Renderer::GetConfig().EnvironmentMapResolution;
		const uint32_t irradianceMapSize = 32;

		Ref<Texture2D> equirectangular = Texture2D::Create({ .HasMips = false }, filepath);
		SK_CORE_VERIFY(equirectangular->GetSpecification().Format == ImageFormat::RGBA32F, "Environment Texture is not HDR!");

		const uint32_t mipCount = ImageUtils::CalcMipLevels(cubemapSize, cubemapSize);

		TextureSpecification cubemapSpec;
		cubemapSpec.Format = ImageFormat::RGBA32F;
		cubemapSpec.Width = cubemapSize;
		cubemapSpec.Height = cubemapSize;
		cubemapSpec.HasMips = true;
		cubemapSpec.Storage = true;

		cubemapSpec.DebugName = fmt::format("EnvironmentMap Filtered {}", filepath);
		auto radiance = TextureCube::Create(cubemapSpec);

		cubemapSpec.Width = irradianceMapSize;
		cubemapSpec.Height = irradianceMapSize;
		cubemapSpec.DebugName = fmt::format("IrradianceMap {}", filepath);
		auto irradiance = TextureCube::Create(cubemapSpec);

		RT_CreateEnvironmentMap(commandBuffer, radiance, irradiance, filepath);

		return { radiance, irradiance };
	}

	std::pair<Ref<TextureCube>, Ref<TextureCube>> Renderer::RT_CreateEnvironmentMap(const std::filesystem::path& filepath)
	{
		auto commandBuffer = RenderCommandBuffer::Create(fmt::format("CreateEnvironmentMap '{}'", filepath));
		commandBuffer->RT_Begin();
		auto result = RT_CreateEnvironmentMap(commandBuffer, filepath);
		commandBuffer->RT_End();
		commandBuffer->RT_Execute();
		return result;
	}

	#pragma endregion

	///////////////////////////////////////////////////////////////////////////////////////////////////
	//// Multi Threaded
	///////////////////////////////////////////////////////////////////////////////////////////////////

	#pragma region Multi Treaded

	void Renderer::MT::GenerateMips(Ref<Image2D> targetImage)
	{
		MT::Submit([targetImage]()
		{
			auto commandBuffer = RenderCommandBuffer::Create(fmt::format("GenerateMips '{}'", targetImage->GetSpecification().DebugName));
			commandBuffer->RT_Begin();
			RT_GenerateMips(commandBuffer, targetImage);
			commandBuffer->RT_End();
			commandBuffer->RT_Execute();
		});
	}

	std::pair<Ref<TextureCube>, Ref<TextureCube>> Renderer::MT::CreateEnvironmentMap(const std::filesystem::path& filepath)
	{
		const uint32_t cubemapSize = Renderer::GetConfig().EnvironmentMapResolution;
		const uint32_t irradianceMapSize = 32;

		TextureSpecification cubemapSpec;
		cubemapSpec.Format = ImageFormat::RGBA32F;
		cubemapSpec.Width = cubemapSize;
		cubemapSpec.Height = cubemapSize;
		cubemapSpec.HasMips = true;
		cubemapSpec.Storage = true;

		cubemapSpec.DebugName = fmt::format("EnvironmentMap Filtered {}", filepath);
		auto radiance = TextureCube::Create(cubemapSpec);

		cubemapSpec.Width = irradianceMapSize;
		cubemapSpec.Height = irradianceMapSize;
		cubemapSpec.DebugName = fmt::format("IrradianceMap {}", filepath);
		auto irradiance = TextureCube::Create(cubemapSpec);

		MT::Submit([radiance, irradiance, filepath]()
		{
			auto commandBuffer = RenderCommandBuffer::Create(fmt::format("CreateEnvironmentMap '{}'", filepath));
			commandBuffer->RT_Begin();
			RT_CreateEnvironmentMap(commandBuffer, radiance, irradiance, filepath);
			commandBuffer->RT_End();
			commandBuffer->RT_Execute();
		});

		return { radiance, irradiance };
	}

	#pragma endregion

	ShaderCache& Renderer::GetShaderCache()
	{
		return s_Data->m_ShaderCache;
	}

	void Renderer::ShaderReloaded(Ref<Shader> shader)
	{
		SK_NOT_IMPLEMENTED();
		// #Renderer #Disabled reload/recompile shader
#if TODO
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
#endif
	}

	void Renderer::AcknowledgeShaderDependency(Ref<Shader> shader, Weak<Material> material)
	{
#if TODO
		s_Data->m_ShaderDependencies[shader->GetHash()].Materials.push_back(material);
#endif
	}

	void Renderer::AcknowledgeShaderDependency(Ref<Shader> shader, Weak<RenderPass> renderPass)
	{
#if TODO
		s_Data->m_ShaderDependencies[shader->GetHash()].RenderPasses.push_back(renderPass);
#endif
	}

	void Renderer::ReportLiveObejcts()
	{
		//s_RendererContext->ReportLiveObjects();
	}

	uint32_t Renderer::GetCurrentFrameIndex()
	{
		return s_Data->m_FrameIndex;
	}

	uint32_t Renderer::RT_GetCurrentFrameIndex()
	{
		return s_Data->m_RTFrameIndex;
	}

	Ref<ShaderLibrary> Renderer::GetShaderLibrary()
	{
		return s_Data->m_ShaderLibrary;
	}

	const ResourceCache* Renderer::GetResourceCache()
	{
		return &s_Data->m_ResourceCache;
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

	Ref<Image2D> Renderer::GetBRDFLUTTexture()
	{
		return s_Data->m_BRDFLUTTexture;
	}

	Ref<Sampler> Renderer::GetLinearClampSampler()
	{
		return s_Data->m_Samplers.LinearClamp;
	}

	Ref<Sampler> Renderer::GetNearestClampSampler()
	{
		return s_Data->m_Samplers.NearestClamp;
	}

	const Samplers& Renderer::GetSamplers()
	{
		return s_Data->m_Samplers;
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
		return *s_CommandQueue[0];
	}

	std::pair<RenderCommandQueue&, std::unique_lock<std::mutex>> Renderer::GetMTCommandQueue()
	{
		std::unique_lock lock(s_MTCommandQueueMutex);

		return { *s_MTCommandQueue[0], std::move(lock) };
	}

	Ref<Image2D> Renderer::CreateBRDFLUT()
	{
		auto shader = Renderer::GetShaderLibrary()->Get("BRDF_LUT");
		const uint32_t imageSize = 256;

		ImageSpecification spec;
		spec.Format = ImageFormat::RG16F;
		spec.Width = imageSize;
		spec.Height = imageSize;
		spec.Usage = ImageUsage::Storage;
		spec.DebugName = "BRDF_LUT";
		auto image = Image2D::Create(spec);


		auto pipeline = ComputePipeline::Create(shader, "BRDF_LUT");
		auto pass = ComputePass::Create(shader, "BRDF_LUT");
		pass->SetInput("o_LUT", image);
		SK_CORE_VERIFY(pass->Validate());
		pass->Bake();

		auto commandBuffer = RenderCommandBuffer::Create("BRDF_LUT");

		commandBuffer->Begin();
		Renderer::BeginComputePass(commandBuffer, pass);
		Renderer::Dispatch(commandBuffer, pipeline, { imageSize / 32, imageSize / 32, 1 });
		Renderer::EndComputePass(commandBuffer, pass);
		commandBuffer->End();

		return image;
	}

	void ResourceCache::Add(std::string_view key, nvrhi::ResourceHandle handle)
	{
		auto strKey = std::string(key);
		SK_CORE_VERIFY(m_Resources.contains(strKey) == false);
		m_Resources[strKey] = handle;
	}

	void ResourceCache::Remove(std::string_view key)
	{
		auto strKey = std::string(key);
		m_Resources.erase(strKey);
	}

	bool ResourceCache::Contains(std::string_view key) const
	{
		return m_Resources.find(key) != m_Resources.end();
	}

	nvrhi::ResourceHandle ResourceCache::Get(std::string_view key) const
	{
		return m_Resources.find(key)->second;
	}

}