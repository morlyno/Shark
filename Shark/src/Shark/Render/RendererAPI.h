#pragma once

#include "Shark/Core/Base.h"

#include "Shark/Render/RenderCommandBuffer.h"
#include "Shark/Render/FrameBuffer.h"
#include "Shark/Render/Shader.h"
#include "Shark/Render/ConstantBuffer.h"
#include "Shark/Render/Texture.h"
#include "Shark/Render/Mesh.h"
#include "Shark/Render/Buffers.h"
#include "Shark/Render/Pipeline.h"
#include "Shark/Render/Material.h"
#include "Shark/Render/SwapChain.h"
#include "Shark/Render/RenderPass.h"

namespace Shark {

	struct RendererCapabilities
	{
		uint32_t MaxMipLeves;
		uint32_t MaxAnisotropy;

		float MinLODBias;
		float MaxLODBias;
	};

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

	class RendererAPI : public RefCount
	{
	public:
		virtual ~RendererAPI() = default;

		virtual void Init() = 0;
		virtual void ShutDown() = 0;
		
		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;

		virtual void BeginRenderPass(Ref<RenderCommandBuffer> commandBuffer, Ref<RenderPass> renderPass) = 0;
		virtual void EndRenderPass(Ref<RenderCommandBuffer> commandBuffer, Ref<RenderPass> renderPass) = 0;

		virtual void RenderFullScreenQuad(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Material> material) = 0;
		
		virtual void BeginBatch(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer) = 0;
		virtual void RenderBatch(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Material> material, uint32_t indexCount, uint32_t startIndex) = 0;
		virtual void EndBatch(Ref<RenderCommandBuffer> renderCommandBuffer) = 0;

		virtual void RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, uint32_t indexCount) = 0;
		virtual void RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, Ref<VertexBuffer> vertexBuffer, uint32_t vertexCount) = 0;;

		virtual void RenderCube(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Material> material) = 0;

		virtual void RenderSubmesh(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Mesh> mesh, uint32_t submeshIndex) = 0;
		virtual void RenderSubmeshWithMaterial(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Mesh> mesh, uint32_t submeshIndex, Ref<Material> material) = 0;

		virtual void CopyImage(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> sourceImage, Ref<Image2D> destinationImage) = 0;
		virtual void RT_CopyImage(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> sourceImage, Ref<Image2D> destinationImage) = 0;

		virtual std::pair<Ref<TextureCube>, Ref<TextureCube>> CreateEnvironmentMap(const std::filesystem::path& filepath) = 0;

		virtual void GenerateMips(Ref<Image2D> image) = 0;
		virtual void RT_GenerateMips(Ref<Image2D> image) = 0;

		virtual TimeStep GetGPUTime() const = 0;

		virtual uint32_t GetCurrentFrameIndex() const = 0;
		virtual uint32_t RT_GetCurrentFrameIndex() const = 0;
		virtual Ref<RenderCommandBuffer> GetCommandBuffer() const = 0;

		virtual bool ResourcesCreated() const = 0;
		virtual const RendererCapabilities& GetCapabilities() const = 0;

	public:
		static void SetAPI(RendererAPIType api) { s_CurrentAPI = api; }
		static RendererAPIType GetCurrentAPI() { return s_CurrentAPI; }

		static Ref<RendererAPI> Create();

	public:
		inline static RendererAPIType s_CurrentAPI;

	};

}