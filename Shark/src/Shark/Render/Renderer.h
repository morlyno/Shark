#pragma once

#include "Shark/Core/Core.h"
#include "Shark/Render/Buffers.h"
#include "Shark/Render/Shaders.h"

namespace Shark {

	class Renderer
	{
	public:
		static void BeginScean();
		static void EndScean();

		static void Submit( std::unique_ptr<VertexBuffer>& vertexbuffer,std::unique_ptr<IndexBuffer>& indexbuffer,std::shared_ptr<Shaders>& shaders );
	private:
		static Renderer s_Instants;
	};

}