#pragma once

#include "Shark/Render/ShaderCompiler/Common.h"
#include "Shark/Render/ShaderCompiler/PlatformShaderCompiler.h"

namespace Shark::D3D11 {

	class ShaderCompiler : public PlatformShaderCompiler
	{
	public:
		ShaderCompiler(const CompilerOptions& options);

		virtual bool Reload(const ShaderInfo& info, nvrhi::ShaderType compiledStages, CompilerResult& result) override;

	private:
		bool CompileStage(const ShaderInfo& info, nvrhi::ShaderType stage, CompilerResult& result);
		void CrossCompile(const ShaderInfo& info, nvrhi::ShaderType stage, CompilerResult& result);
		std::string CompileHLSL(const ShaderInfo& info, nvrhi::ShaderType stage, CompilerResult& result);

	private:
		const CompilerOptions& m_Options;

		std::map<nvrhi::ShaderType, std::string> m_HLSLSource;
	};

}
