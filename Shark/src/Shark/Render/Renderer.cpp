#include "skpch.h"
#include "Renderer.h"

#include "Shark/Render/RendererCommand.h"
#include "Shark/Render/Renderer2D.h"
#include "Shark/Render/TestRenderer.h"

namespace Shark {

	struct RendererBaseData
	{
		ShaderLibrary ShaderLib;
		Ref<Texture2D> WidthTexture;
	};
	static RendererBaseData* s_BaseData = nullptr;

	void Renderer::Init()
	{
		RendererCommand::Init();

		s_BaseData = new RendererBaseData;
		s_BaseData->ShaderLib.Load("assets/Shaders/MainShader.hlsl");
		s_BaseData->ShaderLib.Load("assets/Shaders/TestShader.hlsl");
		uint32_t color = 0xFFFFFFFF;
		s_BaseData->WidthTexture = Texture2D::Create(1, 1, &color);

		Renderer2D::Init();
		TestRenderer::Init();
	}

	void Renderer::ShutDown()
	{
		TestRenderer::ShutDown();
		Renderer2D::ShutDown();

		delete s_BaseData;

		RendererCommand::ShutDown();
	}

	ShaderLibrary& Renderer::GetShaderLib()
	{
		return s_BaseData->ShaderLib;
	}

	Ref<Texture2D> Renderer::GetWidthTexture()
	{
		return s_BaseData->WidthTexture;
	}

	Ref<Shaders> Renderer::GetStandartShader()
	{
		return s_BaseData->ShaderLib.Get("TestShader");
	}

}