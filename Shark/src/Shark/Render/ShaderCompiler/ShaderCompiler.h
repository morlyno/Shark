#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/ShaderCompiler/Common.h"
#include "Shark/Render/ShaderCompiler/D3D11ShaderCompiler.h"

struct IDxcCompiler3;
struct IDxcUtils;

namespace spirv_cross {
	struct Resource;
	class Compiler;
}

namespace Shark {

	class ShaderCompiler : public RefCount
	{
	public:
		ShaderCompiler(const std::filesystem::path& sourcePath, const CompilerOptions& options);
		static Ref<ShaderCompiler> Load(const std::filesystem::path& sourcePath, const CompilerOptions& options);

		bool Reload();
		void ClearState();

		const ShaderInfo& GetInfo() const { return m_Info; }
		bool HasStage(nvrhi::ShaderType stage) { return m_SpirvBinary.contains(stage); }
		std::span<const uint32_t> GetBinary(nvrhi::ShaderType stage) const { return m_SpirvBinary.at(stage); }
		const ShaderReflectionData& GetRelfectionData() const { return m_ReflectionData; }

		const D3D11ShaderCompilerInterface& GetD3D11Compiler() const { return m_D3D11Compiler; }
	private:
		void Preprocess();
		bool CompileStage(nvrhi::ShaderType stage);
		std::string HLSLCompileStage(nvrhi::ShaderType stage);
		std::string GLSLCompileStage(nvrhi::ShaderType stage);
		void Reflect();
		void ReflectStage(nvrhi::ShaderType stage);
		void BuildNameCache();

		void ReflectMembers(const spirv_cross::Resource& resource, ShaderReflection::MemberList& memberList);
		ShaderReflection::Resource& ReflectResource(const spirv_cross::Resource& resource);

	private:
		CompilerOptions m_Options;

		ShaderInfo m_Info;
		std::string m_Source;

		std::map<nvrhi::ShaderType, ShaderSourceInfo> m_SourceInfo;
		std::map<nvrhi::ShaderType, std::vector<uint32_t>> m_SpirvBinary;
		nvrhi::ShaderType m_CompiledStages = nvrhi::ShaderType::None;

		ShaderReflectionData m_ReflectionData;

		// Current state for shader reflection
		nvrhi::ShaderType m_CurrentStage = nvrhi::ShaderType::None;
		spirv_cross::Compiler* m_CurrentCompiler = nullptr;

		D3D11ShaderCompilerInterface m_D3D11Compiler;

		friend class D3D11ShaderCompilerInterface;
	};

}
