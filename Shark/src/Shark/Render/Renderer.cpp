#include "skpch.h"
#include "Renderer.h"
#include "RendererCommand.h"

namespace Shark {

	Renderer Renderer::s_Instants;

	void Renderer::BeginScean()
	{
	}

	void Renderer::EndScean()
	{
	}

	void Renderer::Submit( std::unique_ptr<VertexBuffer>& vertexbuffer,std::unique_ptr<IndexBuffer>& indexbuffer,std::shared_ptr<Shaders>& shaders )
	{
		vertexbuffer->Bind();
		indexbuffer->Bind();
		shaders->Bind();

		RendererCommand::DrawIndexed( indexbuffer->GetCount() );
	}

}