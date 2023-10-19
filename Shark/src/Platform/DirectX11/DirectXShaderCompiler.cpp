#include "skpch.h"
#include "DirectXShaderCompiler.h"

#include "Shark/File/FileSystem.h"
#include "Shark/Math/Math.h"
#include "Shark/Utils/String.h"

#include "Platform/DirectX11/ShaderUtils.h"
#include "Platform/DirectX11/DirectXShader.h"
#include "Platform/DirectX11/DirectXShaderCache.h"

#include <fmt/chrono.h>
#include <yaml-cpp/yaml.h>

#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_hlsl.hpp>
#include <d3dcompiler.h>

namespace YAML {

	template<>
	struct convert<Shark::ShaderReflection::MemberDeclaration>
	{
		static Node encode(const Shark::ShaderReflection::MemberDeclaration& memberDeclaration)
		{
			Node node(NodeType::Map);
			node.force_insert("Name", memberDeclaration.Name);
			node.force_insert("Type", Shark::ToString(memberDeclaration.Type));
			node.force_insert("Size", memberDeclaration.Size);
			node.force_insert("Offset", memberDeclaration.Offset);
			return node;
		}

		static bool decode(const Node& node, Shark::ShaderReflection::MemberDeclaration& outMemberDeclaration)
		{
			if (!node.IsMap() || node.size() != 4)
				return false;

			outMemberDeclaration.Name = node["Name"].as<std::string>();
			outMemberDeclaration.Type = Shark::StringToShaderReflectionVariabelType(node["Type"].as<std::string>());
			outMemberDeclaration.Size = node["Size"].as<uint32_t>();
			outMemberDeclaration.Offset = node["Offset"].as<uint32_t>();
			return true;
		}
	};

	template<>
	struct convert<Shark::ShaderReflection::ConstantBuffer>
	{
		static Node encode(const Shark::ShaderReflection::ConstantBuffer& constantBuffer)
		{
			Node node(NodeType::Map);
			node.force_insert("Name", constantBuffer.Name);
			node.force_insert("Stage", Shark::ToString(constantBuffer.Stage));
			node.force_insert("UpdateFrequency", Shark::ToString(constantBuffer.UpdateFrequency));
			node.force_insert("Binding", constantBuffer.Binding);
			node.force_insert("Size", constantBuffer.Size);
			node.force_insert("MemberCount", constantBuffer.MemberCount);
			node.force_insert("Members", Node(constantBuffer.Members));
			return node;
		}

		static bool decode(const Node& node, Shark::ShaderReflection::ConstantBuffer& outConstantBuffer)
		{
			if (!node.IsMap() || node.size() != 7)
				return false;

			outConstantBuffer.Name = node["Name"].as<std::string>();
			outConstantBuffer.Stage = Shark::StringToShaderReflectionShaderStage(node["Stage"].as<std::string>());
			outConstantBuffer.UpdateFrequency = Shark::StringToShaderReflectionUpdateFrequency(node["UpdateFrequency"].as<std::string>());
			outConstantBuffer.Binding = node["Binding"].as<uint32_t>();
			outConstantBuffer.Size = node["Size"].as<uint32_t>();
			outConstantBuffer.MemberCount = node["MemberCount"].as<uint32_t>();
			outConstantBuffer.Members = node["Members"].as<std::vector<Shark::ShaderReflection::MemberDeclaration>>();
			return true;
		}
	};

	template<>
	struct convert<Shark::ShaderReflection::Resource>
	{
		static Node encode(const Shark::ShaderReflection::Resource& resource)
		{
			Node node(NodeType::Map);
			node.force_insert("Name", resource.Name);
			node.force_insert("Stage", Shark::ToString(resource.Stage));
			node.force_insert("Type", Shark::ToString(resource.Type));
			node.force_insert("Binding", resource.Binding);
			node.force_insert("ArraySize", resource.ArraySize);
			return node;
		}

		static bool decode(const Node& node, Shark::ShaderReflection::Resource& outResource)
		{
			if (!node.IsMap() || node.size() != 5)
				return false;

			outResource.Name = node["Name"].as<std::string>();
			outResource.Stage = Shark::StringToShaderReflectionShaderStage(node["Stage"].as<std::string>());
			outResource.Type = Shark::StringToShaderReflectionResourceType(node["Type"].as<std::string>());
			outResource.Binding = node["Binding"].as<uint32_t>();
			outResource.ArraySize = node["ArraySize"].as<uint32_t>();
			return true;
		}
	};

}

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
			if (String::Compare(stage, "vertex", String::Case::Ingnore)) return ShaderUtils::ShaderStage::Vertex;
			if (String::Compare(stage, "vert", String::Case::Ingnore)) return ShaderUtils::ShaderStage::Vertex;


			if (String::Compare(stage, "pixel", String::Case::Ingnore)) return ShaderUtils::ShaderStage::Pixel;
			if (String::Compare(stage, "frag", String::Case::Ingnore)) return ShaderUtils::ShaderStage::Pixel;
			if (String::Compare(stage, "fragment", String::Case::Ingnore)) return ShaderUtils::ShaderStage::Pixel;

			SK_CORE_ASSERT(false, "Unkown Shader Stage");
			return ShaderUtils::ShaderStage::None;
		}

		static ShaderUtils::ShaderLanguage GetLanguageFromFileExtension(const std::filesystem::path& shaderSourcePath)
		{
			auto extension = shaderSourcePath.extension().string();

			if (String::Compare(extension, ".hlsl", String::Case::Ingnore))
				return ShaderUtils::ShaderLanguage::HLSL;

			if (String::Compare(extension, ".glsl", String::Case::Ingnore))
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
				case ShaderUtils::ShaderStage::Vertex: return "vs_5_0";
				case ShaderUtils::ShaderStage::Pixel: return "ps_5_0";
			}

			SK_CORE_ASSERT(false, "Unkown Shader Stage");
			return nullptr;
		}

		static std::string_view GetDirectXCacheDirectory()
		{
			return "Cache/Shaders/DirectX";
		}

		static std::string_view GetSPIRVCacheDirectory()
		{
			return "Cache/Shaders/DirectX/SPIRV";
		}

		static std::string_view GetReflectionDataCacheDirectory()
		{
			return "Cache/Shaders/ReflectionData";
		}

		static void CreateChacheDirectoryIfNeeded()
		{
			const std::filesystem::path directXCacheDirectory = GetDirectXCacheDirectory();
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

		static std::string GetDirectXCacheFile(const std::string& fileName, ShaderUtils::ShaderStage::Type stage)
		{
			return fmt::format("{0}/{1}{2}", utils::GetDirectXCacheDirectory(), fileName, utils::GetCacheFileExtension(stage));
		}

		static ShaderReflection::ShaderStage ToShaderReflectionShaderStage(ShaderUtils::ShaderStage::Type stage)
		{
			switch (stage)
			{
				case ShaderUtils::ShaderStage::None: return ShaderReflection::ShaderStage::None;
				case ShaderUtils::ShaderStage::Vertex: return ShaderReflection::ShaderStage::Vertex;
				case ShaderUtils::ShaderStage::Pixel: return ShaderReflection::ShaderStage::Pixel;
			}

			SK_CORE_ASSERT(FALSE, "Unkown Shader Stage");
			return ShaderReflection::ShaderStage::None;
		}

		static ShaderReflection::VariableType GetVariableType(const spirv_cross::SPIRType& spirType)
		{
			ShaderReflection::VariableType baseType = ShaderReflection::VariableType::None;

			switch (spirType.basetype)
			{
				case spirv_cross::SPIRType::Int: baseType = ShaderReflection::VariableType::Int; break;
				case spirv_cross::SPIRType::UInt: baseType = ShaderReflection::VariableType::UInt; break;
				case spirv_cross::SPIRType::Float: baseType = ShaderReflection::VariableType::Float; break;
				case spirv_cross::SPIRType::Boolean: baseType = ShaderReflection::VariableType::Bool; break;
				default: SK_CORE_VERIFY(false, "Invalid BaseType"); break;
			}

			if (spirType.columns == 1)
			{
				(uint32_t&)baseType += spirType.vecsize - 1;
				return baseType;
			}

			if (spirType.columns == 3 && spirType.vecsize == 3 && baseType == ShaderReflection::VariableType::Float)
				return ShaderReflection::VariableType::Mat3;

			if (spirType.columns == 4 && spirType.vecsize == 4 && baseType == ShaderReflection::VariableType::Float)
				return ShaderReflection::VariableType::Mat4;

			SK_CORE_ASSERT(false, "Unkown VariableType");
			return ShaderReflection::VariableType::None;
		}

		static void EraseExtensions(std::string& hlslSource)
		{
			size_t pos;
			while ((pos = hlslSource.find("#extension")) != std::string::npos)
			{
				hlslSource.erase(pos, hlslSource.find_first_of("\n\r"));
			}
		}

		static std::string GetNameForSKTexture2D(const std::string& fullName)
		{
			return fullName.substr(0, fullName.find_last_of('.'));
		}

		static std::chrono::system_clock::time_point GetFileTimeAsSystemTime(const std::filesystem::path filepath)
		{
			const auto fileTime = std::filesystem::last_write_time(filepath).time_since_epoch().count() - std::filesystem::__std_fs_file_time_epoch_adjustment;
			return std::chrono::system_clock::time_point(std::chrono::system_clock::duration(fileTime));
		}

		static ShaderReflection::UpdateFrequencyType GetUpdateFrequencyFromBinding(uint32_t binding)
		{
			static constexpr uint32_t BindingPerScene = 0;
			static constexpr uint32_t BindingPerDrawCall = 1;

			if (binding == BindingPerScene)
				return ShaderReflection::UpdateFrequencyType::PerScene;
			if (binding == BindingPerDrawCall)
				return ShaderReflection::UpdateFrequencyType::PerDrawCall;

			return ShaderReflection::UpdateFrequencyType::PerMaterial;
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
		m_ReflectionData = {};

		utils::CreateChacheDirectoryIfNeeded();

		std::string source = FileSystem::ReadString(m_ShaderSourcePath);
		m_ShaderSource = PreProcess(source);

		SK_CORE_INFO_TAG(Tag::Renderer, "Compiling Shader: {}", m_ShaderSourcePath);

		const ShaderUtils::ShaderStage::Flags changedStages = DirectXShaderCache::HasChanged(this);
		bool compileSucceeded = CompileOrLoadBinaries(changedStages, forceCompile);
		if (!compileSucceeded)
		{
			SK_CORE_ASSERT(false);
			return false;
		}

		if (forceCompile || changedStages)
		{
			ReflectAllShaderStages(m_SPIRVData);
			SerializeReflectionData();
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
		shader->SetReflectionData(compiler->m_ReflectionData);

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
			metadata.SourceMetadata = GetMetadata(moduleSource);
			metadata.CacheFile = utils::GetDirectXCacheFile(m_ShaderSourcePath.stem().string(), stage);

			m_Stages |= stage;
		}

		return shaderSource;
	}

	std::unordered_map<Shark::ShaderUtils::ShaderStage::Type, std::string> DirectXShaderCompiler::PreProcessGLSL(const std::string& source)
	{
		auto shaderSource = PreProcessGLSLSource(source);

		for (const auto& [stage, moduleSource] : shaderSource)
		{
			auto& metadata = m_ShaderStageMetadata[stage];
			metadata.Stage = stage;
			metadata.HashCode = Hash::GenerateFNV(moduleSource);
			metadata.SourceMetadata = GetMetadata(moduleSource);
			metadata.CacheFile = utils::GetDirectXCacheFile(m_ShaderSourcePath.stem().string(), stage);

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

	std::string DirectXShaderCompiler::GetMetadata(const std::string& source)
	{
		static constexpr std::string_view BeginMetadataToken = "begin_metadata";
		static constexpr std::string_view EndMetadataToken = "end_metadata";

		size_t begin = source.find(BeginMetadataToken);
		if (begin == std::string::npos)
			return {};

		size_t end = source.find(EndMetadataToken);
		if (end == std::string::npos)
			return {};

		size_t endOfLineEnd = source.find_first_of("\r\n", end);

		std::string metadata = source.substr(begin, endOfLineEnd - begin);
		String::Remove(metadata, "// ");
		String::Remove(metadata, "//");
		return metadata;
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
		else
		{
			ReadReflectionData();
		}

		return true;
	}

	bool DirectXShaderCompiler::CompileOrLoadBinary(ShaderUtils::ShaderStage::Type stage, ShaderUtils::ShaderStage::Flags changedStages, bool forceCompile)
	{
		std::vector<byte>& binary = m_ShaderBinary[stage];
		std::vector<uint32_t>& spirvBinary = m_SPIRVData[stage];

		if (!forceCompile && !(changedStages & stage))
			TryLoadDirectXAndSPIRV(stage, binary, spirvBinary);

		if (binary.empty() && !spirvBinary.empty())
		{
			std::string error = CompileFromSPIRV(stage, spirvBinary, binary);
			SK_CORE_ASSERT(error.empty() == !binary.empty());
			if (error.empty() && !binary.empty())
			{
				SerializeDirectX(stage, binary);
				SerializeSPIRV(stage, spirvBinary);
			}
		}

		if (binary.empty())
		{
			std::string error = Compile(stage, binary, spirvBinary);
			if (error.empty())
			{
				SerializeDirectX(stage, binary);
				SerializeSPIRV(stage, spirvBinary);
				return true;
			}

			SK_CORE_ERROR_TAG("Renderer", "Failed to Compile Shader! Trying to load older Version from Cache.\n{}", error);
			TryLoadDirectXAndSPIRV(stage, binary, spirvBinary);
			if (binary.size())
			{
				const std::string cacheFile = fmt::format("{0}/{1}{2}", utils::GetDirectXCacheDirectory(), m_ShaderSourcePath.stem().string(), utils::GetCacheFileExtension(stage));
				SK_CORE_WARN_TAG("Renderer", "Older Shader loaded\n\tSource: {}\n\tModified: {}", cacheFile, utils::GetFileTimeAsSystemTime(cacheFile));
			}
			else
			{
				SK_CORE_ERROR_TAG("Renderer", "Failed to load older Shader version from Cache!");
				return false;
			}
		}

		return true;
	}

	class FileIncluder : public shaderc::CompileOptions::IncluderInterface
	{
	public:
		FileIncluder(const std::filesystem::path& baseDirectory = "Resources/Shaders")
			: m_BaseDirectory(std::filesystem::absolute(baseDirectory))
		{}

		virtual shaderc_include_result* GetInclude(const char* requested_source, shaderc_include_type type, const char* requesting_source, size_t include_depth) override
		{
			std::string sourcePath = (m_BaseDirectory / requested_source).string();
			SK_CORE_ASSERT(FileSystem::Exists(sourcePath));
			std::string fileContent = FileSystem::ReadString(sourcePath);

			if (fileContent.empty())
			{
				IncludeResult& includeResult = m_IncludeResults[sourcePath];
				includeResult.SourcePath = sourcePath;
				includeResult.Content = fileContent;

				includeResult.Result.source_name = nullptr;
				includeResult.Result.source_name_length = 0;
				includeResult.Result.content = "File not found";
				includeResult.Result.content_length = strlen(includeResult.Result.content);
				return &includeResult.Result;
			}

			IncludeResult& includeResult = m_IncludeResults[sourcePath];
			includeResult.SourcePath = sourcePath;
			includeResult.Content = fileContent;

			includeResult.Result.source_name = includeResult.SourcePath.c_str();
			includeResult.Result.source_name_length = includeResult.SourcePath.length();
			includeResult.Result.content = includeResult.Content.c_str();
			includeResult.Result.content_length = includeResult.Content.length();

			return &includeResult.Result;
		}


		virtual void ReleaseInclude(shaderc_include_result* data) override
		{
		}

	private:
		std::filesystem::path m_BaseDirectory;

		struct IncludeResult
		{
			std::string SourcePath;
			std::string Content;
			shaderc_include_result Result;
		};

		std::unordered_map<std::string, IncludeResult> m_IncludeResults;
	};

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
		shadercOptions.SetIncluder(std::make_unique<FileIncluder>());

		std::string name = fmt::format("{}{}", m_ShaderSourcePath.stem().string(), utils::ShaderStageExtension(stage));

		const shaderc::PreprocessedSourceCompilationResult preprocessedSourceResult = compiler.PreprocessGlsl(m_ShaderSource.at(stage), utils::ToShaderCShaderKind(stage), name.c_str(), shadercOptions);
		if (preprocessedSourceResult.GetCompilationStatus() != shaderc_compilation_status_success)
			return preprocessedSourceResult.GetErrorMessage();

		std::string preprocessedSource = std::string(preprocessedSourceResult.cbegin(), preprocessedSourceResult.cend());
		const shaderc::SpvCompilationResult compiledModule = compiler.CompileGlslToSpv(preprocessedSource, utils::ToShaderCShaderKind(stage), name.c_str(), shadercOptions);
		utils::EraseExtensions(preprocessedSource);

		if (compiledModule.GetCompilationStatus() != shaderc_compilation_status_success)
		{
			return compiledModule.GetErrorMessage();
		}

		outputSPIRVDebug = { compiledModule.begin(), compiledModule.end() };

		std::string hlslSource;
		if (m_Language != ShaderUtils::ShaderLanguage::HLSL)
			hlslSource = CrossCompileToHLSL(stage, outputSPIRVDebug);

		if (std::string error = CompileHLSL(stage, hlslSource.size() ? hlslSource : preprocessedSource, outputBinary); error.size())
		{
			return error;
		}

		return {};
	}

	std::string DirectXShaderCompiler::CompileFromSPIRV(ShaderUtils::ShaderStage::Type stage, const std::vector<uint32_t>& spirvBinary, std::vector<byte>& outputBinary)
	{
		std::string hlslSourceCode = CrossCompileToHLSL(stage, spirvBinary);
		if (hlslSourceCode.empty())
			return "Cross Compiling SPIRV to HLSL Failed";

		return CompileHLSL(stage, hlslSourceCode, outputBinary);
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

	std::string DirectXShaderCompiler::CrossCompileToHLSL(ShaderUtils::ShaderStage::Type stage, const std::vector<uint32_t>& spirvBinary)
	{
		spirv_cross::CompilerHLSL compilerHLSL(spirvBinary);
		spirv_cross::CompilerHLSL::Options options;
		options.shader_model = 40;
		compilerHLSL.set_hlsl_options(options);

		if (stage == ShaderUtils::ShaderStage::Vertex)
		{
			spirv_cross::Compiler compiler(spirvBinary);
			spirv_cross::ShaderResources shaderResources = compiler.get_shader_resources();
			for (const auto& resource : shaderResources.stage_inputs)
			{
				std::string name = resource.name.substr(2);
				uint32_t location = compiler.get_decoration(resource.id, spv::DecorationLocation);
				SK_CORE_TRACE("Vertex Attribute Remap from location = {} to {}", location, name);
				compilerHLSL.add_vertex_attribute_remap({ location, name });
			}
		}

		return compilerHLSL.compile();
	}

	void DirectXShaderCompiler::SerializeDirectX(ShaderUtils::ShaderStage::Type stage, const std::vector<byte>& directXData)
	{
		const std::string cacheFile = fmt::format("{0}/{1}{2}", utils::GetDirectXCacheDirectory(), m_ShaderSourcePath.stem().string(), utils::GetCacheFileExtension(stage));
		if (!FileSystem::WriteBinary(FileSystem::GetResourcePath(cacheFile), Buffer::FromArray(directXData)))
		{
			SK_CORE_ERROR_TAG("Renderer", "Failed to Serialize DirectX Data!");
			return;
		}

		m_StagesWrittenToCache |= stage;
	}

	void DirectXShaderCompiler::TryLoadDirectX(ShaderUtils::ShaderStage::Type stage, std::vector<byte>& directXData)
	{
		const std::string cacheFile = fmt::format("{0}/{1}{2}", utils::GetDirectXCacheDirectory(), m_ShaderSourcePath.stem().string(), utils::GetCacheFileExtension(stage));
		Buffer fileData = FileSystem::ReadBinary(FileSystem::GetResourcePath(cacheFile));
		if (!fileData)
			return;

		fileData.CopyTo(directXData);
		fileData.Release();
		return;
	}

	void DirectXShaderCompiler::SerializeSPIRV(ShaderUtils::ShaderStage::Type stage, const std::vector<uint32_t>& spirvData)
	{
		const std::string cacheFile = fmt::format("{}/{}{}", utils::GetSPIRVCacheDirectory(), m_ShaderSourcePath.stem().string(), utils::GetCacheFileExtension(stage));
		if (!FileSystem::WriteBinary(FileSystem::GetResourcePath(cacheFile), Buffer::FromArray(spirvData)))
		{
			SK_CORE_ERROR_TAG("Renderer", "Failed to Serialize SPIRV Data");
			return;
		}
	}

	void DirectXShaderCompiler::TryLoadSPIRV(ShaderUtils::ShaderStage::Type stage, std::vector<uint32_t>& outputSPIRVData)
	{
		const std::string cacheFile = fmt::format("{}/{}{}", utils::GetSPIRVCacheDirectory(), m_ShaderSourcePath.stem().string(), utils::GetCacheFileExtension(stage));
		Buffer fileData = FileSystem::ReadBinary(FileSystem::GetResourcePath(cacheFile));
		if (!fileData)
			return;

		fileData.CopyTo(outputSPIRVData);
		fileData.Release();
	}

	void DirectXShaderCompiler::TryLoadDirectXAndSPIRV(ShaderUtils::ShaderStage::Type stage, std::vector<byte>& outputDirectXData, std::vector<uint32_t>& outputSPIRVData)
	{
		TryLoadDirectX(stage, outputDirectXData);
		TryLoadSPIRV(stage, outputSPIRVData);
	}

	void DirectXShaderCompiler::SerializeReflectionData()
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "ShaderReflection" << YAML::Value;
		out << YAML::BeginMap;

		out << YAML::Key << "ConstantBuffers" << YAML::Value;
		out << YAML::BeginSeq;
		for (const auto& [name, cbData] : m_ReflectionData.ConstantBuffers)
			out << YAML::Node(cbData);
		out << YAML::EndSeq;

		out << YAML::Key << "Resources" << YAML::Value;
		out << YAML::BeginSeq;
		for (const auto& [name, resource] : m_ReflectionData.Resources)
			out << YAML::Node(resource);
		out << YAML::EndSeq;

		out << YAML::EndMap;
		out << YAML::EndMap;

		const std::string cacheFile = fmt::format("{0}/{1}.yaml", utils::GetReflectionDataCacheDirectory(), m_ShaderSourcePath.stem().string());
		FileSystem::WriteString(FileSystem::GetResourcePath(cacheFile), out.c_str());
	}

	bool DirectXShaderCompiler::ReadReflectionData()
	{
		const std::string cacheFile = fmt::format("{0}/{1}.yaml", utils::GetReflectionDataCacheDirectory(), m_ShaderSourcePath.stem().string());
		std::string fileContent = FileSystem::ReadString(FileSystem::GetResourcePath(cacheFile));
		if (fileContent.empty())
			return false;

		YAML::Node masterNode = YAML::Load(fileContent);
		YAML::Node shaderReflectionNode = masterNode["ShaderReflection"];
		if (!shaderReflectionNode)
			return false;

		auto constantBuffers = shaderReflectionNode["ConstantBuffers"];
		for (const auto& node : constantBuffers)
		{
			ShaderReflection::ConstantBuffer constantBuffer = node.as<ShaderReflection::ConstantBuffer>();
			m_ReflectionData.ConstantBuffers[constantBuffer.Name] = std::move(constantBuffer);
		}

		auto resources = shaderReflectionNode["Resources"];
		for (const auto& node : resources)
		{
			ShaderReflection::Resource resource = node.as<ShaderReflection::Resource>();
			m_ReflectionData.Resources[resource.Name] = std::move(resource);
		}

		return true;
	}

	void DirectXShaderCompiler::ReflectAllShaderStages(const std::unordered_map<ShaderUtils::ShaderStage::Type, std::vector<uint32_t>>& spirvData)
	{
		for (const auto& [stage, spirvBinary] : spirvData)
		{
			try
			{
				ReflectShaderStage(stage, spirvBinary);
			}
			catch (const spirv_cross::CompilerError& error)
			{
				SK_CORE_ERROR_TAG("Renderer", "Failed to reflect shader stage!\n\tStage: {}\n\tError Message: {}", stage, error.what());
			}
		}
	}

	void DirectXShaderCompiler::ReflectShaderStage(ShaderUtils::ShaderStage::Type stage, const std::vector<uint32_t>& spirvBinary)
	{
		SK_CORE_TRACE_TAG("Renderer", "===========================");
		SK_CORE_TRACE_TAG("Renderer", " DirectX Shader Reflection");
		SK_CORE_TRACE_TAG("Renderer", "===========================");

		spirv_cross::Compiler compiler(spirvBinary);
		spirv_cross::ShaderResources shaderResources = compiler.get_shader_resources();

		const auto& metadata = m_ShaderStageMetadata.at(stage);
		auto overriddenUpdateFruequencies = GetUpdateFrequencies(metadata);

		SK_CORE_TRACE_TAG("Renderer", "Constant Buffers:");
		for (const auto& constantBuffer : shaderResources.uniform_buffers)
		{
			const spirv_cross::SPIRType& bufferType = compiler.get_type(constantBuffer.type_id);
			std::string name = compiler.get_name(constantBuffer.id);
			if (name.empty())
				name = constantBuffer.name;

			uint32_t size = (uint32_t)compiler.get_declared_struct_size(bufferType);
			uint32_t memberCount = (uint32_t)bufferType.member_types.size();
			uint32_t binding = compiler.get_decoration(constantBuffer.id, spv::DecorationBinding);

			auto& data = m_ReflectionData.ConstantBuffers[name];
			data.Name = name;
			data.Stage = utils::ToShaderReflectionShaderStage(stage);
			data.UpdateFrequency = overriddenUpdateFruequencies.contains(binding) ? overriddenUpdateFruequencies.at(binding) : utils::GetUpdateFrequencyFromBinding(binding);
			data.Binding = binding;
			data.Size = size;
			data.MemberCount = memberCount;

			SK_CORE_TRACE_TAG("Renderer", "  Name: {}", name);
			SK_CORE_TRACE_TAG("Renderer", "  UpdateFrequency: {}", ToString(data.UpdateFrequency));
			SK_CORE_TRACE_TAG("Renderer", "  Binding: {}", binding);
			SK_CORE_TRACE_TAG("Renderer", "  Size: {}", size);
			SK_CORE_TRACE_TAG("Renderer", "  Members: {}", memberCount);

			uint32_t index = 0;
			uint32_t offset = 0;
			for (const auto& member : bufferType.member_types)
			{
				auto& memberData = data.Members.emplace_back();
				const spirv_cross::SPIRType& memberType = compiler.get_type(member);
				const auto& memberName = compiler.get_member_name(constantBuffer.base_type_id, index);
				memberData.Name = fmt::format("{}.{}", name, memberName);
				memberData.Type = utils::GetVariableType(memberType);
				memberData.Size = (uint32_t)compiler.get_declared_struct_member_size(bufferType, index);
				memberData.Offset = offset;
				index++;
				offset += memberData.Size;

				SK_CORE_TRACE_TAG("Renderer", "   Member: {}", memberData.Name);
				SK_CORE_TRACE_TAG("Renderer", "    - Type: {}", ToString(memberData.Type));
				SK_CORE_TRACE_TAG("Renderer", "    - Size: {}", memberData.Size);
				SK_CORE_TRACE_TAG("Renderer", "    - Offset: {}", memberData.Offset);
			}

			SK_CORE_TRACE_TAG("Renderer", "-------------------");
		}

		SK_CORE_TRACE_TAG("Renderer", "Combined Sampled Images:");
		for (const auto& resource : shaderResources.sampled_images)
		{
			const auto& imageType = compiler.get_type(resource.type_id);
			const auto& name = resource.name;

			auto& reflectionData = m_ReflectionData.Resources[name];
			reflectionData.Name = name;
			reflectionData.Stage = utils::ToShaderReflectionShaderStage(stage);
			reflectionData.Binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
			reflectionData.ArraySize = imageType.array[0];

			switch (imageType.image.dim)
			{
				case spv::Dim2D: reflectionData.Type = ShaderReflection::ResourceType::Sampler2D; break;
				case spv::Dim3D: reflectionData.Type = ShaderReflection::ResourceType::Sampler3D; break;
				case spv::DimCube: reflectionData.Type = ShaderReflection::ResourceType::SamplerCube; break;
				default: SK_CORE_ASSERT(false, "Unkown Dimension"); break;
			}

			SK_CORE_TRACE_TAG("Renderer", "  Name: {}", reflectionData.Name);
			SK_CORE_TRACE_TAG("Renderer", "  Type: {}", ToString(reflectionData.Type));
			SK_CORE_TRACE_TAG("Renderer", "  Binding: {}", reflectionData.Binding);
			SK_CORE_TRACE_TAG("Renderer", "  Array Size: {}", reflectionData.ArraySize);
			SK_CORE_TRACE_TAG("Renderer", "-------------------");
		}

		SK_CORE_TRACE_TAG("Renderer", "Textures:");
		for (const auto& resource : shaderResources.separate_images)
		{
			const auto& imageType = compiler.get_type(resource.type_id);

			auto& reflectionData = m_ReflectionData.Resources[resource.name];
			reflectionData.Name = resource.name;
			reflectionData.Stage = utils::ToShaderReflectionShaderStage(stage);
			reflectionData.Binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
			reflectionData.ArraySize = imageType.array[0];

			switch (imageType.image.dim)
			{
				case spv::Dim2D: reflectionData.Type = ShaderReflection::ResourceType::Texture2D; break;
				case spv::Dim3D: reflectionData.Type = ShaderReflection::ResourceType::Texture3D; break;
				case spv::DimCube: reflectionData.Type = ShaderReflection::ResourceType::TextureCube; break;
				default: SK_CORE_ASSERT(false, "Unkown Dimension"); break;
			}

			SK_CORE_TRACE_TAG("Renderer", "  Name: {}", reflectionData.Name);
			SK_CORE_TRACE_TAG("Renderer", "  Type: {}", ToString(reflectionData.Type));
			SK_CORE_TRACE_TAG("Renderer", "  Binding: {}", reflectionData.Binding);
			SK_CORE_TRACE_TAG("Renderer", "  Array Size: {}", reflectionData.ArraySize);
			SK_CORE_TRACE_TAG("Renderer", "-------------------");

		}

		SK_CORE_TRACE_TAG("Renderer", "Samplers:");
		for (const auto& resource : shaderResources.separate_samplers)
		{
			const auto& imageType = compiler.get_type(resource.type_id);
			const auto& name = resource.name;

			auto& reflectionData = m_ReflectionData.Resources[name];
			reflectionData.Name = name;
			reflectionData.Stage = utils::ToShaderReflectionShaderStage(stage);
			reflectionData.Binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
			reflectionData.ArraySize = imageType.array[0];
			reflectionData.Type = ShaderReflection::ResourceType::Sampler;

			SK_CORE_TRACE_TAG("Renderer", "  Name: {}", name);
			SK_CORE_TRACE_TAG("Renderer", "  Type: {}", ToString(reflectionData.Type));
			SK_CORE_TRACE_TAG("Renderer", "  Binding: {}", reflectionData.Binding);
			SK_CORE_TRACE_TAG("Renderer", "  Array Size: {}", reflectionData.ArraySize);
			SK_CORE_TRACE_TAG("Renderer", "-------------------");
		}

	}

	std::map<uint32_t, Shark::ShaderReflection::UpdateFrequencyType> DirectXShaderCompiler::GetUpdateFrequencies(const Metadata& metadata)
	{
		if (metadata.SourceMetadata.empty())
			return {};

		std::map<uint32_t, ShaderReflection::UpdateFrequencyType> overriddenUpdateFruequencies;
		std::string_view overrides = metadata.SourceMetadata;

		size_t bol = 0;
		size_t eol = 0;

		const std::string_view BindingToken = "binding=";
		const std::string_view UpdateFrequencyToken = "UpdateFrequency::";

		size_t offset = 0;

		while (true)
		{
			bol = overrides.find("set", offset);
			if (bol == std::string::npos)
				break;

			eol = overrides.find_first_of("\r\n", bol);
			offset = eol;
			std::string_view line = overrides.substr(bol, eol - bol);
			size_t bindingBegin = line.find(BindingToken);
			if (bindingBegin == std::string::npos)
				continue;

			size_t bindingValueBegin = bindingBegin + BindingToken.length();
			size_t bindingValueEnd = line.find(' ', bindingValueBegin);
			std::string bindingString = std::string(line.substr(bindingValueBegin, bindingValueEnd - bindingValueBegin));

			size_t updateFrequencyBegin = line.find(UpdateFrequencyToken);
			if (updateFrequencyBegin == std::string::npos)
				continue;

			size_t updateFrequencyValueBegin = updateFrequencyBegin + UpdateFrequencyToken.length();
			size_t updateFrequencyValueEnd = line.find_first_of(" \r\n", updateFrequencyValueBegin);
			std::string updateFrequencyString = std::string(line.substr(updateFrequencyValueBegin, updateFrequencyValueEnd - updateFrequencyValueBegin));

			String::Strip(bindingString);
			String::Strip(updateFrequencyString);

			uint32_t binding = std::atoi(bindingString.c_str());
			ShaderReflection::UpdateFrequencyType updateFrequency = StringToShaderReflectionUpdateFrequency(updateFrequencyString);
			overriddenUpdateFruequencies[binding] = updateFrequency;
		}

		return overriddenUpdateFruequencies;
	}

}
