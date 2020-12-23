#pragma once

#include "Shark/Core/Core.h"
#include "Shark/Render/Buffers.h"
#include "Shark/Render/Shaders.h"
#include "Shark/Render/OrtographicCamera.h"

#include <DirectXMath.h>

namespace Shark {

	class Renderer
	{
	public:
		static void BeginScean(OrtographicCamera& camera);
		static void EndScean();

		static void Submit(std::unique_ptr<VertexBuffer>& vertexbuffer, std::unique_ptr<IndexBuffer>& indexbuffer, std::shared_ptr<Shaders>& shaders);
	private:
		struct SceanData
		{
			DirectX::XMMATRIX ViewProjectionMatrix;
		};
		static SceanData* m_SceanData;
	};

}