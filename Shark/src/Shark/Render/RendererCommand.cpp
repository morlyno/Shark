#include "skpch.h"
#include "RendererCommand.h"

namespace Shark {

	std::unique_ptr<RendererAPI> RendererCommand::s_RendererAPI = RendererAPI::Create();

}