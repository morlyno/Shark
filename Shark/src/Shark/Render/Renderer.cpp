#include "skpch.h"
#include "Renderer.h"

#include "Shark/Debug/Profiler.h"
#include "Shark/Render/Buffers.h"

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
		std::map<std::string, std::pair<nvrhi::BindingLayoutHandle, nvrhi::BindingSetHandle>> m_BindingSets;

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

	static constexpr uint32_t s_CommandQueueCount = 2;
	static RenderCommandQueue* s_CommandQueue[s_CommandQueueCount];
	static uint32_t s_CommandQueueSubmissionIndex = 0;

	static bool s_SingleThreadedIsExecuting = false;

	void Renderer::Init()
	{
		SK_PROFILE_FUNCTION();

		s_CommandQueue[0] = sknew RenderCommandQueue();
		s_CommandQueue[1] = sknew RenderCommandQueue();

		s_Data = sknew RendererData;
		s_Data->m_ShaderLibrary = Ref<ShaderLibrary>::Create();
		s_Data->m_ShaderCache.LoadRegistry();

		{
			s_Data->m_Samplers.NearestRepeat       = Sampler::Create({ .Filter = FilterMode::Nearest, .Address = AddressMode::Repeat });
			s_Data->m_Samplers.NearestClamp        = Sampler::Create({ .Filter = FilterMode::Nearest, .Address = AddressMode::ClampToEdge });
			s_Data->m_Samplers.NearestMirrorRepeat = Sampler::Create({ .Filter = FilterMode::Nearest, .Address = AddressMode::MirrorRepeat });
			s_Data->m_Samplers.LinearRepeat        = Sampler::Create({ .Filter = FilterMode::Linear,  .Address = AddressMode::Repeat });
			s_Data->m_Samplers.LinearClamp         = Sampler::Create({ .Filter = FilterMode::Linear,  .Address = AddressMode::ClampToEdge });
			s_Data->m_Samplers.LinearMirrorRepeat  = Sampler::Create({ .Filter = FilterMode::Linear,  .Address = AddressMode::MirrorRepeat });
		}

		Log::EnabledTags()["ShaderCompiler"].Level = LogLevel::Warn;
		Timer loadShadersTimer;

		SK_CORE_INFO_TAG("Renderer", "Loading Shaders...");

		auto shaderLibrary = GetShaderLibrary();
		shaderLibrary->SetCompilerOptions({ .ForceCompile = false, .Optimize = true, .GenerateDebugInfo = true });

		// 3D
		shaderLibrary->Load("Resources/Shaders/Simple.hlsl");
		shaderLibrary->Load("Resources/Shaders/SharkPBR.hlsl");
		shaderLibrary->Load("Resources/Shaders/Skybox.hlsl");
		shaderLibrary->Load("Resources/Shaders/BRDF_LUT.hlsl");
		shaderLibrary->Load("Resources/Shaders/Tonemap.hlsl");
		shaderLibrary->Load("Resources/Shaders/Composite.hlsl");

		// 2D
		shaderLibrary->Load("Resources/Shaders/Renderer2D_Quad.hlsl");
		shaderLibrary->Load("Resources/Shaders/2D/Renderer2D_Circle.hlsl");
		shaderLibrary->Load("Resources/Shaders/2D/Renderer2D_Line.hlsl");
		shaderLibrary->Load("Resources/Shaders/2D/Renderer2D_Text.hlsl");

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
		//shaderLibrary->Load("resources/Shaders/Commands/BlitImage.glsl");
		shaderLibrary->Load("Resources/Shaders/Commands/LinearSample.hlsl");
		shaderLibrary->Load("Resources/Shaders/Commands/LinearSampleArray.hlsl");

		shaderLibrary->Load("Resources/Shaders/ImGui.hlsl");
		s_Data->m_ShaderCache.SaveRegistry();

		SK_CORE_INFO_TAG("Renderer", "Finished loading shaders in {}", loadShadersTimer.Elapsed());


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
			spec.GenerateMips = false;

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

		// Sampler binding set
		{
			nvrhi::BindingSetDesc set;
			set.trackLiveness = false;
			set.bindings = {
				nvrhi::BindingSetItem::Sampler(0, s_Data->m_Samplers.NearestRepeat->GetHandle()),
				nvrhi::BindingSetItem::Sampler(1, s_Data->m_Samplers.NearestClamp->GetHandle()),
				nvrhi::BindingSetItem::Sampler(2, s_Data->m_Samplers.NearestMirrorRepeat->GetHandle()),
				nvrhi::BindingSetItem::Sampler(3, s_Data->m_Samplers.LinearRepeat->GetHandle()),
				nvrhi::BindingSetItem::Sampler(4, s_Data->m_Samplers.LinearClamp->GetHandle()),
				nvrhi::BindingSetItem::Sampler(5, s_Data->m_Samplers.LinearMirrorRepeat->GetHandle()),
			};

			nvrhi::BindingLayoutHandle layoutHandle;
			nvrhi::BindingSetHandle setHandle;
			nvrhi::utils::CreateBindingSetAndLayout(GetGraphicsDevice(), nvrhi::ShaderType::All, 3, set, layoutHandle, setHandle, true);
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

		s_Data->m_ShaderCache.SaveRegistry();

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

		skdelete s_CommandQueue[0];
		skdelete s_CommandQueue[1];
	}

	DeviceManager* Renderer::GetDeviceManager()
	{
		return Application::Get().GetDeviceManager();
	}

	nvrhi::IDevice* Renderer::GetGraphicsDevice()
	{
		return GetDeviceManager()->GetDevice();
	}

	void Renderer::BeginEventMarker(Ref<RenderCommandBuffer> commandBuffer, const std::string& name)
	{
		commandBuffer->BeginMarker(name.c_str());
	}

	void Renderer::EndEventMarker(Ref<RenderCommandBuffer> commandBuffer)
	{
		commandBuffer->EndMarker();
	}

	void Renderer::BeginFrame()
	{
		//s_RendererAPI->BeginFrame();
		s_Data->m_FrameIndex++;
		Renderer::Submit([]() { s_Data->m_RTFrameIndex++; });
	}

	void Renderer::EndFrame()
	{
		//s_RendererAPI->EndFrame();
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

	static void RT_ClearFramebufferAttachments(nvrhi::ICommandList* commandList, Ref<FrameBuffer> framebuffer, bool isLoadClear)
	{
		auto fbHandle = framebuffer->GetHandle();
		const auto& specification = framebuffer->GetSpecification();
		const bool clearColor = !isLoadClear || specification.ClearColorOnLoad;
		const bool clearDepth = !isLoadClear || specification.ClearDepthOnLoad;

		if (clearDepth && framebuffer->HasDepthAtachment())
		{
			nvrhi::utils::ClearDepthStencilAttachment(commandList, fbHandle, specification.ClearDepthValue, specification.ClearStencilValue);
		}

		if (!clearColor)
			return;

		for (uint32_t i = 0; i < framebuffer->GetAttachmentCount(); i++)
		{
			const auto& attachment = fbHandle->getDesc().colorAttachments[i];
			const auto& clearColor = framebuffer->GetClearColor(i);

			if (nvrhi::getFormatInfo(attachment.texture->getDesc().format).kind == nvrhi::FormatKind::Integer)
				commandList->clearTextureUInt(attachment.texture, attachment.subresources, (uint32_t)clearColor.a);
			else
				commandList->clearTextureFloat(attachment.texture, attachment.subresources, clearColor);
		}
	}

	void Renderer::ClearFramebuffer(Ref<RenderCommandBuffer> commandBuffer, Ref<FrameBuffer> framebuffer)
	{
		Submit([commandBuffer, framebuffer]() { RT_ClearFramebuffer(commandBuffer, framebuffer); });
	}

	void Renderer::RT_ClearFramebuffer(Ref<RenderCommandBuffer> commandBuffer, Ref<FrameBuffer> framebuffer)
	{
		RT_ClearFramebufferAttachments(commandBuffer->GetHandle(), framebuffer, false);
	}

	void Renderer::WriteBuffer(Ref<RenderCommandBuffer> commandBuffer, Ref<GpuBuffer> buffer, const Buffer bufferData)
	{
		Submit([commandBuffer, buffer, temp = Buffer::Copy(bufferData)]() mutable
		{
			RT_WriteBuffer(commandBuffer, buffer, temp);
			temp.Release();
		});
	}

	void Renderer::RT_WriteBuffer(Ref<RenderCommandBuffer> commandBuffer, Ref<GpuBuffer> buffer, const Buffer bufferData)
	{
		auto commandList = commandBuffer->GetHandle();
		commandList->writeBuffer(buffer->GetHandle(), bufferData.As<const void>(), bufferData.Size);
	}

	namespace utils {

		static void BindShaderInputManager(nvrhi::GraphicsState& state, const ShaderInputManager& inputManager)
		{
			Ref<Shader> shader = inputManager.GetShader();

			for (uint32_t set = inputManager.GetStartSet(); set <= inputManager.GetEndSet(); set++)
			{
				if (shader->HasLayout(set))
					state.bindings[shader->MapSet(set)] = inputManager.GetHandle(set);
			}
		}

		static void BindShaderInputManager(nvrhi::ComputeState& state, const ShaderInputManager& inputManager)
		{
			Ref<Shader> shader = inputManager.GetShader();

			for (uint32_t set = inputManager.GetStartSet(); set <= inputManager.GetEndSet(); set++)
			{
				if (shader->HasLayout(set))
					state.bindings[shader->MapSet(set)] = inputManager.GetHandle(set);
			}
		}

	}

	void Renderer::BeginRenderPass(Ref<RenderCommandBuffer> commandBuffer, Ref<RenderPass> renderPass, bool expliciteClear)
	{
		Ref<FrameBuffer> framebuffer = renderPass->GetTargetFramebuffer();
		Ref<Shader> shader = renderPass->GetSpecification().Shader;

		Submit([commandBuffer, renderPass, framebuffer, shader]()
		{
			SK_CORE_TRACE_TAG("Renderer", "Renderer - BeginRenderPass '{}'", renderPass->GetSpecification().DebugName);

			auto commandList = commandBuffer->GetHandle();
			commandList->beginMarker(renderPass->GetSpecification().DebugName.c_str());

			RT_ClearFramebufferAttachments(commandBuffer->GetHandle(), framebuffer, true);

			nvrhi::GraphicsState& graphicsState = commandBuffer->GetGraphicsState();

			graphicsState = nvrhi::GraphicsState();
			graphicsState.framebuffer = framebuffer->GetHandle();
			graphicsState.viewport.addViewport(framebuffer->GetViewport());
			graphicsState.vertexBuffers.resize(1);
			graphicsState.bindings.resize(shader->GetBindingLayouts().size());

			utils::BindShaderInputManager(graphicsState, renderPass->GetInputManager());
		});
	}

	void Renderer::EndRenderPass(Ref<RenderCommandBuffer> commandBuffer, Ref<RenderPass> renderPass)
	{
		// 
		// NOTE(moro): Do not remove commandBuffer and renderPass!
		//             This capture keeps them alive until the pass finished.
		// 
		Renderer::Submit([commandBuffer, renderPass]()
		{
			SK_CORE_TRACE_TAG("Renderer", "Renderer - EndRenderPass '{}'", renderPass->GetSpecification().DebugName);

			auto commandList = commandBuffer->GetHandle();
			commandList->endMarker();
		});
	}

	void Renderer::BeginComputePass(Ref<RenderCommandBuffer> commandBuffer, Ref<ComputePass> computePass)
	{
		Renderer::Submit([commandBuffer, computePass, shader = computePass->GetShader()]()
		{
			SK_CORE_TRACE_TAG("Renderer", "Renderer - BeginComputePass '{}'", computePass->GetSpecification().DebugName);

			auto commandList = commandBuffer->GetHandle();
			commandList->beginMarker(computePass->GetSpecification().DebugName.c_str());

			nvrhi::ComputeState& computeState = commandBuffer->GetComputeState();
			computeState = nvrhi::ComputeState();
			computeState.bindings.resize(shader->GetBindingLayouts().size());

			utils::BindShaderInputManager(computeState, computePass->GetInputManager());
		});
	}

	void Renderer::EndComputePass(Ref<RenderCommandBuffer> commandBuffer, Ref<ComputePass> computePass)
	{
		// 
		// NOTE(moro): Do not remove commandBuffer and computePass!
		//             This capture keeps them alive until the pass finished.
		// 
		Renderer::Submit([commandBuffer, computePass]()
		{
			SK_CORE_TRACE_TAG("Renderer", "Renderer - EndComputePass '{}'", computePass->GetSpecification().DebugName);

			auto commandList = commandBuffer->GetHandle();
			commandList->endMarker();
		});
	}

	void Renderer::Dispatch(Ref<RenderCommandBuffer> commandBuffer, Ref<ComputePipeline> pipeline, const glm::vec3& workGroups, const Buffer pushConstantData)
	{
		Renderer::Submit([commandBuffer, pipeline, workGroups, temp = Buffer::Copy(pushConstantData)]() mutable
		{
			SK_CORE_TRACE_TAG("Renderer", "Renderer - Dispatch '{}' {}", pipeline->GetDebugName(), workGroups);

			nvrhi::ComputeState& computeState = commandBuffer->GetComputeState();
			computeState.pipeline = pipeline->GetHandle();

			auto commandList = commandBuffer->GetHandle();
			commandList->setComputeState(computeState);

			if (temp.Size)
				commandList->setPushConstants(temp.As<const void>(), temp.Size);

			commandList->dispatch(workGroups.x, workGroups.y, workGroups.z);
			temp.Release();
		});
	}

	void Renderer::Dispatch(Ref<RenderCommandBuffer> commandBuffer, Ref<ComputePipeline> pipeline, Ref<Material> material, const glm::vec3& workGroups, const Buffer pushConstantData)
	{
		Renderer::Submit([commandBuffer, pipeline, material, workGroups, temp = Buffer::Copy(pushConstantData)]() mutable
		{
			SK_CORE_TRACE_TAG("Renderer", "Renderer - Dispatch '{}' '{}' {}", pipeline->GetDebugName(), material->GetName(), workGroups);

			nvrhi::ComputeState& computeState = commandBuffer->GetComputeState();
			computeState.pipeline = pipeline->GetHandle();

			utils::BindShaderInputManager(computeState, material->GetInputManager());

			auto commandList = commandBuffer->GetHandle();
			commandList->setComputeState(computeState);
			commandList->setPushConstants(temp.As<const void>(), temp.Size);

			commandList->dispatch(workGroups.x, workGroups.y, workGroups.z);
			temp.Release();
		});
	}

	void Renderer::RenderFullScreenQuad(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, Buffer pushConstantsData)
	{
		Renderer::RenderGeometry(commandBuffer, pipeline, material, s_Data->m_QuadVertexBuffer, s_Data->m_QuadIndexBuffer, s_Data->m_QuadIndexBuffer->GetCount(), pushConstantsData);
	}

	void Renderer::BeginBatch(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer)
	{
		//s_RendererAPI->BeginBatch(renderCommandBuffer, pipeline, vertexBuffer, indexBuffer);
	}

	void Renderer::RenderBatch(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Material> material, uint32_t indexCount, uint32_t startIndex)
	{
		//s_RendererAPI->RenderBatch(renderCommandBuffer, material, indexCount, startIndex);
	}

	void Renderer::EndBatch(Ref<RenderCommandBuffer> renderCommandBuffer)
	{
		//s_RendererAPI->EndBatch(renderCommandBuffer);
	}

	void Renderer::RenderGeometry(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, uint32_t indexCount, Buffer pushConstant)
	{
		Renderer::Submit([=, temp = Buffer::Copy(pushConstant)]() mutable
		{
			nvrhi::GraphicsState graphicsState = commandBuffer->GetGraphicsState();

			graphicsState.pipeline = pipeline->GetHandle();

			nvrhi::VertexBufferBinding vbufBinding;
			vbufBinding.buffer = vertexBuffer->GetHandle();
			vbufBinding.slot = 0;
			vbufBinding.offset = 0;
			graphicsState.vertexBuffers[0] = vbufBinding;

			if (material)
			{
				utils::BindShaderInputManager(graphicsState, material->GetInputManager());
			}

			graphicsState.indexBuffer.buffer = indexBuffer->GetHandle();
			graphicsState.indexBuffer.format = nvrhi::Format::R32_UINT;
			graphicsState.indexBuffer.offset = 0;

			nvrhi::DrawArguments drawArgs;
			drawArgs.vertexCount = indexCount;

			auto commandList = commandBuffer->GetHandle();
			commandList->setGraphicsState(graphicsState);
			commandList->setPushConstants(temp.As<const void>(), temp.Size);
			commandList->drawIndexed(drawArgs);
			temp.Release();
		});
	}

	void Renderer::RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, Ref<VertexBuffer> vertexBuffer, uint32_t vertexCount)
	{
	}

	void Renderer::RenderCube(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Material> material)
	{
		RenderGeometry(commandBuffer, pipeline, material, s_Data->m_CubeVertexBuffer, s_Data->m_CubeIndexBuffer, s_Data->m_CubeIndexBuffer->GetCount());
	}

	void Renderer::RenderSubmesh(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Mesh> mesh, uint32_t submeshIndex, Ref<MaterialTable> materialTable)
	{
		//s_RendererAPI->RenderSubmesh(commandBuffer, pipeline, mesh, submeshIndex, materialTable);
	}

	void Renderer::RenderSubmeshWithMaterial(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Mesh> mesh, Ref<MeshSource> meshSource, uint32_t submeshIndex, Ref<Material> material, Buffer pushConstantsData)
	{
		Renderer::Submit([=, temp = Buffer::Copy(pushConstantsData)]() mutable
		{
			auto vertexBuffer = meshSource->GetVertexBuffer();
			auto indexBuffer = meshSource->GetIndexBuffer();

			nvrhi::GraphicsState& drawState = commandBuffer->GetGraphicsState();

			drawState.pipeline = pipeline->GetHandle();

			nvrhi::VertexBufferBinding vbufBinding;
			vbufBinding.buffer = vertexBuffer->GetHandle();
			vbufBinding.slot = 0;
			vbufBinding.offset = 0;
			drawState.vertexBuffers[0] = vbufBinding;

			if (material)
			{
				utils::BindShaderInputManager(drawState, material->GetInputManager());
			}

			drawState.indexBuffer.buffer = indexBuffer->GetHandle();
			drawState.indexBuffer.format = nvrhi::Format::R32_UINT;
			drawState.indexBuffer.offset = 0;

			const auto& submeshes = meshSource->GetSubmeshes();
			const auto& submesh = submeshes[submeshIndex];

			nvrhi::DrawArguments drawArgs;
			drawArgs.vertexCount = submesh.IndexCount;
			drawArgs.startIndexLocation = submesh.BaseIndex;
			drawArgs.startVertexLocation = submesh.BaseVertex;

			auto commandList = commandBuffer->GetHandle();
			commandList->setGraphicsState(drawState);
			commandList->setPushConstants(temp.As<const void>(), temp.Size);
			commandList->drawIndexed(drawArgs);
			temp.Release();
		});
	}

	void Renderer::CopyImage(Ref<Image2D> sourceImage, Ref<Image2D> destinationImage)
	{
		//s_RendererAPI->CopyImage(sourceImage, destinationImage);
	}

	void Renderer::CopyImage(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> sourceImage, Ref<Image2D> destinationImage)
	{
		Renderer::Submit([commandBuffer, sourceImage, destinationImage]()
		{
			SK_PROFILE_SCOPED("Renderer - CopyImage");
			SK_CORE_TRACE_TAG("Renderer", "CopyImage '{}' -> '{}'", sourceImage->GetSpecification().DebugName, destinationImage->GetSpecification().DebugName);

			auto commandList = commandBuffer->GetHandle();
			
			nvrhi::ITexture* srcTex = sourceImage->GetHandle();
			nvrhi::ITexture* dstTex = destinationImage->GetHandle();

			nvrhi::TextureSlice slice;

			for (uint32_t mip = 0; mip < srcTex->getDesc().mipLevels; mip++)
			{
				slice.mipLevel = mip;

				for (uint32_t layer = 0; layer < srcTex->getDesc().arraySize; layer++)
				{
					slice.arraySlice = layer;
					commandList->copyTexture(dstTex, slice, srcTex, slice);
				}
			}

		});
	}

	void Renderer::CopyMip(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> sourceImage, uint32_t sourceMip, Ref<Image2D> destinationImage, uint32_t destinationMip)
	{
		Renderer::Submit([commandBuffer, sourceImage, sourceMip, destinationImage, destinationMip]()
		{
			SK_PROFILE_SCOPED("Renderer - CopyMip");
			SK_CORE_TRACE_TAG("Renderer", "CopyMip '{}':{} -> '{}':{}", sourceImage->GetSpecification().DebugName, sourceMip, destinationImage->GetSpecification().DebugName, destinationMip);

			auto commandList = commandBuffer->GetHandle();

			auto srcSlice = nvrhi::TextureSlice().setMipLevel(sourceMip);
			auto dstSlice = nvrhi::TextureSlice().setMipLevel(destinationMip);

			nvrhi::ITexture* srcTex = sourceImage->GetHandle();
			nvrhi::ITexture* dstTex = destinationImage->GetHandle();

			for (uint32_t layer = 0; layer < srcTex->getDesc().arraySize; layer++)
			{
				srcSlice.arraySlice = layer;
				dstSlice.arraySlice = layer;

				commandList->copyTexture(dstTex, dstSlice,
										 srcTex, srcSlice);
			}
		});
	}

	void Renderer::BlitImage(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> sourceImage, Ref<Image2D> destinationImage)
	{
		//s_RendererAPI->BlitImage(commandBuffer, sourceImage, destinationImage);
	}

	static void GenerateMipsForLayer(Ref<RenderCommandBuffer> commandBuffer, Ref<ComputePipeline> pipeline, Ref<Image2D> targetImage, uint32_t layer)
	{
		SK_PROFILE_SCOPED("Renderer - GenerateMips");
		SK_CORE_VERIFY(!ImageUtils::IsIntegerBased(targetImage->GetSpecification().Format));
		SK_CORE_VERIFY(targetImage->GetSpecification().Usage == ImageUsage::Storage);

		static constexpr uint32_t NUM_LODS = 4;
		Ref<ViewableResource> targetView = targetImage;

		if (targetImage->GetSpecification().Layers > 1)
		{
			ImageViewSpecification viewSpec;
			viewSpec.BaseLayer = layer;
			viewSpec.LayerCount = 1;
			viewSpec.BaseMip = 0;
			viewSpec.MipCount = targetImage->GetSpecification().MipLevels;
			viewSpec.Dimension = nvrhi::TextureDimension::Texture2DArray;
			targetView = ImageView::Create(targetImage, viewSpec);
		}

		struct Settings
		{
			uint32_t Dispatch;
			uint32_t LODs;
		} settings;

		float targetMipWidth = (float)targetImage->GetWidth();
		float targetMipHeight = (float)targetImage->GetHeight();

		auto dstSubresource = nvrhi::TextureSubresourceSet(0, 1, layer, 1);
		auto srcSubresource = nvrhi::TextureSubresourceSet(0, 1, layer, 1);

		const uint32_t layers = targetImage->GetSpecification().Layers;
		const uint32_t mipLevels = targetImage->GetSpecification().MipLevels;
		for (uint32_t baseMip = 1, dispatch = 0; baseMip < mipLevels; baseMip += NUM_LODS, dispatch++)
		{
			targetMipWidth /= 2;
			targetMipHeight /= 2;

			settings.Dispatch = dispatch;
			settings.LODs = std::min(mipLevels - baseMip, NUM_LODS);

			auto material = Material::Create(pipeline->GetShader(), fmt::format("Mip {}-{} Material", baseMip, baseMip + NUM_LODS - 1));
			auto& inputManager = material->GetInputManager();

			inputManager.SetInput("u_Source", targetView);
			inputManager.SetInputSubresourceSet("u_Source", srcSubresource.setBaseMipLevel(baseMip - 1), 0);

			for (uint32_t i = 0; i < NUM_LODS; i++)
			{
				if ((baseMip + i) < (mipLevels))
				{
					dstSubresource.baseMipLevel = baseMip + i;
					inputManager.SetInput("o_Mips", targetView, i);
					inputManager.SetInputSubresourceSet("o_Mips", dstSubresource, i);
				}
				else
				{
					auto nullUAV = (targetImage->GetSpecification().Layers == 1) ? s_Data->m_NullUAVs[i - 1] : s_Data->m_NullArrayUAVs[i - 1];
					inputManager.SetInput("o_Mips", nullUAV, i);
				}
			}

			material->Prepare();

			const auto workGroups = glm::max({ targetMipWidth / 8, targetMipHeight / 8, 1 }, glm::uvec3(1));
			Renderer::Dispatch(commandBuffer, pipeline, material, workGroups, Buffer::FromValue(settings));
		}

	}

	static ImageFormat ConvertToWritableFormat(ImageFormat format)
	{
		switch (format)
		{
			case ImageFormat::sRGBA: return ImageFormat::RGBA;
		}

		SK_CORE_ASSERT(!ImageUtils::IsSRGB(format));
		return format;
	}

	void Renderer::GenerateMips(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> targetImage)
	{
		SK_PROFILE_SCOPED("Renderer - GenerateMips");
		SK_CORE_VERIFY(!ImageUtils::IsIntegerBased(targetImage->GetSpecification().Format));

		Ref<Image2D> generationImage = targetImage;
		if (targetImage->GetSpecification().Usage != ImageUsage::Storage)
		{
			ImageSpecification specification = targetImage->GetSpecification();
			specification.Usage = ImageUsage::Storage;
			specification.Format = ConvertToWritableFormat(specification.Format);
			generationImage = Image2D::Create(specification);
			CopyMip(commandBuffer, targetImage, 0, generationImage, 0);
		}

		auto shader = generationImage->GetSpecification().Layers == 1 ? Renderer::GetShaderLibrary()->Get("LinearSample") : Renderer::GetShaderLibrary()->Get("LinearSampleArray");

		auto pipeline = ComputePipeline::Create(shader, "GenerateMips");
		auto pass = ComputePass::Create(shader, "GenerateMips");

		Renderer::BeginComputePass(commandBuffer, pass);

		for (uint32_t layer = 0; layer < generationImage->GetSpecification().Layers; layer++)
		{
			GenerateMipsForLayer(commandBuffer, pipeline, generationImage, layer);
		}

		Renderer::EndComputePass(commandBuffer, pass);

		if (generationImage != targetImage)
		{
			CopyImage(commandBuffer, generationImage, targetImage);
		}
	}

	void Renderer::GenerateMips(Ref<Image2D> image)
	{
		auto commandBuffer = RenderCommandBuffer::Create("GenerateMips Commands");
		commandBuffer->Begin();
		GenerateMips(commandBuffer, image);
		commandBuffer->End();
		commandBuffer->Execute();
	}

	void Renderer::RT_GenerateMips(Ref<Image2D> image)
	{
		//SK_NOT_IMPLEMENTED();
	}

	std::pair<Ref<TextureCube>, Ref<TextureCube>> Renderer::CreateEnvironmentMap(const std::filesystem::path& filepath)
	{
		SK_PROFILE_SCOPED("Renderer - CreateEnvironmentMap");

		const uint32_t cubemapSize = Renderer::GetConfig().EnvironmentMapResolution;
		const uint32_t irradianceMapSize = 32;

		Ref<Texture2D> equirectangular = Texture2D::Create({ .GenerateMips = false }, filepath);
		SK_CORE_VERIFY(equirectangular->GetSpecification().Format == ImageFormat::RGBA32F, "Environment Texture is not HDR!");

		const uint32_t mipCount = ImageUtils::CalcMipLevels(cubemapSize, cubemapSize);

		TextureSpecification cubemapSpec;
		cubemapSpec.Format = ImageFormat::RGBA32F;
		cubemapSpec.Width = cubemapSize;
		cubemapSpec.Height = cubemapSize;
		cubemapSpec.GenerateMips = true;
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

		auto commandBuffer = RenderCommandBuffer::Create("Environment");
		commandBuffer->Begin();

		{
			auto pipeline = ComputePipeline::Create(equirectToCubeShader);
			auto pass = ComputePass::Create(equirectToCubeShader, LayoutShareMode::PassOnly, "EquirectangularToCube");
			pass->SetInput("u_Equirect", equirectangular);
			pass->SetInput("u_Sampler", Renderer::GetLinearClampSampler());
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
			pass->SetInput("u_Sampler", Renderer::GetLinearClampSampler());
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
				material->Set("o_Filtered", filtered);

				auto& inputManager = material->GetInputManager();
				inputManager.SetInputSubresourceSet("o_Filtered", subresource.setBaseMipLevel(i));
				SK_CORE_VERIFY(material->Validate());
				material->Prepare();

				Dispatch(commandBuffer, pipeline, material, { workGroups, workGroups, 6 }, Buffer::FromValue(roughness));
			}
			EndComputePass(commandBuffer, pass);
		}

		{
			auto pipeline = ComputePipeline::Create(irradianceShader);
			auto pass = ComputePass::Create(irradianceShader, LayoutShareMode::PassOnly, "EnvIrradiance");
			pass->SetInput("o_Irradiance", irradianceMap);
			pass->SetInput("u_Radiance", filtered);
			pass->SetInput("u_Sampler", GetLinearClampSampler());
			SK_CORE_VERIFY(pass->Validate());
			pass->Bake();

			BeginComputePass(commandBuffer, pass);
			Dispatch(commandBuffer, pipeline, { irradianceMap->GetWidth() / 32, irradianceMap->GetHeight() / 32, 6 }, Buffer::FromValue(GetConfig().IrradianceMapComputeSamples));
			EndComputePass(commandBuffer, pass);


			GenerateMips(commandBuffer, irradianceMap->GetImage());
		}

		commandBuffer->End();
		commandBuffer->Execute();

		return { filtered, irradianceMap };
	}

	std::pair<Ref<TextureCube>, Ref<TextureCube>> Renderer::RT_CreateEnvironmentMap(const std::filesystem::path& filepath)
	{
		SK_NOT_IMPLEMENTED();
		return { nullptr, nullptr };
	}

	ShaderCache& Renderer::GetShaderCache()
	{
		return s_Data->m_ShaderCache;
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

	std::pair<nvrhi::BindingLayoutHandle, nvrhi::BindingSetHandle> Renderer::GetBindingSet(const std::string& name)
	{
		if (!s_Data->m_BindingSets.contains(name))
			return {};

		return s_Data->m_BindingSets.at(name);
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
		auto pass = ComputePass::Create(shader, LayoutShareMode::PassOnly, "BRDF_LUT");
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

}