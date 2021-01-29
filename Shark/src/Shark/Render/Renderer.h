#pragma once

#include "Shark/Core/Base.h"
#include "RendererCommand.h"

#include "Shark/Render/Buffers.h"
#include "Shark/Render/Shaders.h"
#include "Shark/Render/Texture.h"
#include "Shark/Render/OrtographicCamera.h"

#include <DirectXMath.h>

namespace Shark {

	class Renderer
	{
	public:
		static void Init(const class Window& window);
		static void ShutDown();

		static void BeginScean(OrtographicCamera& camera);
		static void EndScean();

		static void Submit(Ref<VertexBuffer>& vertexbuffer, Ref<IndexBuffer>& indexbuffer, Ref<Shaders>& shaders, const DirectX::XMMATRIX& translation);
		static void Submit(Ref<VertexBuffer>& vertexbuffer, Ref<IndexBuffer>& indexbuffer, Ref<Shaders>& shaders, Ref<Texture> texture, const DirectX::XMMATRIX& translation);
	private:
		struct SceanData
		{
			DirectX::XMMATRIX ViewProjectionMatrix;
		};
		static Scope<SceanData> m_SceanData;
	};

}
