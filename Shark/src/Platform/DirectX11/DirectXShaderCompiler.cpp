#include "skpch.h"
#include "DirectXShaderCompiler.h"

#include "Shark/File/FileSystem.h"
#include "Shark/Math/Math.h"
#include "Shark/Utils/String.h"

#include "Platform/DirectX11/DirectXShader.h"
#include "Platform/DirectX11/DirectXShaderCache.h"

#include <fmt/chrono.h>

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

		static std::string_view GetDirectXCacheDirectory()
		{
			return "Cache/Shaders/DirectX";
		}

		static void CreateChacheDirectoryIfNeeded()
		{
			const auto directXCacheDirectory = GetDirectXCacheDirectory();
			if (!std::filesystem::exists(directXCacheDirectory))
				std::filesystem::create_directories(directXCacheDirectory);
		}

		static std::string_view GetCacheFileExtension(ShaderUtils::ShaderStage::Type stage)
		{
			switch (stage)
			{
				case ShaderUtils::ShaderStage::Vertex: return ".vert";
				case ShaderUtils::ShaderStage::Pixel: return ".frag";
			}

			SK_CORE_VERIFY(false);
			return "";
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
		m_ShaderBinary.clear();

		utils::CreateChacheDirectoryIfNeeded();

		std::string source = FileSystem::ReadString(m_ShaderSourcePath);
		m_ShaderSource = PreProcess(source);

		SK_CORE_TRACE_TAG(Tag::Renderer, "Compiling Shader: {}", m_ShaderSourcePath);

		const ShaderUtils::ShaderStage::Flags changedStages = DirectXShaderCache::HasChanged(this);
		bool compileSucceeded = CompileOrLoadBinaries(changedStages, forceCompile);
		if (!compileSucceeded)
		{
			SK_CORE_ASSERT(false);
			return false;
		}

		if (forceCompile || changedStages)
		{
			try
			{
				RelfectShaderStages(m_SPIRVData);
			}
			catch (const std::exception& e)
			{
				SK_CORE_ERROR(e.what());
			}
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
		auto shaderSource = PreProcessHLSLSource(source);

		for (const auto& [stage, moduleSource] : shaderSource)
		{
			auto& metadata = m_ShaderStageMetadata[stage];
			metadata.Stage = stage;
			metadata.HashCode = Hash::GenerateFNV(moduleSource);

			m_Stages |= stage;
		}

		return shaderSource;
	}

	std::unordered_map<Shark::ShaderUtils::ShaderStage::Type, std::string> DirectXShaderCompiler::PreProcessGLSL(const std::string& source)
	{
		auto shaderSource = PreProcessHLSLSource(source);

		for (const auto& [stage, moduleSource] : shaderSource)
		{
			auto& metadata = m_ShaderStageMetadata[stage];
			metadata.Stage = stage;
			metadata.HashCode = Hash::GenerateFNV(moduleSource);

			m_Stages |= stage;
		}

		return shaderSource;
	}

	std::unordered_map<Shark::ShaderUtils::ShaderStage::Type, std::string> DirectXShaderCompiler::PreProcessHLSLSource(const std::string& source)
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

	std::unordered_map<Shark::ShaderUtils::ShaderStage::Type, std::string> DirectXShaderCompiler::PreProcessGLSLSource(const std::string& source)
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

	bool DirectXShaderCompiler::CompileOrLoadBinaries(ShaderUtils::ShaderStage::Flags changedStages, bool forceCompile)
	{
		SK_SCOPED_TIMER("DirectXShaderCompiler::CompileOrGetBinaries");

		for (auto& [stage, source] : m_ShaderSource)
		{
			if (!CompileOrLoadBinary(stage, changedStages, forceCompile))
				return false;
		}

		if (forceCompile || changedStages)
		{
			DirectXShaderCache::OnShaderCompiled(this);
		}

		return true;
	}

	bool DirectXShaderCompiler::CompileOrLoadBinary(ShaderUtils::ShaderStage::Type stage, ShaderUtils::ShaderStage::Flags changedStages, bool forceCompile)
	{
		std::vector<byte>& binary = m_ShaderBinary[stage];

		if (!forceCompile && !(changedStages & stage))
			TryLoadDirectX(stage, binary);

		if (binary.empty())
		{
			std::string error = Compile(stage, binary, m_SPIRVData[stage]);
			if (error.empty())
			{
				SerializeDirectX(stage, binary);
				return true;
			}

			SK_CORE_ERROR_TAG("Renderer", "Failed to Compile Shader! Trying to load older Version from Cache.\n{}", error);
			TryLoadDirectX(stage, binary);
			if (binary.size())
			{
				const std::string cacheFile = fmt::format("{0}/{1}{2}", utils::GetDirectXCacheDirectory(), m_ShaderSourcePath.stem().string(), utils::GetCacheFileExtension(stage));
				auto fileTime = std::filesystem::last_write_time(cacheFile).time_since_epoch().count() - std::filesystem::__std_fs_file_time_epoch_adjustment;
				auto lastWritten = std::chrono::system_clock::time_point(std::chrono::system_clock::duration(fileTime));
				SK_CORE_WARN_TAG("Renderer", "Cached Shader loaded\n\tSource: {}\n\tModified: {}", cacheFile, lastWritten);
				return true;
			}
			else
			{
				SK_CORE_ERROR_TAG("Renderer", "Failed to load Shader from Cache!");
				return false;
			}
		}

		return true;
	}

	std::string DirectXShaderCompiler::Compile(ShaderUtils::ShaderStage::Type stage, std::vector<byte>& outputBinary, std::vector<uint32_t>& outputSPIRVDebug)
	{
		shaderc::Compiler compiler;
		shaderc::CompileOptions shadercOptions;

		shadercOptions.SetSourceLanguage(utils::GetShaderCSourceLanguage(m_Language));
		shadercOptions.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
		shadercOptions.SetGenerateDebugInfo();

		if (!m_Options.DisableOptimization)
			shadercOptions.SetOptimizationLevel(shaderc_optimization_level_performance);

		shadercOptions.SetAutoSampledTextures(m_Options.AutoCombineImageSamplers);

		std::string name = fmt::format("{}{}", m_ShaderSourcePath.stem().string(), utils::ShaderStageExtension(stage));
		const shaderc::SpvCompilationResult compiledModule = compiler.CompileGlslToSpv(m_ShaderSource.at(stage), utils::ToShaderCShaderKind(stage), name.c_str(), shadercOptions);

		if (compiledModule.GetCompilationStatus() != shaderc_compilation_status_success)
		{
			return compiledModule.GetErrorMessage();
		}

		outputSPIRVDebug = { compiledModule.begin(), compiledModule.end() };

		std::string hlslSource;
		if (m_Language != ShaderUtils::ShaderLanguage::HLSL)
			hlslSource = CrossCompileToHLSL(outputSPIRVDebug);

		if (std::string error = CompileHLSL(stage, hlslSource.size() ? hlslSource : m_ShaderSource.at(stage), outputBinary); error.size())
		{
			return error;
		}

		return {};
	}

	std::string DirectXShaderCompiler::CompileHLSL(ShaderUtils::ShaderStage::Type stage, const std::string& hlslSourceCode, std::vector<byte>& outputBinary) const
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
			errorMessage->Release();
			return errorStr;
		}

		outputBinary.resize(shaderBinary->GetBufferSize());
		memcpy(outputBinary.data(), shaderBinary->GetBufferPointer(), shaderBinary->GetBufferSize());
		shaderBinary->Release();
		return {};
	}

	std::string DirectXShaderCompiler::CrossCompileToHLSL(const std::vector<uint32_t>& spirvBinary)
	{
		spirv_cross::CompilerHLSL compilerHLSL(spirvBinary);
		spirv_cross::CompilerHLSL::Options options;
		options.shader_model = 40;
		compilerHLSL.set_hlsl_options(options);
		return compilerHLSL.compile();
	}

	void DirectXShaderCompiler::SerializeDirectX(ShaderUtils::ShaderStage::Type stage, const std::vector<byte>& directXData)
	{
		const std::string cacheFile = fmt::format("{0}/{1}{2}", utils::GetDirectXCacheDirectory(), m_ShaderSourcePath.stem().string(), utils::GetCacheFileExtension(stage));
		std::ofstream stream(cacheFile, std::ios::out | std::ios::binary);
		if (!stream)
		{
			SK_CORE_ERROR_TAG("Renderer", "Failed to Serialize DirectX Data!");
			return;
		}

		stream.write((const char*)directXData.data(), directXData.size());
		stream.close();
		m_StagesWrittenToCache |= stage;
	}

	bool DirectXShaderCompiler::TryLoadDirectX(ShaderUtils::ShaderStage::Type stage, std::vector<byte>& directXData)
	{
		const std::string cacheFile = fmt::format("{0}/{1}{2}", utils::GetDirectXCacheDirectory(), m_ShaderSourcePath.stem().string(), utils::GetCacheFileExtension(stage));
		Buffer fileData = FileSystem::ReadBinary(cacheFile);
		if (!fileData)
			return false;

		fileData.CopyTo(directXData);
		fileData.Release();
	}

	void DirectXShaderCompiler::RelfectShaderStages(const std::unordered_map<ShaderUtils::ShaderStage::Type, std::vector<uint32_t>> spirvData)
	{
		for (const auto& [stage, spirvBinary] : spirvData)
		{
			SK_CORE_TRACE_TAG("Renderer", "===========================");
			SK_CORE_TRACE_TAG("Renderer", " DirectX Shader Reflection");
			SK_CORE_TRACE_TAG("Renderer", "===========================");

			spirv_cross::Compiler compiler(spirvBinary);
			spirv_cross::ShaderResources shaderResources = compiler.get_shader_resources();

			SK_CORE_TRACE_TAG("Renderer", "Constant Buffers:");
			for (const auto& constantBuffer : shaderResources.uniform_buffers)
			{
				const spirv_cross::SPIRType& bufferType = compiler.get_type(constantBuffer.base_type_id);
				const auto& name = constantBuffer.name;
				uint32_t size = compiler.get_declared_struct_size(bufferType);
				uint32_t members = bufferType.member_types.size();
				uint32_t binding = compiler.get_decoration(constantBuffer.id, spv::DecorationBinding);

				SK_CORE_TRACE_TAG("Renderer", "  Name: {}", name);
				SK_CORE_TRACE_TAG("Renderer", "  Binding: {}", binding);
				SK_CORE_TRACE_TAG("Renderer", "  Size: {}", size);
				SK_CORE_TRACE_TAG("Renderer", "  Members: {}", members);
				SK_CORE_TRACE_TAG("Renderer", "-------------------");
			}

			SK_CORE_TRACE_TAG("Renderer", "Combined Sampled Images:");
			for (const auto& resource : shaderResources.sampled_images)
			{
				const auto& imageType = compiler.get_type(resource.type_id);
				const auto& name = resource.name;
				uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
				uint32_t arraySize = imageType.array[0];

				SK_CORE_TRACE_TAG("Renderer", "  Name: {}", name);
				SK_CORE_TRACE_TAG("Renderer", "  Binding: {}", binding);
				SK_CORE_TRACE_TAG("Renderer", "  Array Size: {}", arraySize);
				SK_CORE_TRACE_TAG("Renderer", "-------------------");
			}
			
			SK_CORE_TRACE_TAG("Renderer", "Textures:");
			for (const auto& resource : shaderResources.separate_images)
			{
				const auto& imageType = compiler.get_type(resource.type_id);
				const auto& name = resource.name;
				uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
				uint32_t arraySize = imageType.array[0];

				SK_CORE_TRACE_TAG("Renderer", "  Name: {}", name);
				SK_CORE_TRACE_TAG("Renderer", "  Binding: {}", binding);
				SK_CORE_TRACE_TAG("Renderer", "  Array Size: {}", arraySize);
				SK_CORE_TRACE_TAG("Renderer", "-------------------");
			}
			
			SK_CORE_TRACE_TAG("Renderer", "Samplers:");
			for (const auto& resource : shaderResources.separate_samplers)
			{
				const auto& imageType = compiler.get_type(resource.type_id);
				const auto& name = resource.name;
				uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
				uint32_t arraySize = imageType.array[0];

				SK_CORE_TRACE_TAG("Renderer", "  Name: {}", name);
				SK_CORE_TRACE_TAG("Renderer", "  Binding: {}", binding);
				SK_CORE_TRACE_TAG("Renderer", "  Array Size: {}", arraySize);
				SK_CORE_TRACE_TAG("Renderer", "-------------------");
			}

		}
	}

}
