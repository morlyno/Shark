#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/ShaderCompiler/Common.h"
#include "Shark/Render/ShaderCompiler/D3D11ShaderCompiler.h"

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

		const auto& GetSpirvBinaries() const { return m_SpirvBinary; }

	private:
		void Preprocess();
		bool CompileStage(nvrhi::ShaderType stage);
		std::string HLSLCompileStage(nvrhi::ShaderType stage);
		std::string GLSLCompileStage(nvrhi::ShaderType stage);
		void Reflect();
		void ReflectStage(nvrhi::ShaderType stage);
		void MapBindings();
		void BuildCombinedImageSampler();
		ShaderInputInfo* FindInputInfo(const std::string& name);

	private:
		CompilerOptions m_Options;

		ShaderInfo m_Info;
		std::string m_Source;

		std::map<nvrhi::ShaderType, ShaderSourceInfo> m_SourceInfo;
		std::map<nvrhi::ShaderType, std::vector<uint32_t>> m_SpirvBinary;
		nvrhi::ShaderType m_CompiledStages = nvrhi::ShaderType::None;

		std::vector<std::pair<std::string, std::string>> m_CombinedImageSampler;
		ShaderReflection m_Reflection;

		//std::map<GraphicsBinding, ShaderReflection::BindingItem*> m_ShaderResources;

		D3D11ShaderCompilerInterface m_D3D11Compiler;
	};

}
