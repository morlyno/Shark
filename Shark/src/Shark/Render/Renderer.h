#pragma once

#include "Shark/Core/Base.h"

#include "Shark/Render/RendererAPI.h"

#include "Shark/Render/Buffers.h"
#include "Shark/Render/Shader.h"
#include "Shark/Render/Texture.h"
#include "Shark/Render/RenderCommandBuffer.h"
#include "Shark/Render/ConstantBuffer.h"
#include "Shark/Render/FrameBuffer.h"
#include "Shark/Render/Pipeline.h"
#include "Shark/Render/SwapChain.h"

namespace Shark {

	enum class RendererAPIType
	{
		None = 0,
		DirectX11
	};

	inline std::string_view ToStringView(RendererAPIType api)
	{
		switch (api)
		{
			case RendererAPIType::None: return "None"sv;
			case RendererAPIType::DirectX11: return "DirectX11"sv;
		}
		SK_CORE_ASSERT(false, "Unkown RendererAPIType");
		return "Unkown";
	}

	class Renderer
	{
	public:
		static void Init();
		static void ShutDown();

		static void BeginFrame();
		static void EndFrame();

		static void WaitAndRender();
		static bool IsOnRenderThread();
		static bool IsExecuting();

		static void ResizeSwapChain(uint32_t widht, uint32_t height);

		template<typename TFunc>
		static void Submit(const TFunc& func)
		{
			SK_CORE_VERIFY(!IsExecuting());
			GetCommandQueue().Submit(func);
		}

		template<typename TFunc>
		static void SubmitResourceFree(const TFunc& func)
		{
			GetResourceFreeQueue().Submit(func);
		}

		static void RenderFullScreenQuad(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Material> material);

		static void BeginBatch(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer);
		static void RenderBatch(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Material> material, Ref<ConstantBufferSet> constantBufferSet, uint32_t indexCount, uint32_t startIndex);
		static void EndBatch(Ref<RenderCommandBuffer> renderCommandBuffer);

		static void RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, Ref<ConstantBufferSet> constantBufferSet, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, uint32_t indexCount);
		static void RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, Ref<ConstantBufferSet> constantBufferSet, Ref<VertexBuffer> vertexBuffer, uint32_t vertexCount);

		static void GenerateMips(Ref<Image2D> image);
		static void RT_GenerateMips(Ref<Image2D> image);

		static void ClearAllCommandBuffers();
		static const RendererCapabilities& GetCapabilities();

		static Ref<ShaderLibrary> GetShaderLib();
		static Ref<Texture2D> GetWhiteTexture();

		static bool IsInsideFrame();

		static Ref<RendererAPI> GetRendererAPI();

		static void SetAPI(RendererAPIType api);
		static RendererAPIType GetAPI();

	private:
		static CommandQueue& GetCommandQueue();
		static CommandQueue& GetResourceFreeQueue();

	private:
		static Ref<RendererAPI> s_RendererAPI;
		friend class DirectXRenderer;
	};

}
