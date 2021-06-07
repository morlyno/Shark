#include "skpch.h"
#include "Renderer.h"

#include "Shark/Render/RendererCommand.h"
#include "Shark/Render/Renderer2D.h"
#include "Shark/Render/TestRenderer.h"

namespace Shark {

	struct RendererBaseData
	{
		ShaderLibrary ShaderLib;
		Ref<Shaders> Default2DShader;
		Ref<Texture2D> WhiteTexture;
		Ref<Material> Default2DMaterial;
	};
	static RendererBaseData* s_BaseData = nullptr;

	void Renderer::Init()
	{
		RendererCommand::Init();

		s_BaseData = new RendererBaseData;
		uint32_t color = 0xFFFFFFFF;
		s_BaseData->WhiteTexture = Texture2D::Create(1, 1, &color);
		s_BaseData->Default2DShader = Shaders::Create("assets/Shaders/MainShader.hlsl");
		s_BaseData->Default2DMaterial = Material::Create(s_BaseData->Default2DShader, "Default2DMaterial");

		s_BaseData->ShaderLib.Add(s_BaseData->Default2DShader);
		s_BaseData->ShaderLib.Load("assets/Shaders/TestShader.hlsl");

		Renderer2D::Init();
	}

	void Renderer::ShutDown()
	{
		Renderer2D::ShutDown();

		delete s_BaseData;

		RendererCommand::ShutDown();
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
		return s_BaseData->Default2DShader;
	}

	Ref<Material> Renderer::GetDefault2DMaterial()
	{
		return s_BaseData->Default2DMaterial;
	}

}