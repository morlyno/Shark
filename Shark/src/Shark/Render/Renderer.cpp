#include "skpch.h"
#include "Renderer.h"

#include "Shark/Render/RendererCommand.h"
#include "Shark/Render/Renderer2D.h"

namespace Shark {

	void Renderer::Init()
	{
		RendererCommand::Init();
		Renderer2D::Init();
	}

	void Renderer::ShutDown()
	{
		Renderer2D::ShutDown();
		RendererCommand::ShutDown();
	}

}