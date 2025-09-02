#include "skpch.h"
#include "ShaderCompiler.h"

#include "Shark/Core/Memory.h"
#include "Shark/File/FileSystem.h"
#include "Shark/Utils/String.h"

#include "Shark/Render/Renderer.h"
#include "Shark/Render/ShaderCompiler/GLSLIncluder.h"
#include "Shark/Render/ShaderCompiler/ShaderPreprocessor.h"

#include <atlcomcli.h>
#include <dxc/dxcapi.h>
#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_cross.hpp>

#if SK_WITH_DX11
	#include <d3dcompiler.h>
#endif

namespace Shark {

	struct DxcInstances
	{
		inline static ATL::CComPtr<IDxcCompiler3> Compiler = nullptr;
		inline static ATL::CComPtr<IDxcUtils> Utils = nullptr;
		inline static ATL::CComPtr<IDxcIncludeHandler> IncludeHandler = nullptr;
	};

	namespace utils {

		static ShaderLanguage ShaderLanguageFromFileExtension(const std::string extension)
		{
			if (String::Compare(extension, ".hlsl", String::Case::Ignore))
				return ShaderLanguage::HLSL;

			if (String::Compare(extension, ".glsl", String::Case::Ignore))
				return ShaderLanguage::GLSL;

			SK_CORE_ASSERT(false, "Unkown Shader Extension");
			return ShaderLanguage::None;
		}

		static shaderc_source_language GetShaderCSourceLanguage(ShaderLanguage language)
		{
			switch (language)
			{
				case ShaderLanguage::HLSL: return shaderc_source_language_hlsl;
				case ShaderLanguage::GLSL: return shaderc_source_language_glsl;
			}

			SK_CORE_ASSERT(false, "Unkown ShaderLanguage");
			return (shaderc_source_language)0;
		}

		static ShaderReflection::ShaderStage ToShaderReflectionShaderStage(nvrhi::ShaderType stage)
		{
			switch (stage)
			{
				case nvrhi::ShaderType::None: return ShaderReflection::ShaderStage::None;
				case nvrhi::ShaderType::Vertex: return ShaderReflection::ShaderStage::Vertex;
				case nvrhi::ShaderType::Pixel: return ShaderReflection::ShaderStage::Pixel;
				case nvrhi::ShaderType::Compute: return ShaderReflection::ShaderStage::Compute;
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

	}

	ShaderCompiler::ShaderCompiler(const std::filesystem::path& sourcePath, const CompilerOptions& options)
		: m_Options(options), m_D3D11Compiler(*this)
	{
		m_Info.SourcePath = FileSystem::Relative(sourcePath);
		m_Info.Language = utils::ShaderLanguageFromFileExtension(m_Info.SourcePath.extension().string());
		m_Info.ShaderID = Hash::GenerateFNV(m_Info.SourcePath.string());
	}

	Ref<ShaderCompiler> ShaderCompiler::Load(const std::filesystem::path& sourcePath, const CompilerOptions& options)
	{
		auto compiler = Ref<ShaderCompiler>::Create(sourcePath, options);
		if (!compiler->Reload())
			return nullptr;
		return compiler;
	}

	bool ShaderCompiler::Reload()
	{
		m_SourceInfo.clear();
		m_SpirvBinary.clear();

		m_Source = FileSystem::ReadString(m_Info.SourcePath);
		if (m_Source.empty())
			return false;

		Preprocess();
		
		auto& shaderCache = Renderer::GetShaderCache();

		for (const auto& [stage, sourceInfo] : m_SourceInfo)
		{
			shaderCache.UpdateOptions(sourceInfo, m_Options);

			auto cacheState = shaderCache.GetCacheState(sourceInfo);
			if (!m_Options.Force && cacheState == ShaderCacheState::UpToDate)
			{
				shaderCache.LoadBinary(sourceInfo, m_SpirvBinary[stage]);
				shaderCache.LoadD3D11(sourceInfo, m_D3D11Compiler.GetBinary(stage));
			}
			else if (!CompileStage(stage) && cacheState == ShaderCacheState::OutOfDate)
			{
				SK_CORE_WARN_TAG("Renderer", "Loading older shader version from cache");
				shaderCache.LoadBinary(sourceInfo, m_SpirvBinary[stage]);
			}

			if (!m_SpirvBinary.contains(stage) || !m_D3D11Compiler.LoadOrCompileStage(stage, shaderCache))
			{
				// At this point the compilation of this stage failed and the cache is missing.
				SK_CORE_ERROR_TAG("Renderer", "Compiling shader '{}' failed!", m_Info.SourcePath.filename());
				return false;
			}
		}

		if (m_CompiledStages == nvrhi::ShaderType::None)
			shaderCache.LoadReflection(m_Info.ShaderID, m_ReflectionData);
		else
			Reflect();

		for (const auto& [stage, binary] : m_SpirvBinary)
		{
			shaderCache.GetEntry(m_SourceInfo.at(stage)).SourcePath = m_Info.SourcePath;
			shaderCache.CacheStage(m_SourceInfo.at(stage), binary, m_D3D11Compiler.GetBinary(stage));
		}

		shaderCache.CacheReflection(m_Info.ShaderID, m_ReflectionData);
		return true;
	}

	void ShaderCompiler::ClearState()
	{
		m_Source = {};
		m_SourceInfo.clear();
		m_SpirvBinary.clear();
		m_CompiledStages = nvrhi::ShaderType::None;
		m_ReflectionData = {};

		m_D3D11Compiler.ClearState();
	}

	void ShaderCompiler::Preprocess()
	{
		PreProcessorResult result;

		switch (m_Info.Language)
		{
			case ShaderLanguage::HLSL:
			{
				HLSLPreprocessor preprocessor;
				result = preprocessor.Preprocess(m_Source);
				break;
			}
			case ShaderLanguage::GLSL:
			{
				GLSLPreprocssor preprocessor;
				result = preprocessor.Preprocess(m_Source);
				break;
			}
		}

		for (auto& [stage, source] : result)
		{
			m_SourceInfo[stage] = {
				.Stage = stage,
				.HashCode = Hash::GenerateFNV(source),
				.ShaderID = m_Info.ShaderID,
				.Source = std::move(source)
			};
		}
	}

	bool ShaderCompiler::CompileStage(nvrhi::ShaderType stage)
	{
		std::string errorMessage;
		switch (m_Info.Language)
		{
			case ShaderLanguage::HLSL: errorMessage = HLSLCompileStage(stage); break;
			case ShaderLanguage::GLSL: errorMessage = GLSLCompileStage(stage); break;
		}

		if (!errorMessage.empty())
		{
			SK_CORE_ERROR_TAG("Renderer", "Failed to compile {} shader '{}'.\n{}", stage, m_Info.SourcePath, errorMessage);
			return false;
		}

#if 0
		errorMessage = m_D3D11Compiler.CompileStage(stage);
		if (!errorMessage.empty())
		{
			SK_CORE_ERROR_TAG("Renderer", "Failed to compile {} shader '{}'.\n{}", stage, m_Info.SourcePath, errorMessage);
			return false;
		}
#endif

		m_CompiledStages |= stage;
		return true;
	}

	std::string ShaderCompiler::HLSLCompileStage(nvrhi::ShaderType stage)
	{
		const ShaderSourceInfo& sourceInfo = m_SourceInfo.at(stage);

		if (!DxcInstances::Compiler)
		{
			DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&DxcInstances::Compiler));
			DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&DxcInstances::Utils));
			DxcInstances::Utils->CreateDefaultIncludeHandler(&DxcInstances::IncludeHandler);
		}

		using ATL::CComPtr;

		CComPtr<IDxcBlobEncoding> sourceBlob;
		DxcInstances::Utils->CreateBlob(sourceInfo.Source.data(), (UINT32)sourceInfo.Source.size(), DXC_CP_ACP, &sourceBlob);


		const wchar_t* version = s_ShaderTypeMappings.at(stage).DXC;
		std::wstring filename = m_Info.SourcePath.stem().wstring();

		std::vector<LPCWSTR> arguments = {
			filename.c_str(),

			L"-I Resources/Shaders/",

			L"-E", L"main",
			L"-T", version,
			L"-spirv",

			L"-fspv-reflect",
			L"-fspv-target-env=vulkan1.2",
			//L"-fspv-preserve-bindings",
			//L"-fspv-preserve-interface"
		};

		if (m_Options.GenerateDebugInfo)
		{
			arguments.emplace_back(L"-Qembed_debug");
			arguments.emplace_back(DXC_ARG_DEBUG);
		}

		if (m_Options.Optimize)
			arguments.emplace_back(DXC_ARG_OPTIMIZATION_LEVEL3);

		DxcBuffer buffer = {};
		buffer.Encoding = DXC_CP_ACP;
		buffer.Ptr = sourceBlob->GetBufferPointer();
		buffer.Size = sourceBlob->GetBufferSize();

		CComPtr<IDxcResult> result = nullptr;
		HRESULT hResult = DxcInstances::Compiler->Compile(&buffer, arguments.data(), (UINT32)arguments.size(),
														  DxcInstances::IncludeHandler, IID_PPV_ARGS(&result));

		result->GetStatus(&hResult);
		if (FAILED(hResult))
		{
			CComPtr<IDxcBlobEncoding> errorBlob;
			hResult = result->GetErrorBuffer(&errorBlob);
			return (const char*)errorBlob->GetBufferPointer();
		}

		CComPtr<IDxcBlob> code;
		result->GetResult(&code);

		auto& spirvBinary = m_SpirvBinary[stage];
		Memory::Write(spirvBinary, code->GetBufferPointer(), code->GetBufferSize());

		return {};
	}

	std::string ShaderCompiler::GLSLCompileStage(nvrhi::ShaderType stage)
	{
		const ShaderSourceInfo& sourceInfo = m_SourceInfo.at(stage);

		shaderc::Compiler compiler;
		shaderc::CompileOptions shadercOptions;

		shadercOptions.SetSourceLanguage(utils::GetShaderCSourceLanguage(m_Info.Language));
		shadercOptions.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
		shadercOptions.SetGenerateDebugInfo();

		if (m_Options.Optimize)
			shadercOptions.SetOptimizationLevel(shaderc_optimization_level_performance);

		shadercOptions.SetIncluder(std::make_unique<FileIncluder>());

		std::string name = fmt::format("{}{}", m_Info.SourcePath.stem().string(), s_ShaderTypeMappings.at(stage).Extension);

		const shaderc::PreprocessedSourceCompilationResult preprocessedSourceResult = compiler.PreprocessGlsl(sourceInfo.Source, s_ShaderTypeMappings.at(stage).ShaderC, name.c_str(), shadercOptions);
		if (preprocessedSourceResult.GetCompilationStatus() != shaderc_compilation_status_success)
			return preprocessedSourceResult.GetErrorMessage();

		std::string preprocessedSource = std::string(preprocessedSourceResult.cbegin(), preprocessedSourceResult.cend());
		const shaderc::SpvCompilationResult compiledModule = compiler.CompileGlslToSpv(preprocessedSource, s_ShaderTypeMappings.at(stage).ShaderC, name.c_str(), shadercOptions);
		//utils::EraseExtensions(preprocessedSource);

		if (compiledModule.GetCompilationStatus() != shaderc_compilation_status_success)
		{
			return compiledModule.GetErrorMessage();
		}

		m_SpirvBinary[stage] = { compiledModule.begin(), compiledModule.end() };
		return {};
	}

	void ShaderCompiler::Reflect()
	{
		for (const auto& [stage, binary] : m_SpirvBinary)
		{
			ReflectStage(stage);
			m_D3D11Compiler.ReflectStage(stage);
		}

		BuildNameCache();
	}

	void ShaderCompiler::ReflectStage(nvrhi::ShaderType stage)
	{
		const auto& spirvBinary = m_SpirvBinary.at(stage);
		spirv_cross::Compiler compiler(spirvBinary.data(), spirvBinary.size());
		spirv_cross::ShaderResources shaderResources = compiler.get_shader_resources();

		m_CurrentStage = stage;
		m_CurrentCompiler = &compiler;

		for (const auto& pushConstant : shaderResources.push_constant_buffers)
		{
			const auto& type = compiler.get_type(pushConstant.type_id);
			const auto& name = compiler.get_name(pushConstant.id);
			SK_CORE_VERIFY(!name.empty());

			m_ReflectionData.HasPushConstant = true;
			m_ReflectionData.PushConstant.Name = name;
			m_ReflectionData.PushConstant.Stage = utils::ToShaderReflectionShaderStage(stage);
			m_ReflectionData.PushConstant.Type = ShaderReflection::ResourceType::PushConstant;
			m_ReflectionData.PushConstant.StructSize = (uint32_t)compiler.get_declared_struct_size(type);

			ReflectMembers(pushConstant, m_ReflectionData.PushConstantMembers);
		}

		for (const auto& constantBuffer : shaderResources.uniform_buffers)
		{
			const auto& type = compiler.get_type(constantBuffer.type_id);
			const auto& name = compiler.get_name(constantBuffer.id);
			SK_CORE_VERIFY(!name.empty());

			const uint32_t set = compiler.get_decoration(constantBuffer.id, spv::DecorationDescriptorSet);
			const uint32_t binding = compiler.get_decoration(constantBuffer.id, spv::DecorationBinding);
			SK_CORE_VERIFY(m_ReflectionData.Resources[set].contains(binding) == false, "Binding ({}.{}) allready in use! (Current: {})", set, binding, name);

			auto& resource = m_ReflectionData.Resources[set][binding];
			resource.Name = name;
			resource.Set = set;
			resource.Binding = binding;
			resource.Stage = utils::ToShaderReflectionShaderStage(stage);
			resource.Type = ShaderReflection::ResourceType::ConstantBuffer;
			resource.StructSize = (uint32_t)compiler.get_declared_struct_size(type);

			ReflectMembers(constantBuffer, m_ReflectionData.Members[set][binding]);
		}

		for (const auto& storageBuffer : shaderResources.storage_buffers)
		{
			const auto& type = compiler.get_type(storageBuffer.type_id);
			const auto& name = compiler.get_name(storageBuffer.id);
			SK_CORE_VERIFY(!name.empty());

			const uint32_t set = compiler.get_decoration(storageBuffer.id, spv::DecorationDescriptorSet);
			const uint32_t binding = compiler.get_decoration(storageBuffer.id, spv::DecorationBinding);
			SK_CORE_VERIFY(m_ReflectionData.Resources[set].contains(binding) == false, "Binding ({}.{}) allready in use! (Current: {})", set, binding, name);

			auto& resource = m_ReflectionData.Resources[set][binding];
			resource.Name = name;
			resource.Set = set;
			resource.Binding = binding;
			resource.Stage = utils::ToShaderReflectionShaderStage(stage);
			resource.Type = ShaderReflection::ResourceType::StorageBuffer;

			const auto& structType = compiler.get_type(type.member_types[0]);
			resource.StructSize = (uint32_t)compiler.get_declared_struct_size(structType);
		}

		for (const auto& resource : shaderResources.sampled_images)
		{
			auto& reflectionData = ReflectResource(resource);

			const auto& type = m_CurrentCompiler->get_type(resource.type_id);
			switch (type.image.dim)
			{
				case spv::Dim2D: reflectionData.Type = ShaderReflection::ResourceType::Texture2D; break;
				case spv::Dim3D: reflectionData.Type = ShaderReflection::ResourceType::Texture3D; break;
				case spv::DimCube: reflectionData.Type = ShaderReflection::ResourceType::TextureCube; break;
				default: SK_CORE_ASSERT(false, "Unkown Dimension"); break;
			}
		}

		for (const auto& resource : shaderResources.separate_images)
		{
			auto& reflectionData = ReflectResource(resource);

			const auto& type = m_CurrentCompiler->get_type(resource.type_id);
			switch (type.image.dim)
			{
				case spv::Dim2D: reflectionData.Type = ShaderReflection::ResourceType::Image2D; break;
				case spv::Dim3D: reflectionData.Type = ShaderReflection::ResourceType::Image3D; break;
				case spv::DimCube: reflectionData.Type = ShaderReflection::ResourceType::ImageCube; break;
				default: SK_CORE_ASSERT(false, "Unkown Dimension"); break;
			}
		}

		for (const auto& resource : shaderResources.storage_images)
		{
			auto& reflectionData = ReflectResource(resource);

			const auto& type = m_CurrentCompiler->get_type(resource.type_id);
			switch (type.image.dim)
			{
				case spv::Dim2D: reflectionData.Type = ShaderReflection::ResourceType::StorageImage2D; break;
				case spv::Dim3D: reflectionData.Type = ShaderReflection::ResourceType::StorageImage3D; break;
				case spv::DimCube: reflectionData.Type = ShaderReflection::ResourceType::StorageImageCube; break;
				default: SK_CORE_ASSERT(false, "Unkown Dimension"); break;
			}
		}

		for (const auto& resource : shaderResources.separate_samplers)
		{
			auto& reflectionData = ReflectResource(resource);

			reflectionData.Type = ShaderReflection::ResourceType::Sampler;
		}

		m_CurrentStage = nvrhi::ShaderType::None;
		m_CurrentCompiler = nullptr;
	}

	void ShaderCompiler::BuildNameCache()
	{
		for (const auto& [set, bindings] : m_ReflectionData.Resources)
		{
			for (const auto& [binding, resource] : bindings)
			{
				m_ReflectionData.NameCache[resource.Name] = { resource.Set, resource.Binding };

				if (resource.Type == ShaderReflection::ResourceType::ConstantBuffer)
				{
					uint32_t index = 0;
					for (const auto& member : m_ReflectionData.Members.at(set).at(binding))
					{
						m_ReflectionData.MemberNameCache[member.Name] = { set, binding, index++ };
					}
				}
			}
		}
	}

	void ShaderCompiler::ReflectMembers(const spirv_cross::Resource& resource, ShaderReflection::MemberList& memberList)
	{
		const auto& resourceName = m_CurrentCompiler->get_name(resource.id);
		const auto& resourceType = m_CurrentCompiler->get_type(resource.base_type_id);

		uint32_t index = 0, offset = 0;
		for (const auto& memberID : resourceType.member_types)
		{
			const auto& type = m_CurrentCompiler->get_type(memberID);
			const auto& name = m_CurrentCompiler->get_member_name(resource.base_type_id, index);
			const uint32_t typeSize = (uint32_t)m_CurrentCompiler->get_declared_struct_member_size(resourceType, index);

			memberList.push_back({
				.Name = fmt::format("{}.{}", resourceName, name),
				.Type = utils::GetVariableType(type),
				.Size = typeSize,
				.Offset = offset
								 });

			index++;
			offset += typeSize;
		}
	}

	ShaderReflection::Resource& ShaderCompiler::ReflectResource(const spirv_cross::Resource& resource)
	{
		const auto& type = m_CurrentCompiler->get_type(resource.type_id);
		const auto& name = m_CurrentCompiler->get_name(resource.id);
		SK_CORE_VERIFY(!name.empty());

		const uint32_t set = m_CurrentCompiler->get_decoration(resource.id, spv::DecorationDescriptorSet);
		const uint32_t binding = m_CurrentCompiler->get_decoration(resource.id, spv::DecorationBinding);
		SK_CORE_VERIFY(m_ReflectionData.Resources[set].contains(binding) == false, "Binding ({}.{}) allready in use! (Current: {})", set, binding, name);

		auto& reflectionData = m_ReflectionData.Resources[set][binding];
		reflectionData.Name = name;
		reflectionData.Set = set;
		reflectionData.Binding = binding;
		reflectionData.Stage = utils::ToShaderReflectionShaderStage(m_CurrentStage);

		if (!type.array.empty())
			reflectionData.ArraySize = type.array[0];

		return reflectionData;
	}

}
