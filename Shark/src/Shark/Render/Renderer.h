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

		static void RenderFullScreenQuad(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Material> material);

		static void RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, Ref<ConstantBufferSet> constantBufferSet, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, uint32_t indexCount);
		static void RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, Ref<ConstantBufferSet> constantBufferSet, Ref<VertexBuffer> vertexBuffer, uint32_t vertexCount);


		static Ref<ShaderLibrary> GetShaderLib();
		static Ref<Texture2D> GetWhiteTexture();
		static Ref<GPUTimer> GetPresentTimer();

		static Ref<RendererAPI> GetRendererAPI();
	};

}
