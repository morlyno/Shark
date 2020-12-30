#include "skpch.h"
#include "Renderer.h"
#include "Shark/Core/Window.h"

#include "Platform/DirectX11/DirectXRendererAPI.h"

namespace Shark {

	Renderer::SceanData* Renderer::m_SceanData = nullptr;

	void Renderer::Init(const Window& window)
	{
		RendererCommand::Init(window);
		SK_CORE_ASSERT(m_SceanData == nullptr, "SceanData already set");
		m_SceanData = new Renderer::SceanData();
	}

	void Renderer::ShutDown()
	{
		RendererCommand::ShutDown();
		delete m_SceanData;
	}

	void Renderer::BeginScean(OrtographicCamera& camera)
	{
		m_SceanData->ViewProjectionMatrix = camera.GetViewProjection();
	}

	void Renderer::EndScean()
	{
	}

	void Renderer::Submit(Ref<VertexBuffer>& vertexbuffer, Ref<IndexBuffer>& indexbuffer, Ref<Shaders>& shaders)
	{
		vertexbuffer->Bind();
		indexbuffer->Bind();
		shaders->Bind();
		shaders->SetSceanData(ShaderType::VertexShader, 0u, m_SceanData, sizeof(SceanData));

		RendererCommand::DrawIndexed(indexbuffer->GetCount());
	}

}