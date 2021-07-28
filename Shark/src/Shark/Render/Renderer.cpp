#include "skpch.h"
#include "Renderer.h"

#include "Shark/Render/RendererCommand.h"
#include "Shark/Render/Renderer2D.h"
#include "Shark/Render/Buffers.h"

namespace Shark {

	struct RendererBaseData
	{
		ShaderLibrary ShaderLib;
		Ref<Texture2D> WhiteTexture;
		Ref<Material> Default2DMaterial;

		Ref<VertexBuffer> QuadVB;
		Ref<IndexBuffer> QuadIB;
	};
	static RendererBaseData* s_BaseData = nullptr;

	void Renderer::Init()
	{
		RendererCommand::Init();

		s_BaseData = new RendererBaseData;
		uint32_t color = 0xFFFFFFFF;

		s_BaseData->ShaderLib.Load("assets/Shaders/TestShader.hlsl");
		auto mainshader = s_BaseData->ShaderLib.Load("assets/Shaders/MainShader.hlsl");
		s_BaseData->ShaderLib.Load("assets/Shaders/FullScreen.hlsl");
		s_BaseData->ShaderLib.Load("assets/Shaders/NegativeEffect.hlsl");
		s_BaseData->ShaderLib.Load("assets/Shaders/BlurEffect.hlsl");
		
		s_BaseData->WhiteTexture = Texture2D::Create(1, 1, &color);
		s_BaseData->Default2DMaterial = Material::Create(mainshader, "Default2DMaterial");

		float vertices[4 * 2] = {
			-1.0f,  1.0f,
			 1.0f,  1.0f,
			 1.0f, -1.0f,
			-1.0f, -1.0f
		};
		uint32_t indices[3 * 2] = {
			0, 1, 2,
			2, 3, 0
		};
		VertexLayout layout = {
			{ VertexDataType::Float2, "Position" }
		};

		s_BaseData->QuadVB = VertexBuffer::Create(layout, vertices, sizeof(vertices));
		s_BaseData->QuadIB = IndexBuffer::Create(indices, std::size(indices));

		Renderer2D::Init();
	}

	void Renderer::ShutDown()
	{
		Renderer2D::ShutDown();

		delete s_BaseData;

		RendererCommand::ShutDown();
	}

	void Renderer::SubmitFullScreenQuad()
	{
		s_BaseData->QuadIB->Bind();
		s_BaseData->QuadVB->Bind();
		RendererCommand::DrawIndexed(s_BaseData->QuadIB->GetCount());
	}

	ShaderLibrary& Renderer::GetShaderLib()
	{
		return s_BaseData->ShaderLib;
	}

	Ref<Texture2D> Renderer::GetWhiteTexture()
	{
		return s_BaseData->WhiteTexture;
	}

	Ref<Shaders> Renderer::GetDefault2DShader()
	{
		return s_BaseData->ShaderLib.Get("MainShader");
	}

	Ref<Material> Renderer::GetDefault2DMaterial()
	{
		return s_BaseData->Default2DMaterial;
	}

}