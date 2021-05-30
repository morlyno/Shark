#include "skpch.h"
#include "Renderer.h"

#include "Shark/Render/RendererCommand.h"
#include "Shark/Render/Renderer2D.h"

namespace Shark {

	static ShaderLibrary s_ShaderLib;

	void Renderer::Init()
	{
		RendererCommand::Init();
		Renderer2D::Init();
	}

	void Renderer::ShutDown()
	{
		s_ShaderLib.Clear();
		Renderer2D::ShutDown();
		RendererCommand::ShutDown();
	}

	ShaderLibrary& Renderer::ShaderLib()
	{
		return s_ShaderLib;
	}

}