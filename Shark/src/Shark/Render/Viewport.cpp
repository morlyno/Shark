#include "skpch.h"
#include "Viewport.h"

#include "Shark/Render/RendererCommand.h"

namespace Shark {

	Ref<Viewport> Viewport::Create(uint32_t width, uint32_t height)
	{
		return RendererCommand::CreateViewport(width, height);
	}

}