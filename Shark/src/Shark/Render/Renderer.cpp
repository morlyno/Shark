#include "skpch.h"
#include "Renderer.h"
#include "Shark/Core/Window.h"
#include "Shark/Render/Renderer2D.h"

#include "Platform/DirectX11/DirectXRendererAPI.h"

namespace Shark {

	Scope<Renderer::SceanData> Renderer::m_SceanData = CreateScope<Renderer::SceanData>();

	void Renderer::Init(const Window& window)
	{
		RendererCommand::Init(window);
		Renderer2D::Init();
	}

	void Renderer::ShutDown()
	{
		Renderer2D::ShutDown();
		RendererCommand::ShutDown();
	}

	void Renderer::BeginScean(OrtographicCamera& camera)
	{
		m_SceanData->ViewProjectionMatrix = camera.GetViewProjection();
	}

	void Renderer::EndScean()
	{
	}

	void Renderer::Submit(Ref<VertexBuffer>& vertexbuffer, Ref<IndexBuffer>& indexbuffer, Ref<Shaders>& shaders, const DirectX::XMMATRIX& translation)
	{
		vertexbuffer->Bind();
		indexbuffer->Bind();
		shaders->Bind();
		shaders->SetBuffer("SceanData", m_SceanData.get(), sizeof(SceanData));
		shaders->SetBuffer("ObjectData", (void*)&translation, sizeof(translation));

		RendererCommand::DrawIndexed(indexbuffer->GetCount());
	}

	void Renderer::Submit(Ref<VertexBuffer>& vertexbuffer, Ref<IndexBuffer>& indexbuffer, Ref<Shaders>& shaders, Ref<Texture> texture, const DirectX::XMMATRIX& translation)
	{
		vertexbuffer->Bind();
		indexbuffer->Bind();
		shaders->Bind();
		texture->Bind();
		shaders->SetBuffer("SceanData", m_SceanData.get(), sizeof(SceanData));
		shaders->SetBuffer("ObjectData", (void*)&translation, sizeof(translation));

		RendererCommand::DrawIndexed(indexbuffer->GetCount());
	}

}