#pragma once

#include "Shark/Render/ShaderCompiler/Common.h"

namespace Shark {

	class PlatformShaderCompiler
	{
	public:
		virtual ~PlatformShaderCompiler() = default;

		virtual bool Reload(const ShaderInfo& info, nvrhi::ShaderType compiledStages, CompilerResult& result) = 0;
	};

}
