#include "skpch.h"
#include "Renderer.h"
#include "RendererCommand.h"

#include "Platform/DirectX11/DirectXRendererAPI.h"

namespace Shark {

	Renderer::SceanData* Renderer::m_SceanData = new Renderer::SceanData();

	void Renderer::BeginScean(OrtographicCamera& camera)
	{
		m_SceanData->ViewProjectionMatrix = camera.GetViewProjection();
	}

	void Renderer::EndScean()
	{
	}

	void Renderer::Submit(std::unique_ptr<VertexBuffer>& vertexbuffer,std::unique_ptr<IndexBuffer>& indexbuffer,std::shared_ptr<Shaders>& shaders)
	{
		vertexbuffer->Bind();
		indexbuffer->Bind();
		shaders->Bind();
		shaders->SetSceanData(ShaderType::VertexShader, 0u, m_SceanData, sizeof(SceanData));

		RendererCommand::DrawIndexed(indexbuffer->GetCount());
	}

}