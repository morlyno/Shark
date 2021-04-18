#pragma once

#include "Shark/Core/Base.h"
#include "RendererCommand.h"

#include "Shark/Render/Buffers.h"
#include "Shark/Render/Shaders.h"
#include "Shark/Render/Texture.h"
#include "Shark/Render/FrameBuffer.h"

#include <DirectXMath.h>

namespace Shark {

	class Renderer
	{
	public:
		static void Init();
		static void ShutDown();

		static void BeginScean(/*OrtographicCamera& camera*/);
		static void EndScean();

		static void Submit(Ref<VertexBuffer>& vertexbuffer, Ref<IndexBuffer>& indexbuffer, Ref<Shaders>& shaders, const DirectX::XMMATRIX& translation);
		static void Submit(Ref<VertexBuffer>& vertexbuffer, Ref<IndexBuffer>& indexbuffer, Ref<Shaders>& shaders, Ref<Texture> texture, const DirectX::XMMATRIX& translation);

		static void ClearFrameBuffer(const Ref<FrameBuffer>& framebuffer, const Buffer& cleardata);
	};

}
