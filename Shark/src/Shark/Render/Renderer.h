#pragma once

#include "Shark/Core/Base.h"

#include "Shark/Render/RendererAPI.h"
#include "Shark/Render/RendererContext.h"

#include "Shark/Render/RenderCommandQueue.h"
#include "Shark/Render/RenderCommandBuffer.h"
#include "Shark/Render/RenderPass.h"
#include "Shark/Render/Pipeline.h"
#include "Shark/Render/Buffers.h"
#include "Shark/Render/Shader.h"
#include "Shark/Render/Texture.h"
#include "Shark/Render/ConstantBuffer.h"
#include "Shark/Render/FrameBuffer.h"
#include "Shark/Render/SwapChain.h"
#include "Shark/Render/Environment.h"

namespace Shark {

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

	class Renderer
	{
	public:
		static void Init();
		static void ShutDown();

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

		template<typename TFunc>
		static void SubmitResourceFree(const TFunc& func)
		{
			auto& queue = GetResourceFreeQueue();

			auto command = [](void* funcPtr)
			{
				auto cmdPtr = (TFunc*)funcPtr;
				(*cmdPtr)();
				cmdPtr->~TFunc();
			};

			void* storage = queue.Allocate(command, sizeof(TFunc));
			new (storage) TFunc(func);
		}

		static void BeginRenderPass(Ref<RenderCommandBuffer> commandBuffer, Ref<RenderPass> renderPass, bool expliciteClear = false);
		static void EndRenderPass(Ref<RenderCommandBuffer> commandBuffer, Ref<RenderPass> renderPass);

		static void RenderFullScreenQuad(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Material> material);

		static void BeginBatch(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer);
		static void RenderBatch(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Material> material, uint32_t indexCount, uint32_t startIndex);
		static void EndBatch(Ref<RenderCommandBuffer> renderCommandBuffer);

		static void RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, uint32_t indexCount);
		static void RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, Ref<VertexBuffer> vertexBuffer, uint32_t vertexCount);

		static void RenderCube(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Material> material);

		static void RenderSubmesh(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Mesh> mesh, uint32_t submeshIndex, Ref<MaterialTable> materialTable);
		static void RenderSubmeshWithMaterial(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Mesh> mesh, Ref<MeshSource> meshSource, uint32_t submeshIndex, Ref<Material> material);

		static void CopyImage(Ref<Image2D> sourceImage, Ref<Image2D> destinationImage);
		static void CopyImage(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> sourceImage, Ref<Image2D> destinationImage);
		static void CopyMip(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> sourceImage, uint32_t sourceMip, Ref<Image2D> destinationImage, uint32_t destinationMip);
		static void BlitImage(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> sourceImage, Ref<Image2D> destinationImage);
		static void GenerateMips(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> image);

		static std::pair<Ref<TextureCube>, Ref<TextureCube>> CreateEnvironmentMap(const std::filesystem::path& filepath);
		static std::pair<Ref<TextureCube>, Ref<TextureCube>> RT_CreateEnvironmentMap(const std::filesystem::path& filepath);

		static void GenerateMips(Ref<Image2D> image);
		static void RT_GenerateMips(Ref<Image2D> image);

		static void ShaderReloaded(Ref<Shader> shader);
		static void AcknowledgeShaderDependency(Ref<Shader> shader, Weak<Material> material);
		static void AcknowledgeShaderDependency(Ref<Shader> shader, Weak<RenderPass> renderPass);

		static void ReportLiveObejcts();

		static uint32_t GetCurrentFrameIndex();
		static uint32_t RT_GetCurrentFrameIndex();

	public:
		static Ref<RendererContext> GetRendererContext();
		static Ref<RendererAPI> GetRendererAPI();

		static Ref<ShaderLibrary> GetShaderLibrary();
		static Ref<Texture2D> GetWhiteTexture();
		static Ref<Texture2D> GetBlackTexture();
		static Ref<TextureCube> GetBlackTextureCube();
		static Ref<Environment> GetEmptyEnvironment();
		static Ref<Texture2D> GetBRDFLUTTexture();

		static RendererCapabilities& GetCapabilities();
		static bool IsOnRenderThread();

		static RendererConfig& GetConfig();
		static void SetConfig(const RendererConfig& config);

	private:
		static RenderCommandQueue& GetCommandQueue();
		static RenderCommandQueue& GetResourceFreeQueue();

	private:
		static Ref<RendererContext> s_RendererContext;
		static Ref<RendererAPI> s_RendererAPI;
		friend class DirectXRenderer;
	};

}
