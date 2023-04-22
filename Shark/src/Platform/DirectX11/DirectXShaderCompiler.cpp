#include "skpch.h"
#include "DirectXShaderCompiler.h"

#include "Shark/File/FileSystem.h"
#include "Shark/Math/Math.h"
#include "Shark/Utils/String.h"

#include "Platform/DirectX11/DirectXShader.h"

#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_hlsl.hpp>
#include <d3dcompiler.h>

namespace Shark {

	namespace utils {

		static shaderc_shader_kind ToShaderCShaderKind(ShaderUtils::ShaderStage::Type shaderStage)
		{
			switch (shaderStage)
			{
				case ShaderUtils::ShaderStage::Vertex: return shaderc_vertex_shader;
				case ShaderUtils::ShaderStage::Pixel: return shaderc_fragment_shader;
			}

			SK_CORE_ASSERT(false, "Invalid Shader Stage");
			return (shaderc_shader_kind)0;
		}

		static std::string_view ShaderStageExtension(ShaderUtils::ShaderStage::Type stage)
		{
			switch (stage)
			{
				case ShaderUtils::ShaderStage::Vertex: return ".vert";
				case ShaderUtils::ShaderStage::Pixel: return ".pixl";
			}

			SK_CORE_ASSERT(false, "Invalid Shader Stage");
			return "";
		}

		static ShaderUtils::ShaderStage::Type GetShaderStage(const std::string& stage)
		{
			if (String::Compare(stage, "vertex", Case::Ingnore)) return ShaderUtils::ShaderStage::Vertex;
			if (String::Compare(stage, "vert", Case::Ingnore)) return ShaderUtils::ShaderStage::Vertex;


			if (String::Compare(stage, "pixel", Case::Ingnore)) return ShaderUtils::ShaderStage::Pixel;
			if (String::Compare(stage, "frag", Case::Ingnore)) return ShaderUtils::ShaderStage::Pixel;
			if (String::Compare(stage, "fragment", Case::Ingnore)) return ShaderUtils::ShaderStage::Pixel;

			SK_CORE_ASSERT(false, "Unkown Shader Stage");
			return ShaderUtils::ShaderStage::None;
		}

		static ShaderUtils::ShaderLanguage GetLanguageFromFileExtension(const std::filesystem::path& shaderSourcePath)
		{
			auto extension = shaderSourcePath.extension().string();

			if (String::Compare(extension, ".hlsl", Case::Ingnore))
				return ShaderUtils::ShaderLanguage::HLSL;

			if (String::Compare(extension, ".glsl", Case::Ingnore))
				return ShaderUtils::ShaderLanguage::GLSL;

			SK_CORE_ASSERT(false, "Unkown Shader Extension");
			return ShaderUtils::ShaderLanguage::None;
		}

		static shaderc_source_language GetShaderCSourceLanguage(ShaderUtils::ShaderLanguage language)
		{
			switch (language)
			{
				case ShaderUtils::ShaderLanguage::HLSL: return shaderc_source_language_hlsl;
				case ShaderUtils::ShaderLanguage::GLSL: return shaderc_source_language_glsl;
			}

			SK_CORE_ASSERT(false, "Unkown ShaderLanguage");
			return (shaderc_source_language)0;
		}

		static const char* GetShaderVersion(ShaderUtils::ShaderStage::Type stage)
		{
			switch (stage)
			{
				case ShaderUtils::ShaderStage::Vertex: return "vs_4_0";
				case ShaderUtils::ShaderStage::Pixel: return "ps_4_0";
			}

			SK_CORE_ASSERT(false, "Unkown Shader Stage");
			return nullptr;
		}

	}


	DirectXShaderCompiler::DirectXShaderCompiler(const std::filesystem::path& shaderSourcePath, bool disableOptimization)
		: m_ShaderSourcePath(shaderSourcePath)
	{
		m_Language = utils::GetLanguageFromFileExtension(shaderSourcePath);
		m_Options.DisableOptimization = disableOptimization;
	}

	DirectXShaderCompiler::DirectXShaderCompiler(const std::filesystem::path& shaderSourcePath, const DirectXShaderCompilerOptions& options)
		: m_ShaderSourcePath(shaderSourcePath), m_Options(options)
	{
		m_Language = utils::GetLanguageFromFileExtension(shaderSourcePath);
	}

	bool DirectXShaderCompiler::Reload(bool forceCompile)
	{
		m_ShaderSource.clear();
		m_SPIRVData.clear();
		m_HLSLShaderSource.clear();
		m_ShaderBinary.clear();

		std::string source = FileSystem::ReadString(m_ShaderSourcePath);
		m_ShaderSource = PreProcess(source);

		SK_CORE_TRACE_TAG(Tag::Renderer, "Compiling Shader: {}", m_ShaderSourcePath);

		const ShaderUtils::ShaderStage::Flags changedStages = ShaderUtils::ShaderStage::Pixel | ShaderUtils::ShaderStage::Vertex;
		bool compileSucceeded = CompileOrGetBinaries(changedStages, forceCompile);
		if (!compileSucceeded)
		{
			SK_CORE_ASSERT(false);
			return false;
		}

		return true;
	}

	Ref<Shader> DirectXShaderCompiler::Compile(const std::filesystem::path& shaderSourcePath, bool forceCompile, bool disableOptimization)
	{
		Ref<DirectXShader> shader = Ref<DirectXShader>::Create();
		shader->m_FilePath = shaderSourcePath;
		shader->m_Name = shaderSourcePath.stem().string();

		Ref<DirectXShaderCompiler> compiler = Ref<DirectXShaderCompiler>::Create(shaderSourcePath, disableOptimization);
		if (!compiler->Reload(forceCompile))
			return nullptr;

		shader->LoadShader(compiler->m_ShaderBinary);
		return shader;
	}

	std::unordered_map<ShaderUtils::ShaderStage::Type, std::string> DirectXShaderCompiler::PreProcess(const std::string& source)
	{
		switch (m_Language)
		{
			case ShaderUtils::ShaderLanguage::HLSL: return PreProcessHLSL(source);
			case ShaderUtils::ShaderLanguage::GLSL:	return PreProcessGLSL(source);
		}

		SK_CORE_VERIFY(false);
		return {};
	}

	std::unordered_map<ShaderUtils::ShaderStage::Type, std::string> DirectXShaderCompiler::PreProcessHLSL(const std::string& source)
	{
		std::unordered_map<ShaderUtils::ShaderStage::Type, std::string> shaderSource;

		const std::string_view ShaderStageToken = "#pragma stage";

		size_t offset = 0;

		while (offset != std::string::npos)
		{
			const size_t moduleBegin = source.find(ShaderStageToken, offset);
			if (moduleBegin == std::string::npos)
			{
				SK_CORE_ERROR_TAG(Tag::Renderer, "Failed to find Shader Stage!");
				return {};
			}

			const size_t moduleEnd = source.find(ShaderStageToken, offset + moduleBegin + ShaderStageToken.length());
			std::string moduleSource = source.substr(moduleBegin, moduleEnd == std::string::npos ? std::string::npos : moduleEnd - moduleBegin);

			const size_t stageBegin = moduleSource.find(ShaderStageToken);
			if (stageBegin == std::string::npos)
			{
				SK_CORE_ERROR_TAG(Tag::Renderer, "Failed to find Shader Stage!");
				return {};
			}

			const size_t stageArgBegin = moduleSource.find_first_not_of(": ", stageBegin + ShaderStageToken.length());
			const size_t stageArgEnd = moduleSource.find_first_of(" \n\r", stageArgBegin);

			std::string stageString = moduleSource.substr(stageArgBegin, stageArgEnd - stageArgBegin);
			ShaderUtils::ShaderStage::Type stage = utils::GetShaderStage(stageString);
			offset = moduleEnd;

			for (size_t versionBegin = moduleSource.find("#version"); versionBegin != std::string::npos; versionBegin = moduleSource.find("#version"))
			{
				const size_t versionEnd = moduleSource.find_first_of("\n\r", versionBegin);
				moduleSource.erase(versionBegin, versionEnd - versionBegin);
				SK_CORE_WARN_TAG(Tag::Renderer, "The #version Preprocessor directive is depricated for shader written in HLSL");
			}

			SK_CORE_ASSERT(stage != ShaderUtils::ShaderStage::None);
			SK_CORE_ASSERT(!moduleSource.empty());
			shaderSource[stage] = moduleSource;
		}

		return shaderSource;
	}

	std::unordered_map<Shark::ShaderUtils::ShaderStage::Type, std::string> DirectXShaderCompiler::PreProcessGLSL(const std::string& source)
	{
		std::unordered_map<ShaderUtils::ShaderStage::Type, std::string> shaderSource;

		const std::string_view VersionToken = "#version";
		const std::string_view ShaderStageToken = "#pragma stage";

		size_t offset = 0;

		while (offset != std::string::npos)
		{
			const size_t moduleBegin = source.find(VersionToken, offset);
			if (moduleBegin == std::string::npos)
			{
				SK_CORE_ERROR_TAG(Tag::Renderer, "Failed to find Shader Version!");
				return {};
			}

			const size_t moduleEnd = source.find(VersionToken, offset + VersionToken.length());
			std::string moduleSource = source.substr(moduleBegin, moduleEnd == std::string::npos ? std::string::npos : moduleEnd - moduleBegin);

			const size_t stageBegin = moduleSource.find(ShaderStageToken);
			if (stageBegin == std::string::npos)
			{
				SK_CORE_ERROR_TAG(Tag::Renderer, "Failed to find Shader Stage!");
				return {};
			}

			const size_t stageArgBegin = moduleSource.find_first_not_of(": ", stageBegin + ShaderStageToken.length());
			const size_t stageArgEnd = moduleSource.find_first_of(" \n\r", stageArgBegin);
			std::string stageString = moduleSource.substr(stageArgBegin, stageArgEnd - stageArgBegin);
			ShaderUtils::ShaderStage::Type stage = utils::GetShaderStage(stageString);
			offset = moduleEnd;

			SK_CORE_ASSERT(stage != ShaderUtils::ShaderStage::None);
			SK_CORE_ASSERT(!moduleSource.empty());
			shaderSource[stage] = moduleSource;
		}

		return shaderSource;
	}

	bool DirectXShaderCompiler::CompileOrGetBinaries(ShaderUtils::ShaderStage::Flags changedStages, bool forceCompile)
	{
		SK_SCOPED_TIMER("DirectXShaderCompiler::CompileOrGetBinaries");

		shaderc::Compiler compiler;
		shaderc::CompileOptions options;

		if (m_Options.AutoCombineImageSamplers)
			options.SetAutoSampledTextures(true);
		
		options.SetSourceLanguage(utils::GetShaderCSourceLanguage(m_Language));
		options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
		options.SetOptimizationLevel(m_Options.DisableOptimization ? shaderc_optimization_level_zero : shaderc_optimization_level_performance);
		options.SetGenerateDebugInfo();

		for (auto& [stage, source] : m_ShaderSource)
		{
			if (!forceCompile && !(changedStages & stage))
				continue;

			std::string name = fmt::format("{}{}", m_ShaderSourcePath.stem().string(), utils::ShaderStageExtension(stage));
			shaderc::SpvCompilationResult compilerResult = compiler.CompileGlslToSpv(source, utils::ToShaderCShaderKind(stage), name.c_str(), options);
			if (compilerResult.GetCompilationStatus() != shaderc_compilation_status_success)
			{
				auto errorMessage = compilerResult.GetErrorMessage();
				SK_CORE_ERROR_TAG(Tag::Renderer, errorMessage);
				return false;
			}

			m_SPIRVData[stage] = { compilerResult.begin(), compilerResult.end() };

			if (m_Language != ShaderUtils::ShaderLanguage::HLSL)
			{
				spirv_cross::CompilerHLSL compilerHLSL(m_SPIRVData.at(stage));
				spirv_cross::CompilerHLSL::Options options;
				options.shader_model = 40;
				compilerHLSL.set_hlsl_options(options);
				m_HLSLShaderSource[stage] = compilerHLSL.compile();
			}

			const auto& shaderSource = m_Language == ShaderUtils::ShaderLanguage::HLSL ? m_ShaderSource.at(stage) : m_HLSLShaderSource.at(stage);
			if (!CompileHLSL(stage, shaderSource, m_ShaderBinary[stage]))
				return false;
		}

		return true;
	}

	bool DirectXShaderCompiler::CompileHLSL(ShaderUtils::ShaderStage::Type stage, const std::string& hlslSourceCode, std::vector<byte>& binary) const
	{
		UINT flags = 0;
		flags |= D3DCOMPILE_ALL_RESOURCES_BOUND;
		//flags |= D3DCOMPILE_WARNINGS_ARE_ERRORS;

		if (m_Options.DisableOptimization)
		{
			flags |= D3DCOMPILE_DEBUG;
			flags |= D3DCOMPILE_OPTIMIZATION_LEVEL0;
		}
		else
		{
			flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
		}

		ID3DBlob* shaderBinary = nullptr;
		ID3DBlob* errorMessage = nullptr;
		std::string name = fmt::format("{}{}", m_ShaderSourcePath.stem().string(), utils::ShaderStageExtension(stage));
		if (FAILED(D3DCompile(hlslSourceCode.data(), hlslSourceCode.size(), name.c_str(), nullptr, nullptr, "main", utils::GetShaderVersion(stage), flags, 0, &shaderBinary, &errorMessage)))
		{
			std::string errorStr = fmt::format("Failed to Compile HLSL (Stage: {0})\n{1}", ToString(stage), (char*)errorMessage->GetBufferPointer());
			SK_CORE_ERROR_TAG(Tag::Renderer, errorStr);
			SK_CORE_ASSERT(false);
			errorMessage->Release();
			return false;
		}

		binary.resize(shaderBinary->GetBufferSize());
		memcpy(binary.data(), shaderBinary->GetBufferPointer(), shaderBinary->GetBufferSize());
		shaderBinary->Release();
		return true;
	}

}
