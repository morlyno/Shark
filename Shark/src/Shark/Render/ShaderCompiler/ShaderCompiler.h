#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/ShaderCompiler/Common.h"
#include "Shark/Render/ShaderCompiler/D3D11ShaderCompiler.h"
#include "Shark/Render/ShaderCompiler/ShaderPreprocessor.h"
#include "Shark/Render/ShaderCompiler/HLSLIncludeHandler.h"

namespace Shark {

	class ShaderCompiler : public RefCount
	{
	public:
		ShaderCompiler(const std::filesystem::path& sourcePath, const CompilerOptions& options);
		static Ref<ShaderCompiler> Load(const std::filesystem::path& sourcePath, const CompilerOptions& options);

		bool Reload();
		void ClearState();

		bool HasStage(nvrhi::ShaderType stage) { return m_SpirvBinary.contains(stage); }
		const ShaderInfo& GetInfo() const { return m_Info; }
		const ShaderSourceInfo& GetSourceInfo(nvrhi::ShaderType stage) const { return m_SourceInfo.at(stage); }
		nvrhi::ShaderType GetCompiledStages() const { return m_CompiledStages; }

		Buffer GetBinary(nvrhi::ShaderType stage, nvrhi::GraphicsAPI api) const;
		const ShaderReflection& GetReflectionData() const { return m_Reflection; }
		const auto& GetRequestedBindingSets() const { return m_RequestedBindingSets; }
		LayoutShareMode GetLayoutMode() const { return m_LayoutMode; }

		const auto& GetSpirvBinaries() const { return m_SpirvBinary; }

	private:
		void SetupDXC();

		bool Preprocess();
		bool CompileStage(nvrhi::ShaderType stage);
		std::string HLSLCompileStage(nvrhi::ShaderType stage);
		void Reflect();
		void ReflectStage(nvrhi::ShaderType stage);
		void MapBindings();
		void ProcessCompilerInstructions();
		void BuildCombinedImageSampler(const std::string& arg0, const std::string& arg1);
		ShaderInputInfo* FindInputInfo(const std::string& name);

	private:
		CompilerOptions m_Options;
		ShaderInfo m_Info;

		Scope<HLSLIncludeHandler> m_IncludeHandler;
		ShaderPreprocessor m_Preprocessor;

		std::map<nvrhi::ShaderType, ShaderSourceInfo> m_SourceInfo;
		std::map<nvrhi::ShaderType, std::vector<uint32_t>> m_SpirvBinary;
		nvrhi::ShaderType m_CompiledStages = nvrhi::ShaderType::None;

		ShaderReflection m_Reflection;
		std::vector<std::string> m_RequestedBindingSets;
		LayoutShareMode m_LayoutMode = LayoutShareMode::Default;

		D3D11ShaderCompilerInterface m_D3D11Compiler;
	};

}
