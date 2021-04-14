#include "skpch.h"
#include "Renderer.h"
#include "Shark/Render/Renderer2D.h"

#include "Platform/DirectX11/DirectXRendererAPI.h"

namespace Shark {

	Scope<Renderer::SceanData> Renderer::m_SceanData = Scope<Renderer::SceanData>::Create();

	struct ClearData
	{
		Ref<VertexBuffer> VertexBuffer;
		Ref<IndexBuffer> IndexBuffer;
	};
	static ClearData s_ClearData;

	static void InitClearData()
	{
		VertexLayout layout({ { VertexDataType::Int, "VertexIndex" } });
		int vertices[4] = { 0, 1, 2, 3 };
		s_ClearData.VertexBuffer = VertexBuffer::Create(layout, Buffer::Ref(vertices));
		int indices[4] = { 0, 1, 2, 4 };
		s_ClearData.IndexBuffer = IndexBuffer::Create(Buffer::Ref(indices));
	}

	static void ShutdownClearData()
	{
		s_ClearData.IndexBuffer.Release();
		s_ClearData.VertexBuffer.Release();
	}

	void Renderer::Init()
	{
		RendererCommand::Init();
		Renderer2D::Init();
		InitClearData();
	}

	void Renderer::ShutDown()
	{
		ShutdownClearData();
		Renderer2D::ShutDown();
		RendererCommand::ShutDown();
	}

	void Renderer::BeginScean(/*OrtographicCamera& camera*/)
	{
		/*m_SceanData->ViewProjectionMatrix = camera.GetViewProjection();*/
	}

	void Renderer::EndScean()
	{
	}

	void Renderer::Submit(Ref<VertexBuffer>& vertexbuffer, Ref<IndexBuffer>& indexbuffer, Ref<Shaders>& shaders, const DirectX::XMMATRIX& translation)
	{
		vertexbuffer->Bind();
		indexbuffer->Bind();
		shaders->Bind();
		shaders->SetBuffer("SceanData", Buffer::Ref(*m_SceanData));
		shaders->SetBuffer("ObjectData", Buffer::Ref(translation));

		RendererCommand::DrawIndexed(indexbuffer->GetCount());
	}

	void Renderer::Submit(Ref<VertexBuffer>& vertexbuffer, Ref<IndexBuffer>& indexbuffer, Ref<Shaders>& shaders, Ref<Texture> texture, const DirectX::XMMATRIX& translation)
	{
		vertexbuffer->Bind();
		indexbuffer->Bind();
		shaders->Bind();
		texture->Bind();
		shaders->SetBuffer("SceanData", Buffer::Ref(*m_SceanData));
		shaders->SetBuffer("ObjectData", Buffer::Ref(translation));

		RendererCommand::DrawIndexed(indexbuffer->GetCount());
	}

	void Renderer::ClearFrameBuffer(const Ref<FrameBuffer>& framebuffer, Ref<Shaders> clearshader, const Buffer& cleardata)
	{
		framebuffer->Bind();
		s_ClearData.VertexBuffer->Bind();
		s_ClearData.IndexBuffer->Bind();
		clearshader->Bind();
		//clearshader->SetBuffer("ClearData", cleardata);
		RendererCommand::DrawIndexed(s_ClearData.IndexBuffer->GetCount());
		clearshader->UnBind();
		s_ClearData.IndexBuffer->Bind();
		s_ClearData.VertexBuffer->UnBind();
	}

}