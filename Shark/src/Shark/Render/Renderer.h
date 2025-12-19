#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Buffer.h"

#include "Shark/Render/DeviceManager.h"

#include "Shark/Render/RenderCommandQueue.h"
#include "Shark/Render/RenderCommandBuffer.h"

#include "Shark/Render/GpuBuffer.h"
#include "Shark/Render/Buffers.h"
#include "Shark/Render/Image.h"
#include "Shark/Render/Texture.h"
#include "Shark/Render/Environment.h"
#include "Shark/Render/Mesh.h"
#include "Shark/Render/MeshSource.h"

#include "Shark/Render/Material.h"
#include "Shark/Render/MaterialAsset.h"
#include "Shark/Render/RenderPass.h"

#include "Shark/Render/FrameBuffer.h"
#include "Shark/Render/Shader.h"
#include "Shark/Render/Pipeline.h"
#include "Shark/Render/ShaderCompiler/ShaderCache.h"

#include "Shark/Render/ComputePass.h"
#include "Shark/Render/ComputePipeline.h"

#include <nvrhi/nvrhi.h>

namespace Shark {

	struct Samplers
	{
		Ref<Sampler> NearestRepeat;
		Ref<Sampler> NearestClamp;
		Ref<Sampler> NearestMirrorRepeat;
		Ref<Sampler> LinearRepeat;
		Ref<Sampler> LinearClamp;
		Ref<Sampler> LinearMirrorRepeat;
	};

	struct RendererConfig
	{
		uint32_t EnvironmentMapResolution = 1024;
		uint32_t IrradianceMapComputeSamples = 512;
	};

	struct RendererCapabilities
	{
		uint32_t MaxMipLeves;
		uint32_t MaxAnisotropy;
	};

	struct BlitImageParams
	{
		ImageSlice SourceBaseSlice = { 0, 0 };
		ImageSlice DestinationBaseSlice = { 0, 0 };
		uint32_t LayerCount = 1;

		std::optional<glm::uvec2> SourceMin, SourceMax;
		std::optional<glm::uvec2> DestinationMin, DestinationMax;
	};

	class Renderer
	{
	public:
		static void Init();
		static void ShutDown();

		static DeviceManager* GetDeviceManager();
		static nvrhi::IDevice* GetGraphicsDevice();

		static void BeginEventMarker(Ref<RenderCommandBuffer> commandBuffer, const std::string& name);
		static void EndEventMarker(Ref<RenderCommandBuffer> commandBuffer);

		static void BeginFrame();
		static void EndFrame();

		static void WaitAndRender();

		template<typename TFunc>
		static void Submit(const TFunc& func)
		{
			auto& commandQueue = GetCommandQueue();
			//SK_CORE_VERIFY(!commandQueue.IsExecuting());
			
			auto command = [](void* funcPtr)
			{
				auto cmdPtr = (TFunc*)funcPtr;
				(*cmdPtr)();
				cmdPtr->~TFunc();
			};

			void* storage = commandQueue.Allocate(command, sizeof(TFunc));
			new (storage) TFunc(func);
		}

		static void ClearFramebuffer(Ref<RenderCommandBuffer> commandBuffer, Ref<FrameBuffer> framebuffer);
		static void RT_ClearFramebuffer(Ref<RenderCommandBuffer> commandBuffer, Ref<FrameBuffer> framebuffer);

		static void WriteBuffer(Ref<RenderCommandBuffer> commandBuffer, Ref<GpuBuffer> buffer, const Buffer bufferData);
		static void RT_WriteBuffer(Ref<RenderCommandBuffer> commandBuffer, Ref<GpuBuffer> buffer, const Buffer bufferData);

		static void BeginRenderPass(Ref<RenderCommandBuffer> commandBuffer, Ref<RenderPass> renderPass, bool expliciteClear = false);
		static void EndRenderPass(Ref<RenderCommandBuffer> commandBuffer, Ref<RenderPass> renderPass);
		static void BeginComputePass(Ref<RenderCommandBuffer> commandBuffer, Ref<ComputePass> computePass);
		static void EndComputePass(Ref<RenderCommandBuffer> commandBuffer, Ref<ComputePass> computePass);

		static void Dispatch(Ref<RenderCommandBuffer> commandBuffer, Ref<ComputePipeline> pipeline, const glm::uvec3& workGroups, const Buffer pushConstantData = {});
		static void Dispatch(Ref<RenderCommandBuffer> commandBuffer, Ref<ComputePipeline> pipeline, Ref<Material> material, const glm::uvec3& workGroups, const Buffer pushConstantData = {});

		static void RenderFullScreenQuad(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, Buffer pushConstantsData = {});

		static void BeginBatch(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer);
		static void RenderBatch(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Material> material, uint32_t indexCount, uint32_t startIndex);
		static void EndBatch(Ref<RenderCommandBuffer> renderCommandBuffer);

		static void RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, uint32_t indexCount, Buffer pushConstant = {});
		static void RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, const nvrhi::DrawArguments& drawArguments, Buffer pushConstant = {});
		static void RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, Ref<VertexBuffer> vertexBuffer, uint32_t vertexCount);

		static void RenderCube(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Material> material);

		static void RenderSubmesh(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Mesh> mesh, uint32_t submeshIndex, Ref<MaterialTable> materialTable);
		static void RenderSubmeshWithMaterial(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Mesh> mesh, Ref<MeshSource> meshSource, uint32_t submeshIndex, Ref<Material> material, Buffer pushConstantsData);

		static void CopyImage(Ref<Image2D> sourceImage, Ref<Image2D> destinationImage);
		static void CopyImage(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> sourceImage, Ref<Image2D> destinationImage);
		static void CopyMip(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> sourceImage, uint32_t sourceMip, Ref<Image2D> destinationImage, uint32_t destinationMip);
		static void BlitImage(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> sourceImage, Ref<Image2D> destinationImage, uint32_t mipSlice, FilterMode filterMode);
		static void BlitImage(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> sourceImage, Ref<Image2D> destinationImage, const BlitImageParams& params, FilterMode filterMode);

		static void GenerateMips(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> image);

		static void GenerateMips(Ref<Image2D> image);
		static void RT_GenerateMips(Ref<Image2D> image);

		static std::pair<Ref<TextureCube>, Ref<TextureCube>> CreateEnvironmentMap(const std::filesystem::path& filepath);
		static std::pair<Ref<TextureCube>, Ref<TextureCube>> RT_CreateEnvironmentMap(const std::filesystem::path& filepath);

		static ShaderCache& GetShaderCache();
		static void ShaderReloaded(Ref<Shader> shader);
		static void AcknowledgeShaderDependency(Ref<Shader> shader, Weak<Material> material);
		static void AcknowledgeShaderDependency(Ref<Shader> shader, Weak<RenderPass> renderPass);

		static void ReportLiveObejcts();

		static uint32_t GetCurrentFrameIndex();
		static uint32_t RT_GetCurrentFrameIndex();

	public:
		static Ref<ShaderLibrary> GetShaderLibrary();
		static Ref<Texture2D> GetWhiteTexture();
		static Ref<Texture2D> GetBlackTexture();
		static Ref<TextureCube> GetBlackTextureCube();
		static Ref<Environment> GetEmptyEnvironment();
		static Ref<Image2D> GetBRDFLUTTexture();
		static Ref<Sampler> GetLinearClampSampler();
		static Ref<Sampler> GetNearestClampSampler();
		static const Samplers& GetSamplers();

		static std::pair<nvrhi::BindingLayoutHandle, nvrhi::BindingSetHandle> GetBindingSet(const std::string& name);

		static RendererCapabilities& GetCapabilities();
		static bool IsOnRenderThread();

		static RendererConfig& GetConfig();
		static void SetConfig(const RendererConfig& config);

	private:
		static RenderCommandQueue& GetCommandQueue();

		static Ref<Image2D> CreateBRDFLUT();

	private:
		friend class DirectXRenderer;
	};

}
