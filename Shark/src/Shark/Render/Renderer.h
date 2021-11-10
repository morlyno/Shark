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

namespace Shark {

	class Renderer
	{
	public:
		static void Init();
		static void ShutDown();

		static void NewFrame();

		static void RenderFullScreenQuad(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Image2D> image);
		static void RenderFullScreenQuadWidthDepth(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Image2D> image, Ref<Image2D> depthImage);

		static void RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<FrameBuffer> frameBuffer, Ref<Shader> shaders, Ref<ConstantBufferSet> constantBufferSet, Ref<Texture2DArray> textureArray, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, uint32_t indexCount, PrimitveTopology topology);
		static void RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<ConstantBufferSet> constantBufferSet, Ref<Texture2DArray> textureArray, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, uint32_t indexCount, PrimitveTopology topology);
		static void RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<ConstantBufferSet> constantBufferSet, Ref<Texture2DArray> textureArray, Ref<VertexBuffer> vertexBuffer, uint32_t vertexCount, PrimitveTopology topology);


		static Ref<ShaderLibrary> GetShaderLib();
		static Ref<Texture2D> GetWhiteTexture();
		static Ref<GPUTimer> GetPresentTimer();

		static Ref<FrameBuffer> GetFinaleCompositFrameBuffer();

		static Ref<RendererAPI> GetRendererAPI();
	};

}
