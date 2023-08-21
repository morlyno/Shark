#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/CommandQueue.h"

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

namespace Shark {

	struct RendererCapabilities
	{
		uint32_t MaxMipLeves;
		uint32_t MaxAnisotropy;

		float MinLODBias;
		float MaxLODBias;
	};

	class RendererAPI : public RefCount
	{
	public:
		virtual ~RendererAPI() = default;

		virtual void Init() = 0;
		virtual void ShutDown() = 0;
		
		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;

		virtual TimeStep GetGPUTime() const = 0;

		virtual void RenderFullScreenQuad(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Material> material) = 0;
		
		virtual void BeginBatch(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer) = 0;
		virtual void RenderBatch(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Material> material, uint32_t indexCount, uint32_t startIndex) = 0;
		virtual void EndBatch(Ref<RenderCommandBuffer> renderCommandBuffer) = 0;

		virtual void RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, uint32_t indexCount) = 0;
		virtual void RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, Ref<VertexBuffer> vertexBuffer, uint32_t vertexCount) = 0;;

		virtual void RenderMesh(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Mesh> mesh, Ref<Pipeline> pipeline) = 0;
		virtual void RenderSubmesh(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Mesh> mesh, uint32_t submeshIndex, Ref<Pipeline> pipeline) = 0;
		virtual void RenderSubmesh(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Mesh> mesh, uint32_t submeshIndex, Ref<ConstantBuffer> sceneData, Ref<ConstantBuffer> meshData) = 0;

		virtual void GenerateMips(Ref<Image2D> image) = 0;
		virtual void RT_GenerateMips(Ref<Image2D> image) = 0;

		virtual const RendererCapabilities& GetCapabilities() const = 0;

		virtual Ref<ShaderLibrary> GetShaderLib() = 0;
		virtual Ref<Texture2D> GetWhiteTexture() = 0;

		virtual bool ResourcesCreated() const = 0;
		virtual bool IsInsideFrame() const = 0;

	};

}