#pragma once

#include "Shark/Core/Base.h"

#include "Shark/Render/RenderCommandBuffer.h"
#include "Shark/Render/FrameBuffer.h"
#include "Shark/Render/Shader.h"
#include "Shark/Render/ConstantBuffer.h"
#include "Shark/Render/Texture.h"
#include "Shark/Render/Buffers.h"
#include "Shark/Render/Pipeline.h"

namespace Shark {

	enum class PrimitveTopology
	{
		Triangle, Line, Dot
	};

	class RendererAPI : public RefCount
	{
	public:
		enum class API
		{
			None = 0, DirectX11 = 1
		};
	public:
		virtual ~RendererAPI() = default;

		virtual void Init() = 0;
		virtual void ShutDown() = 0;
		
		virtual void RenderFullScreenQuad(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Image2D> image) = 0;
		virtual void RenderFullScreenQuadWidthDepth(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Image2D> image, Ref<Image2D> depthImage) = 0;
		
		virtual void RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<FrameBuffer> frameBuffer, Ref<Shader> shaders, Ref<ConstantBufferSet> constantBufferSet, Ref<Texture2DArray> textureArray, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, uint32_t indexCount, PrimitveTopology topology) = 0;
		virtual void RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<ConstantBufferSet> constantBufferSet, Ref<Texture2DArray> textureArray, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, uint32_t indexCount, PrimitveTopology topology) = 0;
		virtual void RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<ConstantBufferSet> constantBufferSet, Ref<Texture2DArray> textureArray, Ref<VertexBuffer> vertexBuffer, uint32_t vertexCount, PrimitveTopology topology) = 0;

		virtual Ref<ShaderLibrary> GetShaderLib() = 0;
		virtual Ref<Texture2D> GetWhiteTexture() = 0;

		virtual Ref<FrameBuffer> GetFinaleCompositFrameBuffer() const = 0;

		virtual void ResizeSwapChain(uint32_t width, uint32_t height) = 0;
		virtual void SwapBuffers(bool vsync) = 0;
		virtual void BindMainFrameBuffer() = 0;

		// Temp
		virtual void SetBlendForImgui(bool blend) = 0;

		static API GetAPI() { return s_API; }
		static void SetAPI(API api) { s_API = api; }

		static Ref<RendererAPI> Create();
	private:
		static API s_API;
	};

}