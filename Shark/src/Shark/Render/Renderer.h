#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/RendererCommand.h"
#include "Shark/Render/Shaders.h"
#include "Shark/Render/Texture.h"
#include "Shark/Render/RenderCommandBuffer.h"
#include "Shark/Render/Buffers.h"
#include "Shark/Render/ConstantBuffer.h"
#include "Shark/Render/Shaders.h"
#include "Shark/Render/RenderCommandBuffer.h"

namespace Shark {

	class Renderer
	{
	public:
		static void Init();
		static void ShutDown();

		static void SubmitFullScreenQuad();

		static void SubmitGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Shaders> shaders, Ref<ConstantBufferSet> constantBufferSet, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, uint32_t indexCount, PrimitveTopology topology);

		static ShaderLibrary& GetShaderLib();
		static Ref<Texture2D> GetWhiteTexture();

	};

}
