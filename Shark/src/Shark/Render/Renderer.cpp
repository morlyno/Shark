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

	void Renderer::Submit(Ref<VertexBuffer>& vertexbuffer, Ref<IndexBuffer>& indexbuffer, Ref<Shaders>& shaders, const DirectX::XMMATRIX& translation)
	{
		vertexbuffer->Bind();
		indexbuffer->Bind();
		shaders->Bind();
		shaders->UploudData("SceanData",ShaderType::VertexShader, m_SceanData, sizeof(SceanData));
		shaders->UploudData("ObjectData", ShaderType::VertexShader, (void*)&translation, sizeof(translation));

		RendererCommand::DrawIndexed(indexbuffer->GetCount());
	}

	void Renderer::Submit(Ref<VertexBuffer>& vertexbuffer, Ref<IndexBuffer>& indexbuffer, Ref<Shaders>& shaders, Ref<Texture> texture, const DirectX::XMMATRIX& translation)
	{
		vertexbuffer->Bind();
		indexbuffer->Bind();
		shaders->Bind();
		texture->Bind();
		shaders->UploudData("SceanData", ShaderType::VertexShader, m_SceanData, sizeof(SceanData));
		shaders->UploudData("ObjectData", ShaderType::VertexShader, (void*)&translation, sizeof(translation));

		RendererCommand::DrawIndexed(indexbuffer->GetCount());
	}

}