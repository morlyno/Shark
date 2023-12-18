#pragma once

#include "Shark/Core/Base.h"

#include "Shark/Render/RendererAPI.h"

#include "Shark/Render/RenderCommandQueue.h"
#include "Shark/Render/Buffers.h"
#include "Shark/Render/Shader.h"
#include "Shark/Render/Texture.h"
#include "Shark/Render/RenderCommandBuffer.h"
#include "Shark/Render/ConstantBuffer.h"
#include "Shark/Render/FrameBuffer.h"
#include "Shark/Render/Pipeline.h"
#include "Shark/Render/SwapChain.h"

namespace Shark {

	class Renderer
	{
	public:
		static void Init();
		static void ShutDown();

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
			Submit(func);
		}

		static void RenderFullScreenQuad(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Material> material);

		static void BeginBatch(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer);
		static void RenderBatch(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Material> material, uint32_t indexCount, uint32_t startIndex);
		static void EndBatch(Ref<RenderCommandBuffer> renderCommandBuffer);

		static void RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, uint32_t indexCount);
		static void RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, Ref<VertexBuffer> vertexBuffer, uint32_t vertexCount);

		static void RenderSubmesh(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Mesh> mesh, uint32_t submeshIndex, Ref<Pipeline> pipeline);
		static void RenderSubmesh(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Mesh> mesh, uint32_t submeshIndex, Ref<ConstantBuffer> sceneData, Ref<ConstantBuffer> meshData, Ref<ConstantBuffer> lightData);
		static void RenderSubmeshWithMaterial(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Mesh> mesh, uint32_t submeshIndex, Ref<Material> material, Ref<ConstantBuffer> sceneData);
		static void RenderSubmeshWithMaterial(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Mesh> mesh, uint32_t submeshIndex, Ref<Material> material, Ref<ConstantBuffer> sceneData, Ref<ConstantBuffer> meshData, Ref<ConstantBuffer> lightData);

		static Ref<SamplerWrapper> GetClampLinearSampler();

		static void GenerateMips(Ref<Image2D> image);
		static void RT_GenerateMips(Ref<Image2D> image);

		static void ReportLiveObejcts();

	public:
		static Ref<RendererAPI> GetRendererAPI();

		static Ref<ShaderLibrary> GetShaderLibrary();
		static Ref<Texture2D> GetWhiteTexture();
		static Ref<Texture2D> GetBlackTexture();
		static Ref<Texture2D> GetBlueTexture();

		static const RendererCapabilities& GetCapabilities();
		static bool IsOnRenderThread();

	private:
		static RenderCommandQueue& GetCommandQueue();

	private:
		static Ref<RendererAPI> s_RendererAPI;
		friend class DirectXRenderer;
	};

}
