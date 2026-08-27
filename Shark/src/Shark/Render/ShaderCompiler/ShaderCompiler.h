#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Enum.h"
#include "Shark/Render/ShaderCompiler/Common.h"
#include "Shark/Render/ShaderCompiler/ShaderPreprocessor.h"

#include <memory>

namespace Shark {
	class PlatformShaderCompiler;
	class HLSLIncludeHandler;
}

namespace Shark {

	struct PreprocessedResult
	{
		std::string Source;
		StageInfo Info;
	};

	class ShaderCompiler
	{
	public:
		ShaderCompiler(const std::filesystem::path& sourcePath, const CompilerOptions& options);
		~ShaderCompiler();
		//static Ref<ShaderCompiler> Load(const std::filesystem::path& sourcePath, const CompilerOptions& options);

		bool Reload();

		const ShaderInfo& GetInfo() const { return m_Info; }
		std::string GetName() const { return m_Info.SourcePath.stem().string(); }
		const CompilerResult& GetResult() const { return *m_Result; }
		void GetResult(Scope<CompilerResult>& result);

	private:
		bool Preprocess(bool processAll);
		bool PreprocessStage(nvrhi::ShaderType stage);
		bool CompileStage(nvrhi::ShaderType stage);
		std::string HLSLCompileStage(nvrhi::ShaderType stage);
		void Reflect();
		void ReflectStage(nvrhi::ShaderType stage);
		void BuildCombinedImageSampler(const std::string& imageName, const std::string& samplerName);

		ShaderInputInfo* FindInputInfo(const std::string& name);

	private:
		CompilerOptions m_Options;
		ShaderInfo m_Info;
		Enum::Flags<nvrhi::ShaderType> m_CompiledStages = nvrhi::ShaderType::None;

		ShaderPreprocessor m_Preprocessor;
		std::shared_ptr<HLSLIncludeHandler> m_IncludeHandler;

		std::map<nvrhi::ShaderType, PreprocessedResult> m_PreprocessedResult;

		Scope<CompilerResult> m_Result;
		std::vector<Scope<PlatformShaderCompiler>> m_PlatformCompilers;
	};

}
