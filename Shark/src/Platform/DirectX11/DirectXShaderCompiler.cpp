#include "skpch.h"
#include "DirectXShaderCompiler.h"

#include "Shark/File/FileSystem.h"
#include "Shark/Math/Math.h"
#include "Shark/Utils/String.h"

#include "Shark/Serialization/SerializationMacros.h"

#include "Platform/Windows/WindowsUtils.h"
#include "Platform/DirectX11/ShaderUtils.h"
#include "Platform/DirectX11/DirectXShader.h"
#include "Platform/DirectX11/DirectXShaderCache.h"

#include <fmt/chrono.h>
#include <yaml-cpp/yaml.h>
#include <Shark/Utils/YAMLUtils.h>

#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_hlsl.hpp>
#include <dxc/dxcapi.h>
#include <dxc/WinAdapter.h>
#include <d3dcompiler.h>
#include <atlcomcli.h>

namespace YAML {

	template<>
	struct convert<Shark::ShaderReflection::MemberDeclaration>
	{
		static Node encode(const Shark::ShaderReflection::MemberDeclaration& memberDeclaration)
		{
			Node node(NodeType::Map);
			node.force_insert("Name", memberDeclaration.Name);
			node.force_insert("Type", memberDeclaration.Type);
			node.force_insert("Size", memberDeclaration.Size);
			node.force_insert("Offset", memberDeclaration.Offset);
			return node;
		}

		static bool decode(const Node& node, Shark::ShaderReflection::MemberDeclaration& outMemberDeclaration)
		{
			if (!node.IsMap() || node.size() != 4)
				return false;

			SK_DESERIALIZE_PROPERTY(node, "Name", outMemberDeclaration.Name, "");
			SK_DESERIALIZE_PROPERTY(node, "Type", outMemberDeclaration.Type, Shark::ShaderReflection::VariableType::None);
			SK_DESERIALIZE_PROPERTY(node, "Size", outMemberDeclaration.Size, 0);
			SK_DESERIALIZE_PROPERTY(node, "Offset", outMemberDeclaration.Offset, 0);
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
			node.force_insert("Set", resource.Set);
			node.force_insert("Binding", resource.Binding);
			node.force_insert("Stage", resource.Stage);
			node.force_insert("Type", resource.Type);
			node.force_insert("ArraySize", resource.ArraySize);
			node.force_insert("StructSize", resource.StructSize);

			node.force_insert("DXBinding", resource.DXBinding);
			node.force_insert("DXSamplerBinding", resource.DXSamplerBinding);
			return node;
		}

		static bool decode(const Node& node, Shark::ShaderReflection::Resource& outResource)
		{
			if (!node.IsMap() || node.size() != 9)
				return false;

			SK_DESERIALIZE_PROPERTY(node, "Name", outResource.Name);
			SK_DESERIALIZE_PROPERTY(node, "Set", outResource.Set);
			SK_DESERIALIZE_PROPERTY(node, "Binding", outResource.Binding);
			SK_DESERIALIZE_PROPERTY(node, "Stage", outResource.Stage);
			SK_DESERIALIZE_PROPERTY(node, "Type", outResource.Type);
			SK_DESERIALIZE_PROPERTY(node, "ArraySize", outResource.ArraySize);
			SK_DESERIALIZE_PROPERTY(node, "StructSize", outResource.StructSize);
			SK_DESERIALIZE_PROPERTY(node, "DXBinding", outResource.DXBinding);
			SK_DESERIALIZE_PROPERTY(node, "DXSamplerBinding", outResource.DXSamplerBinding);
			return true;
		}
	};

}

namespace Shark {

	namespace utils {

		static shaderc_shader_kind ToShaderCShaderKind(ShaderUtils::ShaderStage shaderStage)
		{
			switch (shaderStage)
			{
				case ShaderUtils::ShaderStage::Vertex: return shaderc_vertex_shader;
				case ShaderUtils::ShaderStage::Pixel: return shaderc_fragment_shader;
				case ShaderUtils::ShaderStage::Compute: return shaderc_compute_shader;
			}

			SK_CORE_ASSERT(false, "Invalid Shader Stage");
			return (shaderc_shader_kind)0;
		}

		static std::string_view ShaderStageExtension(ShaderUtils::ShaderStage stage)
		{
			switch (stage)
			{
				case ShaderUtils::ShaderStage::Vertex: return ".vert";
				case ShaderUtils::ShaderStage::Pixel: return ".pixl";
				case ShaderUtils::ShaderStage::Compute: return ".comp";
			}

			SK_CORE_ASSERT(false, "Invalid Shader Stage");
			return "";
		}

		static ShaderUtils::ShaderStage GetShaderStage(const std::string& stage)
		{
			if (String::Compare(stage, "vertex", String::Case::Ignore)) return ShaderUtils::ShaderStage::Vertex;
			if (String::Compare(stage, "vert", String::Case::Ignore)) return ShaderUtils::ShaderStage::Vertex;

			if (String::Compare(stage, "pixel", String::Case::Ignore)) return ShaderUtils::ShaderStage::Pixel;
			if (String::Compare(stage, "frag", String::Case::Ignore)) return ShaderUtils::ShaderStage::Pixel;
			if (String::Compare(stage, "fragment", String::Case::Ignore)) return ShaderUtils::ShaderStage::Pixel;

			if (String::Compare(stage, "compute", String::Case::Ignore)) return ShaderUtils::ShaderStage::Compute;

			SK_CORE_ASSERT(false, "Unkown Shader Stage");
			return ShaderUtils::ShaderStage::None;
		}

		static ShaderUtils::ShaderLanguage GetLanguageFromFileExtension(const std::filesystem::path& shaderSourcePath)
		{
			auto extension = shaderSourcePath.extension().string();

			if (String::Compare(extension, ".hlsl", String::Case::Ignore))
				return ShaderUtils::ShaderLanguage::HLSL;

			if (String::Compare(extension, ".glsl", String::Case::Ignore))
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

		static const char* GetTargetProfileForD3D11(ShaderUtils::ShaderStage stage)
		{
			switch (stage)
			{
				case ShaderUtils::ShaderStage::Vertex: return "vs_5_0";
				case ShaderUtils::ShaderStage::Pixel: return "ps_5_0";
				case ShaderUtils::ShaderStage::Compute: return "cs_5_0";
			}

			SK_CORE_ASSERT(false, "Unkown Shader Stage");
			return nullptr;
		}

		static const wchar_t* GetTargetProfileForDXC(ShaderUtils::ShaderStage stage)
		{
			switch (stage)
			{
				case ShaderUtils::ShaderStage::Vertex: return L"vs_6_0";
				case ShaderUtils::ShaderStage::Pixel: return L"ps_6_0";
				case ShaderUtils::ShaderStage::Compute: return L"cs_6_0";
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

		static std::string_view GetCacheFileExtension(ShaderUtils::ShaderStage stage)
		{
			switch (stage)
			{
				case ShaderUtils::ShaderStage::Vertex: return ".vert";
				case ShaderUtils::ShaderStage::Pixel: return ".frag";
				case ShaderUtils::ShaderStage::Compute: return ".comp";
			}

			SK_CORE_VERIFY(false);
			return "";
		}

		static std::string GetDirectXCacheFile(const std::string& fileName, ShaderUtils::ShaderStage stage)
		{
			return fmt::format("{0}/{1}{2}", utils::GetDirectXCacheDirectory(), fileName, utils::GetCacheFileExtension(stage));
		}

		static ShaderReflection::ShaderStage ToShaderReflectionShaderStage(ShaderUtils::ShaderStage stage)
		{
			switch (stage)
			{
				case ShaderUtils::ShaderStage::None: return ShaderReflection::ShaderStage::None;
				case ShaderUtils::ShaderStage::Vertex: return ShaderReflection::ShaderStage::Vertex;
				case ShaderUtils::ShaderStage::Pixel: return ShaderReflection::ShaderStage::Pixel;
				case ShaderUtils::ShaderStage::Compute: return ShaderReflection::ShaderStage::Compute;
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

	}


	DirectXShaderCompiler::DirectXShaderCompiler(const std::filesystem::path& shaderSourcePath, bool disableOptimization)
		: m_ShaderSourcePath(shaderSourcePath)
	{
		m_Language = utils::GetLanguageFromFileExtension(shaderSourcePath);
		m_Options.DisableOptimization = disableOptimization;
		m_Hash = Hash::GenerateFNV(m_ShaderSourcePath.filename().string());
	}

	DirectXShaderCompiler::DirectXShaderCompiler(const std::filesystem::path& shaderSourcePath, const DirectXShaderCompilerOptions& options)
		: m_ShaderSourcePath(shaderSourcePath), m_Options(options)
	{
		m_Language = utils::GetLanguageFromFileExtension(shaderSourcePath);
		m_Hash = Hash::GenerateFNV(m_ShaderSourcePath.filename().string());
	}

	bool DirectXShaderCompiler::Reload(bool forceCompile)
	{
		m_Options.DisableOptimization = true;
		m_ShaderSource.clear();
		m_SPIRVData.clear();
		m_ShaderBinary.clear();
		m_ReflectionData = {};

		utils::CreateChacheDirectoryIfNeeded();

		std::string source = FileSystem::ReadString(m_ShaderSourcePath);
		m_ShaderSource = PreProcess(source);

		SK_CORE_INFO_TAG("Renderer", "Compiling Shader: {}", m_ShaderSourcePath);

		const ShaderUtils::ShaderStage changedStages = DirectXShaderCache::HasChanged(this);
		bool compileSucceeded = CompileOrLoadBinaries(changedStages, forceCompile);
		if (!compileSucceeded)
		{
			SK_CORE_ASSERT(false);
			return false;
		}

		if (forceCompile || changedStages != ShaderUtils::ShaderStage::None)
		{
			ReflectAllShaderStages(m_SPIRVData);
			SerializeReflectionData();
		}

		if (m_CompiledStages != ShaderUtils::ShaderStage::None)
		{
			SK_CONSOLE_INFO("Shader Compiled\n\nFile: {}\nStages: {}", m_ShaderSourcePath, m_CompiledStages);
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
		shader->SetHash(compiler->GetHash());

		return shader;
	}

	std::unordered_map<ShaderUtils::ShaderStage, std::string> DirectXShaderCompiler::PreProcess(const std::string& source)
	{
		switch (m_Language)
		{
			case ShaderUtils::ShaderLanguage::HLSL: return PreProcessHLSL(source);
			case ShaderUtils::ShaderLanguage::GLSL:	return PreProcessGLSL(source);
		}

		SK_CORE_VERIFY(false);
		return {};
	}

	std::unordered_map<ShaderUtils::ShaderStage, std::string> DirectXShaderCompiler::PreProcessHLSL(const std::string& source)
	{
		auto shaderSource = PreProcessHLSLSource(source);

		for (const auto& [stage, moduleSource] : shaderSource)
		{
			auto& metadata = m_ShaderStageMetadata[stage];
			metadata.Stage = stage;
			metadata.HashCode = Hash::GenerateFNV(moduleSource);
			metadata.CacheFile = utils::GetDirectXCacheFile(m_ShaderSourcePath.filename().string(), stage);

			m_Stages |= stage;
		}

		return shaderSource;
	}

	std::unordered_map<Shark::ShaderUtils::ShaderStage, std::string> DirectXShaderCompiler::PreProcessGLSL(const std::string& source)
	{
		auto shaderSource = PreProcessGLSLSource(source);

		for (const auto& [stage, moduleSource] : shaderSource)
		{
			auto& metadata = m_ShaderStageMetadata[stage];
			metadata.Stage = stage;
			metadata.HashCode = Hash::GenerateFNV(moduleSource);
			metadata.CacheFile = utils::GetDirectXCacheFile(m_ShaderSourcePath.filename().string(), stage);

			m_Stages |= stage;
		}

		return shaderSource;
	}

	std::unordered_map<Shark::ShaderUtils::ShaderStage, std::string> DirectXShaderCompiler::PreProcessHLSLSource(const std::string& source)
	{
		std::unordered_map<ShaderUtils::ShaderStage, std::string> shaderSource;

		const std::string_view ShaderStageToken = "#pragma stage";

		size_t offset = 0;

		while (offset != std::string::npos)
		{
			const size_t moduleBegin = source.find(ShaderStageToken, offset);
			if (moduleBegin == std::string::npos)
			{
				SK_CORE_ERROR_TAG("Renderer", "Failed to find Shader Stage!");
				return {};
			}

			const size_t moduleEnd = source.find(ShaderStageToken, offset + moduleBegin + ShaderStageToken.length());
			std::string moduleSource = source.substr(moduleBegin, moduleEnd == std::string::npos ? std::string::npos : moduleEnd - moduleBegin);

			const size_t stageBegin = moduleSource.find(ShaderStageToken);
			if (stageBegin == std::string::npos)
			{
				SK_CORE_ERROR_TAG("Renderer", "Failed to find Shader Stage!");
				return {};
			}

			const size_t stageArgBegin = moduleSource.find_first_not_of(": ", stageBegin + ShaderStageToken.length());
			const size_t stageArgEnd = moduleSource.find_first_of(" \n\r", stageArgBegin);

			std::string stageString = moduleSource.substr(stageArgBegin, stageArgEnd - stageArgBegin);
			ShaderUtils::ShaderStage stage = utils::GetShaderStage(stageString);
			offset = moduleEnd;

			for (size_t versionBegin = moduleSource.find("#version"); versionBegin != std::string::npos; versionBegin = moduleSource.find("#version"))
			{
				const size_t versionEnd = moduleSource.find_first_of("\n\r", versionBegin);
				moduleSource.erase(versionBegin, versionEnd - versionBegin);
				SK_CORE_WARN_TAG("Renderer", "The #version Preprocessor directive is depricated for shader written in HLSL");
			}

			SK_CORE_ASSERT(stage != ShaderUtils::ShaderStage::None);
			SK_CORE_ASSERT(!moduleSource.empty());
			shaderSource[stage] = moduleSource;
		}

		return shaderSource;
	}

	std::unordered_map<Shark::ShaderUtils::ShaderStage, std::string> DirectXShaderCompiler::PreProcessGLSLSource(const std::string& source)
	{
		std::unordered_map<ShaderUtils::ShaderStage, std::string> shaderSource;

		const std::string_view VersionToken = "#version";
		const std::string_view ShaderStageToken = "#pragma stage";

		size_t offset = 0;

		while (offset != std::string::npos)
		{
			const size_t moduleBegin = source.find(VersionToken, offset);
			if (moduleBegin == std::string::npos)
			{
				SK_CORE_ERROR_TAG("Renderer", "Failed to find Shader Version!");
				return {};
			}

			const size_t moduleEnd = source.find(VersionToken, offset + VersionToken.length());
			std::string moduleSource = source.substr(moduleBegin, moduleEnd == std::string::npos ? std::string::npos : moduleEnd - moduleBegin);

			const size_t stageBegin = moduleSource.find(ShaderStageToken);
			if (stageBegin == std::string::npos)
			{
				SK_CORE_ERROR_TAG("Renderer", "Failed to find Shader Stage!");
				return {};
			}

			const size_t stageArgBegin = moduleSource.find_first_not_of(": ", stageBegin + ShaderStageToken.length());
			const size_t stageArgEnd = moduleSource.find_first_of(" \n\r", stageArgBegin);
			std::string stageString = moduleSource.substr(stageArgBegin, stageArgEnd - stageArgBegin);
			ShaderUtils::ShaderStage stage = utils::GetShaderStage(stageString);
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

	bool DirectXShaderCompiler::CompileOrLoadBinaries(ShaderUtils::ShaderStage changedStages, bool forceCompile)
	{
		ScopedTimer timer("DirectXShaderCompiler::CompileOrGetBinaries");

		for (auto& [stage, source] : m_ShaderSource)
		{
			if (!CompileOrLoadBinary(stage, changedStages, forceCompile))
				return false;
		}

		if (forceCompile || changedStages != ShaderUtils::ShaderStage::None)
		{
			DirectXShaderCache::OnShaderCompiled(this);
		}
		else
		{
			ReadReflectionData();
		}

		return true;
	}

	bool DirectXShaderCompiler::CompileOrLoadBinary(ShaderUtils::ShaderStage stage, ShaderUtils::ShaderStage changedStages, bool forceCompile)
	{
		std::vector<byte>& binary = m_ShaderBinary[stage];
		std::vector<uint32_t>& spirvBinary = m_SPIRVData[stage];

		if (!forceCompile && (changedStages & stage) != stage)
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
				const std::string cacheFile = fmt::format("{0}/{1}{2}", utils::GetDirectXCacheDirectory(), m_ShaderSourcePath.filename().string(), utils::GetCacheFileExtension(stage));
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

	std::string DirectXShaderCompiler::Compile(ShaderUtils::ShaderStage stage, std::vector<byte>& outputBinary, std::vector<uint32_t>& outputSPIRVDebug)
	{
		std::string error;
		if (m_Language == ShaderUtils::ShaderLanguage::HLSL)
			error = CompileHLSLToSPIRV(stage, outputSPIRVDebug);
		else
			error = CompileGLSLToSPIRV(stage, outputSPIRVDebug);

		if (!error.empty())
			return error;

		std::string hlslSource = CrossCompileToHLSL(stage, outputSPIRVDebug);

		error = CompileHLSL(stage, hlslSource.size() ? hlslSource : m_ShaderSource.at(stage), outputBinary);
		m_CompiledStages |= stage;
		return error;
	}

	class HLSLIncludeHandler : public IDxcIncludeHandler
	{
	public:
		HLSLIncludeHandler(CComPtr<IDxcIncludeHandler> defaultIncludeHandler)
			: m_DefaultIncludeHandler(defaultIncludeHandler)
		{}

		virtual HRESULT STDMETHODCALLTYPE LoadSource(_In_z_ LPCWSTR pFilename, _COM_Outptr_result_maybenull_ IDxcBlob** ppIncludeSource)
		{
			std::error_code error;
			auto filepath = std::filesystem::canonical(std::filesystem::path(L"Resources/Shaders") / pFilename, error);
			if (error)
				return E_INVALIDARG;

			return m_DefaultIncludeHandler->LoadSource(filepath.c_str(), ppIncludeSource);
		}

		virtual HRESULT QueryInterface(REFIID riid, void** ppvObject) override { return S_FALSE; }
		virtual ULONG AddRef() override { return 0; }
		virtual ULONG Release() override { return 0; }
	private:
		CComPtr<IDxcIncludeHandler> m_DefaultIncludeHandler;
	};

	std::string DirectXShaderCompiler::CompileHLSLToSPIRV(ShaderUtils::ShaderStage stage, std::vector<uint32_t>& outputSPIRVDebug)
	{
#define CREATE_ERROR_MSG(_base_message, _hResult) fmt::format(_base_message "\n\tHResult: ({}): {}", _hResult, WindowsUtils::TranslateHResult(_hResult))
#define VERIFY_HRESULT(_hResult, _message) if (FAILED(_hResult)) return CREATE_ERROR_MSG(_message, _hResult);
		using ATL::CComPtr;

		HRESULT hResult;

		CComPtr<IDxcCompiler3> compiler;
		hResult = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
		VERIFY_HRESULT(hResult, "Failed to create DXC Compiler");

		CComPtr<IDxcUtils> utils;
		hResult = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
		VERIFY_HRESULT(hResult, "Failed to create DXC Utils");

		CComPtr<IDxcBlobEncoding> sourceBlob;
		hResult = utils->CreateBlob(m_ShaderSource.at(stage).data(), (UINT32)m_ShaderSource.at(stage).size(), DXC_CP_ACP, &sourceBlob);
		VERIFY_HRESULT(hResult, "Failed to create DXC Blob");

		CComPtr<IDxcIncludeHandler> defaultIncludeHandler;
		utils->CreateDefaultIncludeHandler(&defaultIncludeHandler);

#undef VERIFY_HRESULT
#undef CREATE_ERROR_MSG

		const wchar_t* version = utils::GetTargetProfileForDXC(stage);
		std::wstring filename = m_ShaderSourcePath.stem().wstring();

		std::vector<LPCWSTR> arguments = {
			filename.c_str(),

			L"-E", L"main",
			L"-T", version,
			L"-spirv",

			L"-fspv-reflect",
			L"-fspv-target-env=vulkan1.2",
			//L"-fspv-preserve-bindings",
			//L"-fspv-preserve-interface"
		};

		arguments.emplace_back(L"-Qembed_debug");
		arguments.emplace_back(DXC_ARG_DEBUG);

		if (!m_Options.DisableOptimization)
			arguments.emplace_back(DXC_ARG_OPTIMIZATION_LEVEL3);

		DxcBuffer buffer = {};
		buffer.Encoding = DXC_CP_ACP;
		buffer.Ptr = sourceBlob->GetBufferPointer();
		buffer.Size = sourceBlob->GetBufferSize();

		HLSLIncludeHandler includeHandler(defaultIncludeHandler);

		CComPtr<IDxcResult> result = nullptr;
		hResult = compiler->Compile(&buffer, arguments.data(), (UINT32)arguments.size(),
									&includeHandler, IID_PPV_ARGS(&result));

		result->GetStatus(&hResult);
		if (FAILED(hResult))
		{
			CComPtr<IDxcBlobEncoding> errorBlob;
			hResult = result->GetErrorBuffer(&errorBlob);
			return fmt::format("Failed to compile shader!\n\t{}", (const char*)errorBlob->GetBufferPointer());
		}

		CComPtr<IDxcBlob> code;
		result->GetResult(&code);

		outputSPIRVDebug.resize(code->GetBufferSize() / sizeof(uint32_t));
		Buffer writeBuffer = Buffer::FromArray(outputSPIRVDebug);
		writeBuffer.Write((const void*)code->GetBufferPointer(), code->GetBufferSize());

		return {};
	}

	std::string DirectXShaderCompiler::CompileGLSLToSPIRV(ShaderUtils::ShaderStage stage, std::vector<uint32_t>& outputSPIRVDebug)
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
		return {};
	}

	std::string DirectXShaderCompiler::CompileFromSPIRV(ShaderUtils::ShaderStage stage, const std::vector<uint32_t>& spirvBinary, std::vector<byte>& outputBinary)
	{
		std::string hlslSourceCode = CrossCompileToHLSL(stage, spirvBinary);
		if (hlslSourceCode.empty())
			return "Cross Compiling SPIRV to HLSL Failed";

		return CompileHLSL(stage, hlslSourceCode, outputBinary);
	}

	std::string DirectXShaderCompiler::CompileHLSL(ShaderUtils::ShaderStage stage, const std::string& hlslSourceCode, std::vector<byte>& outputBinary) const
	{
		UINT flags = 0;
		flags |= D3DCOMPILE_ALL_RESOURCES_BOUND;
		//flags |= D3DCOMPILE_WARNINGS_ARE_ERRORS;

		flags |= D3DCOMPILE_DEBUG;
		if (m_Options.DisableOptimization)
		{
			flags |= D3DCOMPILE_OPTIMIZATION_LEVEL0;
			flags |= D3DCOMPILE_SKIP_OPTIMIZATION;
		}
		else
			flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;

		ID3DBlob* shaderBinary = nullptr;
		ID3DBlob* errorMessage = nullptr;
		std::string name = fmt::format("{}{}", m_ShaderSourcePath.stem().string(), utils::ShaderStageExtension(stage));
		if (FAILED(D3DCompile(hlslSourceCode.data(), hlslSourceCode.size(), name.c_str(), nullptr, nullptr, "main", utils::GetTargetProfileForD3D11(stage), flags, 0, &shaderBinary, &errorMessage)))
		{
			std::string errorStr = fmt::format("Failed to Compile HLSL (Stage: {0})\n{1}", stage, (char*)errorMessage->GetBufferPointer());
			errorMessage->Release();
			return errorStr;
		}

		outputBinary.resize(shaderBinary->GetBufferSize());
		memcpy(outputBinary.data(), shaderBinary->GetBufferPointer(), shaderBinary->GetBufferSize());
		shaderBinary->Release();
		return {};
	}

	std::string DirectXShaderCompiler::CrossCompileToHLSL(ShaderUtils::ShaderStage stage, const std::vector<uint32_t>& spirvBinary)
	{
		spirv_cross::CompilerHLSL compilerHLSL(spirvBinary);
		spirv_cross::CompilerHLSL::Options options;
		options.shader_model = 50;

		// TODO(moro): try out this options
		//options.preserve_structured_buffers = true;

		compilerHLSL.set_hlsl_options(options);
		compilerHLSL.set_resource_binding_flags(spirv_cross::HLSL_BINDING_AUTO_ALL);

		if (stage == ShaderUtils::ShaderStage::Vertex)
		{
			spirv_cross::ShaderResources shaderResources = compilerHLSL.get_shader_resources();
			for (const auto& resource : shaderResources.stage_inputs)
			{
				std::string name = resource.name.substr(m_Language == ShaderUtils::ShaderLanguage::HLSL ? 7 : 2);
				uint32_t location = compilerHLSL.get_decoration(resource.id, spv::DecorationLocation);
				compilerHLSL.add_vertex_attribute_remap({ location, name });
			}
		}

		try
		{
			return compilerHLSL.compile();
		}
		catch (spirv_cross::CompilerError e)
		{
			SK_CORE_ERROR_TAG("Renderer", "{}", e.what());
			return {};
		}
	}

	void DirectXShaderCompiler::SerializeDirectX(ShaderUtils::ShaderStage stage, const std::vector<byte>& directXData)
	{
		const std::string cacheFile = fmt::format("{0}/{1}{2}", utils::GetDirectXCacheDirectory(), m_ShaderSourcePath.filename().string(), utils::GetCacheFileExtension(stage));
		if (!FileSystem::WriteBinary(cacheFile, Buffer::FromArray(directXData)))
		{
			SK_CORE_ERROR_TAG("Renderer", "Failed to Serialize DirectX Data!");
			return;
		}

		m_StagesWrittenToCache |= stage;
	}

	void DirectXShaderCompiler::TryLoadDirectX(ShaderUtils::ShaderStage stage, std::vector<byte>& directXData)
	{
		const std::string cacheFile = fmt::format("{0}/{1}{2}", utils::GetDirectXCacheDirectory(), m_ShaderSourcePath.filename().string(), utils::GetCacheFileExtension(stage));
		Buffer fileData = FileSystem::ReadBinary(cacheFile);
		if (!fileData)
			return;

		fileData.CopyTo(directXData);
		fileData.Release();
		return;
	}

	void DirectXShaderCompiler::SerializeSPIRV(ShaderUtils::ShaderStage stage, const std::vector<uint32_t>& spirvData)
	{
		const std::string cacheFile = fmt::format("{}/{}{}", utils::GetSPIRVCacheDirectory(), m_ShaderSourcePath.filename().string(), utils::GetCacheFileExtension(stage));
		if (!FileSystem::WriteBinary(cacheFile, Buffer::FromArray(spirvData)))
		{
			SK_CORE_ERROR_TAG("Renderer", "Failed to Serialize SPIRV Data");
			return;
		}
	}

	void DirectXShaderCompiler::TryLoadSPIRV(ShaderUtils::ShaderStage stage, std::vector<uint32_t>& outputSPIRVData)
	{
		const std::string cacheFile = fmt::format("{}/{}{}", utils::GetSPIRVCacheDirectory(), m_ShaderSourcePath.filename().string(), utils::GetCacheFileExtension(stage));
		Buffer fileData = FileSystem::ReadBinary(cacheFile);
		if (!fileData)
			return;

		fileData.CopyTo(outputSPIRVData);
		fileData.Release();
	}

	void DirectXShaderCompiler::TryLoadDirectXAndSPIRV(ShaderUtils::ShaderStage stage, std::vector<byte>& outputDirectXData, std::vector<uint32_t>& outputSPIRVData)
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
		out << YAML::Key << "Resources" << YAML::Value << YAML::Node(m_ReflectionData.Resources);
		out << YAML::Key << "Members" << YAML::Value << YAML::Node(m_ReflectionData.Members);
		
		if (m_ReflectionData.HasPushConstant)
		{
			out << YAML::Key << "PushConstant" << YAML::Value << YAML::Node(m_ReflectionData.PushConstant);
			out << YAML::Key << "PushConstantMembers" << YAML::Value << YAML::Node(m_ReflectionData.PushConstantMembers);
		}

		out << YAML::Key << "NameCache" << YAML::Value << YAML::Node(m_ReflectionData.NameCache);
		out << YAML::Key << "MemberNameCache" << YAML::Value << YAML::Node(m_ReflectionData.MemberNameCache);

		out << YAML::EndMap;
		out << YAML::EndMap;

		const std::string cacheFile = fmt::format("{0}/{1}.yaml", utils::GetReflectionDataCacheDirectory(), m_ShaderSourcePath.filename().string());
		FileSystem::WriteString(cacheFile, out.c_str());
	}

	bool DirectXShaderCompiler::ReadReflectionData()
	{
		const std::string cacheFile = fmt::format("{0}/{1}.yaml", utils::GetReflectionDataCacheDirectory(), m_ShaderSourcePath.filename().string());
		std::string fileContent = FileSystem::ReadString(cacheFile);
		if (fileContent.empty())
			return false;

		YAML::Node masterNode = YAML::Load(fileContent);
		YAML::Node shaderReflectionNode = masterNode["ShaderReflection"];
		if (!shaderReflectionNode)
			return false;

		m_ReflectionData.Resources = shaderReflectionNode["Resources"].as<decltype(m_ReflectionData.Resources)>();
		m_ReflectionData.Members = shaderReflectionNode["Members"].as<decltype(m_ReflectionData.Members)>();

		if (shaderReflectionNode["PushConstant"])
		{
			m_ReflectionData.HasPushConstant = true;
			m_ReflectionData.PushConstant = shaderReflectionNode["PushConstant"].as<ShaderReflection::Resource>();
			m_ReflectionData.PushConstantMembers = shaderReflectionNode["PushConstantMembers"].as<ShaderReflection::MemberList>();
		}

		m_ReflectionData.NameCache = shaderReflectionNode["NameCache"].as<decltype(m_ReflectionData.NameCache)>();
		m_ReflectionData.MemberNameCache = shaderReflectionNode["MemberNameCache"].as<decltype(m_ReflectionData.MemberNameCache)>();

		return true;
	}

	void DirectXShaderCompiler::ReflectAllShaderStages(const std::unordered_map<ShaderUtils::ShaderStage, std::vector<uint32_t>>& spirvData)
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

	void DirectXShaderCompiler::ReflectShaderStage(ShaderUtils::ShaderStage stage, const std::vector<uint32_t>& spirvBinary)
	{
		SK_CORE_TRACE_TAG("Renderer", "===========================");
		SK_CORE_TRACE_TAG("Renderer", " DirectX Shader Reflection ");
		SK_CORE_TRACE_TAG("Renderer", "{0:^27}", stage);
		SK_CORE_TRACE_TAG("Renderer", "===========================");

		spirv_cross::Compiler compiler(spirvBinary);
		spirv_cross::ShaderResources shaderResources = compiler.get_shader_resources();

		const auto& binary = m_ShaderBinary.at(stage);
		ID3D11ShaderReflection* d3dReflector = nullptr;
		HRESULT hResult = D3DReflect(binary.data(), binary.size(), IID_ID3D11ShaderReflection, (void**)&d3dReflector);
		SK_CORE_VERIFY(SUCCEEDED(hResult), "{} {}", hResult, WindowsUtils::TranslateHResult(hResult));

		if (stage == ShaderUtils::ShaderStage::Vertex)
		{
			SK_CORE_TRACE_TAG("Renderer", "Stage Inputs:");
			for (const auto& stageInput : shaderResources.stage_inputs)
			{
				const spirv_cross::SPIRType& inputType = compiler.get_type(stageInput.type_id);
				ShaderReflection::VariableType type = utils::GetVariableType(inputType);
				
				SK_CORE_TRACE_TAG("Renderer", "  Name: {}", stageInput.name);
				SK_CORE_TRACE_TAG("Renderer", "  Type: {}", type);
			}
		}

		SK_CORE_TRACE_TAG("Renderer", "Push Constants:");
		SK_CORE_VERIFY(shaderResources.push_constant_buffers.size() <= 1);
		for (const auto& pushConstant : shaderResources.push_constant_buffers)
		{
			const spirv_cross::SPIRType& bufferType = compiler.get_type(pushConstant.type_id);
			std::string name = compiler.get_name(pushConstant.id);
			if (name.empty())
				name = pushConstant.name;

			m_ReflectionData.HasPushConstant = true;
			ShaderReflection::Resource& resource = m_ReflectionData.PushConstant;
			resource.Name = name;
			resource.Stage = utils::ToShaderReflectionShaderStage(stage);
			resource.Type = ShaderReflection::ResourceType::PushConstant;
			resource.StructSize = (uint32_t)compiler.get_declared_struct_size(bufferType);

			uint32_t memberCount = (uint32_t)bufferType.member_types.size();

			std::string d3dName;
			if (m_Language == ShaderUtils::ShaderLanguage::HLSL)
			{
				d3dName = compiler.get_name(bufferType.self);
				String::Replace(d3dName, ".", "_");
			}
			else
				d3dName = name.substr(2);

			D3D11_SHADER_INPUT_BIND_DESC d3dInputDesc;
			HRESULT hResult = d3dReflector->GetResourceBindingDescByName(d3dName.c_str(), &d3dInputDesc);
			SK_CORE_VERIFY(SUCCEEDED(hResult), "{:x} {}", hResult, WindowsUtils::TranslateHResult(hResult));
			if (SUCCEEDED(hResult))
			{
				resource.DXBinding = d3dInputDesc.BindPoint;
			}

			SK_CORE_TRACE_TAG("Renderer", "  Name: {}", name);
			SK_CORE_TRACE_TAG("Renderer", "  DXBinding: {}", resource.DXBinding);
			SK_CORE_TRACE_TAG("Renderer", "  Size: {}", resource.StructSize);
			SK_CORE_TRACE_TAG("Renderer", "  Members: {}", memberCount);

			uint32_t index = 0;
			uint32_t offset = 0;
			auto& memberList = m_ReflectionData.PushConstantMembers;
			for (const auto& member : bufferType.member_types)
			{
				ShaderReflection::MemberDeclaration& memberData = memberList.emplace_back();
				const spirv_cross::SPIRType& memberType = compiler.get_type(member);
				const auto& memberName = compiler.get_member_name(pushConstant.base_type_id, index);
				memberData.Name = fmt::format("{}.{}", name, memberName);
				memberData.Type = utils::GetVariableType(memberType);
				memberData.Size = (uint32_t)compiler.get_declared_struct_member_size(bufferType, index);
				memberData.Offset = offset;
				index++;
				offset += memberData.Size;

				SK_CORE_TRACE_TAG("Renderer", "   Member: {}", memberData.Name);
				SK_CORE_TRACE_TAG("Renderer", "    - Type: {}", memberData.Type);
				SK_CORE_TRACE_TAG("Renderer", "    - Size: {}", memberData.Size);
				SK_CORE_TRACE_TAG("Renderer", "    - Offset: {}", memberData.Offset);
			}

			SK_CORE_TRACE_TAG("Renderer", "-------------------");
		}

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
			uint32_t set = compiler.get_decoration(constantBuffer.id, spv::DecorationDescriptorSet);

			SK_CORE_VERIFY(m_ReflectionData.Resources[set].contains(binding) == false);
			m_ReflectionData.NameCache[name] = { set, binding };
			ShaderReflection::Resource& resource = m_ReflectionData.Resources[set][binding];
			resource.Name = name;
			resource.Set = set;
			resource.Binding = binding;
			resource.Stage = utils::ToShaderReflectionShaderStage(stage);
			resource.Type = ShaderReflection::ResourceType::ConstantBuffer;
			resource.StructSize = size;

			std::string d3dName;
			if (m_Language == ShaderUtils::ShaderLanguage::HLSL)
			{
				d3dName = constantBuffer.name;
				String::Replace(d3dName, ".", "_");
			}
			else
				d3dName = name.substr(2);

			D3D11_SHADER_INPUT_BIND_DESC d3dInputDesc;
			HRESULT hResult = d3dReflector->GetResourceBindingDescByName(d3dName.c_str(), &d3dInputDesc);
			SK_CORE_ASSERT(SUCCEEDED(hResult), "{:x} {}", hResult, WindowsUtils::TranslateHResult(hResult));
			if (SUCCEEDED(hResult))
			{
				resource.DXBinding = d3dInputDesc.BindPoint;
			}

			SK_CORE_TRACE_TAG("Renderer", "  Name: {}", name);
			SK_CORE_TRACE_TAG("Renderer", "  Binding: {}.{}", set, binding);
			SK_CORE_TRACE_TAG("Renderer", "  DXBinding: {}", resource.DXBinding);
			SK_CORE_TRACE_TAG("Renderer", "  Size: {}", size);
			SK_CORE_TRACE_TAG("Renderer", "  Members: {}", memberCount);

			uint32_t index = 0;
			uint32_t offset = 0;
			SK_CORE_VERIFY(!m_ReflectionData.Members[set].contains(binding));
			auto& memberList = m_ReflectionData.Members[set][binding];
			for (const auto& member : bufferType.member_types)
			{
				SK_CORE_ASSERT(index == memberList.size());
				ShaderReflection::MemberDeclaration& memberData = memberList.emplace_back();
				const spirv_cross::SPIRType& memberType = compiler.get_type(member);
				const auto& memberName = compiler.get_member_name(constantBuffer.base_type_id, index);
				memberData.Name = fmt::format("{}.{}", name, memberName);
				memberData.Type = utils::GetVariableType(memberType);
				memberData.Size = (uint32_t)compiler.get_declared_struct_member_size(bufferType, index);
				memberData.Offset = offset;
				m_ReflectionData.MemberNameCache[memberData.Name] = { set, binding, index };
				index++;
				offset += memberData.Size;

				SK_CORE_TRACE_TAG("Renderer", "   Member: {}", memberData.Name);
				SK_CORE_TRACE_TAG("Renderer", "    - Type: {}", memberData.Type);
				SK_CORE_TRACE_TAG("Renderer", "    - Size: {}", memberData.Size);
				SK_CORE_TRACE_TAG("Renderer", "    - Offset: {}", memberData.Offset);
			}

			SK_CORE_TRACE_TAG("Renderer", "-------------------");
		}

		SK_CORE_TRACE_TAG("Renderer", "Storage Buffers:");
		for (const auto& storageBuffer : shaderResources.storage_buffers)
		{
			const spirv_cross::SPIRType& bufferType = compiler.get_type(storageBuffer.type_id);
			std::string name = compiler.get_name(storageBuffer.id);
			if (name.empty())
				name = storageBuffer.name;

			uint32_t binding = compiler.get_decoration(storageBuffer.id, spv::DecorationBinding);
			uint32_t set = compiler.get_decoration(storageBuffer.id, spv::DecorationDescriptorSet);

			SK_CORE_VERIFY(m_ReflectionData.Resources[set].contains(binding) == false);
			m_ReflectionData.NameCache[name] = { set, binding };
			auto& resource = m_ReflectionData.Resources[set][binding];
			resource.Name = name;
			resource.Set = set;
			resource.Binding = binding;
			resource.Stage = utils::ToShaderReflectionShaderStage(stage);
			resource.Type = ShaderReflection::ResourceType::StorageBuffer;
			
			auto memberStructID = bufferType.member_types[0];
			const spirv_cross::SPIRType& memberStructType = compiler.get_type(memberStructID);
			resource.StructSize = (uint32_t)compiler.get_declared_struct_size(memberStructType);

			D3D11_SHADER_INPUT_BIND_DESC d3dInputDesc;
			HRESULT hResult = d3dReflector->GetResourceBindingDescByName(name.c_str(), &d3dInputDesc);
			SK_CORE_ASSERT(SUCCEEDED(hResult), "{:x} {}", hResult, WindowsUtils::TranslateHResult(hResult));
			if (SUCCEEDED(hResult))
			{
				resource.DXBinding = d3dInputDesc.BindPoint;
			}

			SK_CORE_TRACE_TAG("Renderer", "  Name: {}", resource.Name);
			SK_CORE_TRACE_TAG("Renderer", "  Binding: {}.{}", set, binding);
			SK_CORE_TRACE_TAG("Renderer", "  DXBinding: {}", resource.DXBinding);
			SK_CORE_TRACE_TAG("Renderer", "  Size: {}", resource.StructSize);
			SK_CORE_TRACE_TAG("Renderer", "-------------------");
		}

		SK_CORE_TRACE_TAG("Renderer", "Textures:");
		for (const auto& resource : shaderResources.sampled_images)
		{
			const auto& imageType = compiler.get_type(resource.type_id);
			const auto& name = resource.name;

			uint32_t set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
			uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);

			SK_CORE_VERIFY(m_ReflectionData.Resources[set].contains(binding) == false, "Binding ({}.{}) allready in use! (Current: {})", set, binding, name);
			m_ReflectionData.NameCache[name] = { set, binding };
			auto& reflectionData = m_ReflectionData.Resources[set][binding];
			reflectionData.Name = name;
			reflectionData.Set = set;
			reflectionData.Binding = binding;
			reflectionData.Stage = utils::ToShaderReflectionShaderStage(stage);

			if (!imageType.array.empty())
				reflectionData.ArraySize = imageType.array[0];

			switch (imageType.image.dim)
			{
				case spv::Dim2D: reflectionData.Type = ShaderReflection::ResourceType::Texture2D; break;
				case spv::Dim3D: reflectionData.Type = ShaderReflection::ResourceType::Texture3D; break;
				case spv::DimCube: reflectionData.Type = ShaderReflection::ResourceType::TextureCube; break;
				default: SK_CORE_ASSERT(false, "Unkown Dimension"); break;
			}

			std::string d3dTextureName, d3dSamplerName;
			if (reflectionData.ArraySize)
			{
				d3dTextureName = fmt::format("{}[{}]", name, 0);
				d3dSamplerName = fmt::format("_{}_sampler[{}]", name, 0);
			}
			else
			{
				d3dTextureName = name;
				d3dSamplerName = fmt::format("_{}_sampler", name);
			}

			D3D11_SHADER_INPUT_BIND_DESC d3dInputDesc;
			HRESULT hResult = d3dReflector->GetResourceBindingDescByName(d3dTextureName.c_str(), &d3dInputDesc);
			SK_CORE_ASSERT(SUCCEEDED(hResult), "{:x} {}", hResult, WindowsUtils::TranslateHResult(hResult));
			if (SUCCEEDED(hResult))
			{
				reflectionData.DXBinding = d3dInputDesc.BindPoint;
			}

			hResult = d3dReflector->GetResourceBindingDescByName(d3dSamplerName.c_str(), &d3dInputDesc);
			SK_CORE_ASSERT(SUCCEEDED(hResult), "{:x} {}", hResult, WindowsUtils::TranslateHResult(hResult));
			if (SUCCEEDED(hResult))
			{
				reflectionData.DXSamplerBinding = d3dInputDesc.BindPoint;
			}


			SK_CORE_TRACE_TAG("Renderer", "  Name: {}", reflectionData.Name);
			SK_CORE_TRACE_TAG("Renderer", "  Type: {}", reflectionData.Type);
			SK_CORE_TRACE_TAG("Renderer", "  Binding: {}.{}", set, binding);
			SK_CORE_TRACE_TAG("Renderer", "  DX Binding: {}", reflectionData.DXBinding);
			SK_CORE_TRACE_TAG("Renderer", "  DX Sampler Binding: {}", reflectionData.DXSamplerBinding);
			SK_CORE_TRACE_TAG("Renderer", "  Array Size: {}", reflectionData.ArraySize);
			SK_CORE_TRACE_TAG("Renderer", "-------------------");
		}

		SK_CORE_TRACE_TAG("Renderer", "Images:");
		for (const auto& resource : shaderResources.separate_images)
		{
			const auto& imageType = compiler.get_type(resource.type_id);

			uint32_t set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
			uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);

			SK_CORE_VERIFY(m_ReflectionData.Resources[set].contains(binding) == false);
			m_ReflectionData.NameCache[resource.name] = { set, binding };
			auto& reflectionData = m_ReflectionData.Resources[set][binding];
			reflectionData.Name = resource.name;
			reflectionData.Set = set;
			reflectionData.Binding = binding;
			reflectionData.Stage = utils::ToShaderReflectionShaderStage(stage);

			if (!imageType.array.empty())
				reflectionData.ArraySize = imageType.array[0];


			switch (imageType.image.dim)
			{
				case spv::Dim2D: reflectionData.Type = ShaderReflection::ResourceType::Image2D; break;
				case spv::Dim3D: reflectionData.Type = ShaderReflection::ResourceType::Image3D; break;
				case spv::DimCube: reflectionData.Type = ShaderReflection::ResourceType::ImageCube; break;
				default: SK_CORE_ASSERT(false, "Unkown Dimension"); break;
			}

			D3D11_SHADER_INPUT_BIND_DESC d3dInputDesc;
			HRESULT hResult = d3dReflector->GetResourceBindingDescByName(reflectionData.Name.c_str(), &d3dInputDesc);
			SK_CORE_ASSERT(SUCCEEDED(hResult), "{:x} {}", hResult, WindowsUtils::TranslateHResult(hResult));
			if (SUCCEEDED(hResult))
			{
				reflectionData.DXBinding = d3dInputDesc.BindPoint;
				reflectionData.DXSamplerBinding = d3dInputDesc.BindPoint;
			}

			SK_CORE_TRACE_TAG("Renderer", "  Name: {}", reflectionData.Name);
			SK_CORE_TRACE_TAG("Renderer", "  Type: {}", reflectionData.Type);
			SK_CORE_TRACE_TAG("Renderer", "  Binding: {}.{}", set, binding);
			SK_CORE_TRACE_TAG("Renderer", "  DX Binding: {}", reflectionData.DXBinding);
			SK_CORE_TRACE_TAG("Renderer", "  DX Sampler Binding: {}", reflectionData.DXSamplerBinding);
			SK_CORE_TRACE_TAG("Renderer", "  Array Size: {}", reflectionData.ArraySize);
			SK_CORE_TRACE_TAG("Renderer", "-------------------");

		}

		SK_CORE_TRACE_TAG("Renderer", "Storage Images:");
		for (const auto& resource : shaderResources.storage_images)
		{
			const auto& imageType = compiler.get_type(resource.type_id);

			uint32_t set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
			uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);

			SK_CORE_VERIFY(m_ReflectionData.Resources[set].contains(binding) == false);
			m_ReflectionData.NameCache[resource.name] = { set, binding };
			auto& reflectionData = m_ReflectionData.Resources[set][binding];
			reflectionData.Name = resource.name;
			reflectionData.Set = set;
			reflectionData.Binding = binding;
			reflectionData.Stage = utils::ToShaderReflectionShaderStage(stage);

			if (!imageType.array.empty())
				reflectionData.ArraySize = imageType.array[0];

			switch (imageType.image.dim)
			{
				case spv::Dim2D: reflectionData.Type = ShaderReflection::ResourceType::StorageImage2D; break;
				case spv::Dim3D: reflectionData.Type = ShaderReflection::ResourceType::StorageImage3D; break;
				case spv::DimCube: reflectionData.Type = ShaderReflection::ResourceType::StorageImageCube; break;
				default: SK_CORE_ASSERT(false, "Unkown Dimension"); break;
			}

			D3D11_SHADER_INPUT_BIND_DESC d3dInputDesc;
			HRESULT hResult = d3dReflector->GetResourceBindingDescByName(reflectionData.Name.c_str(), &d3dInputDesc);
			SK_CORE_ASSERT(SUCCEEDED(hResult), "{:x} {}", hResult, WindowsUtils::TranslateHResult(hResult));
			if (SUCCEEDED(hResult))
			{
				reflectionData.DXBinding = d3dInputDesc.BindPoint;
			}

			SK_CORE_TRACE_TAG("Renderer", "  Name: {}", reflectionData.Name);
			SK_CORE_TRACE_TAG("Renderer", "  Type: {}", reflectionData.Type);
			SK_CORE_TRACE_TAG("Renderer", "  Binding: {}.{}", set, binding);
			SK_CORE_TRACE_TAG("Renderer", "  DX Binding: {}", reflectionData.DXBinding);
			SK_CORE_TRACE_TAG("Renderer", "  DX Sampler Binding: {}", reflectionData.DXSamplerBinding);
			SK_CORE_TRACE_TAG("Renderer", "  Array Size: {}", reflectionData.ArraySize);
			SK_CORE_TRACE_TAG("Renderer", "-------------------");

		}

		SK_CORE_TRACE_TAG("Renderer", "Samplers:");
		for (const auto& resource : shaderResources.separate_samplers)
		{
			const auto& imageType = compiler.get_type(resource.type_id);

			uint32_t set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
			uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);

			SK_CORE_VERIFY((m_ReflectionData.Resources.contains(set) && m_ReflectionData.Resources.at(set).contains(binding)) == false);
			m_ReflectionData.NameCache[resource.name] = { set, binding };
			auto& reflectionData = m_ReflectionData.Resources[set][binding];
			reflectionData.Name = resource.name;
			reflectionData.Set = set;
			reflectionData.Binding = binding;
			reflectionData.Stage = utils::ToShaderReflectionShaderStage(stage);

			if (!imageType.array.empty())
				reflectionData.ArraySize = imageType.array[0];

			reflectionData.Type = ShaderReflection::ResourceType::Sampler;

			D3D11_SHADER_INPUT_BIND_DESC d3dInputDesc;
			HRESULT hResult = d3dReflector->GetResourceBindingDescByName(reflectionData.Name.c_str(), &d3dInputDesc);
			SK_CORE_ASSERT(SUCCEEDED(hResult), "{:x} {}", hResult, WindowsUtils::TranslateHResult(hResult));
			if (SUCCEEDED(hResult))
			{
				reflectionData.DXBinding = d3dInputDesc.BindPoint;
				reflectionData.DXSamplerBinding = d3dInputDesc.BindPoint;
			}

			SK_CORE_TRACE_TAG("Renderer", "  Name: {}", reflectionData.Name);
			SK_CORE_TRACE_TAG("Renderer", "  Type: {}", reflectionData.Type);
			SK_CORE_TRACE_TAG("Renderer", "  Binding: {}.{}", set, binding);
			SK_CORE_TRACE_TAG("Renderer", "  DX Binding: {}", reflectionData.DXBinding);
			SK_CORE_TRACE_TAG("Renderer", "  DX Sampler Binding: {}", reflectionData.DXSamplerBinding);
			SK_CORE_TRACE_TAG("Renderer", "  Array Size: {}", reflectionData.ArraySize);
			SK_CORE_TRACE_TAG("Renderer", "-------------------");
		}

	}

}
