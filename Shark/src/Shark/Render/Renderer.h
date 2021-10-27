#pragma once

#include "Shark/Core/Base.h"

#include "Shark/Render/RendererAPI.h"

#include "Shark/Render/Buffers.h"
#include "Shark/Render/Shader.h"
#include "Shark/Render/Texture.h"
#include "Shark/Render/RenderCommandBuffer.h"
#include "Shark/Render/ConstantBuffer.h"
#include "Shark/Render/FrameBuffer.h"

namespace Shark {

	class Renderer
	{
	public:
		static void Init();
		static void ShutDown();

		static void BeginRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<FrameBuffer> framebuffer);
		static void EndRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer);

		static void SubmitFullScreenQuad();

		static void RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<FrameBuffer> frameBuffer, Ref<Shader> shaders, Ref<ConstantBufferSet> constantBufferSet, Ref<Texture2DArray> textureArray, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, uint32_t indexCount, PrimitveTopology topology);

		static Ref<FrameBuffer> GetFinaleCompositFrameBuffer();

		static Ref<RendererAPI> GetRendererAPI();

		static ShaderLibrary& GetShaderLib();
		static Ref<Texture2D> GetWhiteTexture();

	};

}
