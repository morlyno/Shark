#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/ShaderCompiler/Common.h"
#include "Shark/Render/ShaderCompiler/PlatformShaderCompiler.h"

namespace Shark::Vulkan {

	class ShaderCompiler : public PlatformShaderCompiler
	{
	public:
		ShaderCompiler(const CompilerOptions& options);

		virtual bool Reload(const ShaderInfo& info, nvrhi::ShaderType compiledStages, CompilerResult& result) override;

	private:
		bool CompileStage(const ShaderInfo& info, nvrhi::ShaderType stage, CompilerResult& result);

	private:
		const CompilerOptions& m_Options;

	};

}
