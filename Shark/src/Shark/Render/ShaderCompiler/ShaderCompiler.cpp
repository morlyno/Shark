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

	}

	ShaderCompiler::ShaderCompiler(const std::filesystem::path& sourcePath, const CompilerOptions& options)
		: m_Options(options)
	{
		m_Info.SourcePath = FileSystem::Relative(sourcePath);
		m_Info.Language = utils::ShaderLanguageFromFileExtension(m_Info.SourcePath.extension().string());
		m_Info.ShaderID = Hash::GenerateFNV(m_Info.SourcePath.string());
		m_Options.Optimize = false;
		m_Options.GenerateDebugInfo = true;
		//m_Options.Force = true;
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
		shaderCache.UpdateOptions(m_Info, m_Options);

		for (const auto& [stage, sourceInfo] : m_SourceInfo)
		{
			auto cacheState = shaderCache.GetCacheState(sourceInfo);
			if (!m_Options.Force && cacheState == ShaderCacheState::UpToDate &&
				shaderCache.LoadBinary(sourceInfo, m_SpirvBinary[stage]))
			{
				SK_CORE_INFO_TAG("ShaderCompiler", "Loaded {} shader '{}' from cache", stage, m_Info.SourcePath);
				continue;
			}

			if (!CompileStage(stage) && cacheState == ShaderCacheState::OutOfDate)
			{
				SK_CORE_WARN_TAG("ShaderCompiler", "Loading older shader version from cache");
				shaderCache.LoadBinary(sourceInfo, m_SpirvBinary[stage]);
			}

			if (!m_SpirvBinary.contains(stage) || m_SpirvBinary.at(stage).empty())
			{
				// At this point the compilation of this stage failed and the cache is missing.
				SK_CORE_ERROR_TAG("ShaderCompiler", "Compiling shader '{}' failed!", m_Info.SourcePath.filename());
				SK_CORE_VERIFY(false, "Compiling shader '{}' failed!", m_Info.SourcePath.filename());
				return false;
			}
		}

		bool reflectionLoaded = false;
		if (m_CompiledStages == nvrhi::ShaderType::None)
		{
			reflectionLoaded = shaderCache.LoadReflection(m_Info.ShaderID, m_Reflection);
		}

		if (!reflectionLoaded)
			Reflect();

		m_D3D11Compiler.SetContext(m_Info, m_Options);
		m_D3D11Compiler.CompileOrLoad(*this, shaderCache);

		for (const auto& [stage, binary] : m_SpirvBinary)
		{
			shaderCache.GetEntry(m_Info).SourcePath = m_Info.SourcePath;
			shaderCache.CacheStage(m_SourceInfo.at(stage), binary, m_D3D11Compiler.GetBinary(stage));
		}

		shaderCache.CacheReflection(m_Info.ShaderID, m_Reflection);
		return true;
	}

	void ShaderCompiler::ClearState()
	{
		m_Source = {};
		m_SourceInfo.clear();
		m_SpirvBinary.clear();
		m_CompiledStages = nvrhi::ShaderType::None;
		m_Reflection = {};

		m_D3D11Compiler.ClearState();
	}

	Buffer ShaderCompiler::GetBinary(nvrhi::ShaderType stage, nvrhi::GraphicsAPI api) const
	{
		if (api == nvrhi::GraphicsAPI::D3D11)
			return m_D3D11Compiler.GetBinary(stage);
		return Buffer::FromArray(m_SpirvBinary.at(stage));
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
			SK_CORE_ERROR_TAG("ShaderCompiler", "Failed to compile {} shader '{}'.\n{}", stage, m_Info.SourcePath, errorMessage);
			return false;
		}

		SK_CORE_WARN_TAG("ShaderCompiler", "Compiled {} shader '{}'", stage, m_Info.SourcePath);
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
			ReflectStage(stage);
		
		MapBindings();
	}

	void ShaderCompiler::ReflectStage(nvrhi::ShaderType stage)
	{
		const auto& spirvBinary = m_SpirvBinary.at(stage);
		spirv_cross::Compiler compiler(spirvBinary.data(), spirvBinary.size());
		spirv_cross::ShaderResources shaderResources = compiler.get_shader_resources();

#if 0
	#define CHECK_BINDING(_binding, _typeStr)\
		if (m_ShaderResources.contains(_binding))\
		{\
			auto* item = m_ShaderResources.at(_binding);\
			SK_CORE_ERROR_TAG("ShaderCompiler", "Duplicate Binding {}[{}] in set {}: {}, {}", _typeStr, _binding.Slot, _binding.Set, item->Name, name);\
			SK_CORE_VERIFY(false, "Duplicate Binding {}[{}] in set {}: {}, {}", _typeStr, _binding.Slot, _binding.Set, item->Name, name);\
		}
#else
	#define CHECK_BINDING(...)
#endif

		SK_CORE_VERIFY(shaderResources.push_constant_buffers.size() <= 1);
		for (const auto& pushConstant : shaderResources.push_constant_buffers)
		{
			const auto& type = compiler.get_type(pushConstant.type_id);
			const auto& name = compiler.get_name(pushConstant.id);
			SK_CORE_VERIFY(!name.empty());

			if (!m_Reflection.PushConstant)
			{
				m_Reflection.PushConstant = ShaderResource::PushConstant{
					//.Slot = 0,
					.StructSize = (uint32_t)compiler.get_declared_struct_size(type),
					.Stage = stage
				};
			}
			else
			{
				m_Reflection.PushConstant->Stage |= stage;
				SK_CORE_VERIFY(m_Reflection.PushConstant->StructSize == (uint32_t)compiler.get_declared_struct_size(type));
			}

			//m_ShaderResources[m_ReflectionData.PushConstant.Binding] = &m_ReflectionData.PushConstant;
			//m_SpirvCrossIDs[{ ~0u, 0, GraphicsResourceType::ConstantBuffer }] = pushConstant.id;
		}
		
		for (const auto& constantBuffer : shaderResources.uniform_buffers)
		{
			const auto& type = compiler.get_type(constantBuffer.type_id);
			const auto& name = compiler.get_name(constantBuffer.id);
			SK_CORE_VERIFY(!name.empty());
			SK_CORE_VERIFY(type.array.empty(), "Arrays of constant buffers are not supported");

			const uint32_t set = compiler.get_decoration(constantBuffer.id, spv::DecorationDescriptorSet);
			const uint32_t slot = compiler.get_decoration(constantBuffer.id, spv::DecorationBinding);
			const GraphicsBinding binding = { set, slot, GraphicsResourceType::ConstantBuffer };

			if (set >= m_Reflection.BindingLayouts.size())
				m_Reflection.BindingLayouts.resize(set + 1);

			auto& layout = m_Reflection.BindingLayouts[set];
			CHECK_BINDING(binding, "b");

			auto& buffer = layout.ConstantBuffers[slot];
			buffer.Name = name;
			buffer.StructSize = (uint32_t)compiler.get_declared_struct_size(type);
			buffer.Slot = slot;
			buffer.Stage = stage;

			layout.Stage |= stage;
			layout.InputInfos[name] = {
				.Name = name,
				.Set = set,
				.Slot = slot,
				.Count = 1,
				.GraphicsType = GraphicsResourceType::ConstantBuffer,
				.Type = ShaderInputType::ConstantBuffer
			};

			//m_ShaderResources[binding] = &buffer;
			//m_SpirvCrossIDs[binding] = constantBuffer.id;
		}

		for (const auto& storageBuffer : shaderResources.storage_buffers)
		{
			const auto& type = compiler.get_type(storageBuffer.type_id);
			const auto& name = compiler.get_name(storageBuffer.id);
			SK_CORE_VERIFY(!name.empty());
			SK_CORE_VERIFY(type.array.empty(), "Arrays of storage buffers are not supported");

			const uint32_t set = compiler.get_decoration(storageBuffer.id, spv::DecorationDescriptorSet);
			const uint32_t slot = compiler.get_decoration(storageBuffer.id, spv::DecorationBinding);
			const GraphicsBinding binding = { set, slot, GraphicsResourceType::ShaderResourceView };

			if (set >= m_Reflection.BindingLayouts.size())
				m_Reflection.BindingLayouts.resize(set + 1);

			auto& layout = m_Reflection.BindingLayouts[set];
			CHECK_BINDING(binding, "t");

			auto& buffer = layout.StorageBuffers[slot];
			buffer.Name = name;
			buffer.StructSize = (uint32_t)compiler.get_declared_struct_size(type);
			buffer.Slot = slot;
			buffer.Stage = stage;

			layout.Stage |= stage;
			layout.InputInfos[name] = {
				.Name = name,
				.Set = set,
				.Slot = slot,
				.Count = 1,
				.GraphicsType = GraphicsResourceType::ShaderResourceView,
				.Type = ShaderInputType::StorageBuffer
			};

			//m_ShaderResources[binding] = &buffer;
			//m_SpirvCrossIDs[binding] = storageBuffer.id;
		}

		for (const auto& resource : shaderResources.separate_images)
		{
			const auto& type = compiler.get_type(resource.type_id);
			const auto& name = compiler.get_name(resource.id);
			SK_CORE_VERIFY(!name.empty());

			const uint32_t set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
			const uint32_t slot = compiler.get_decoration(resource.id, spv::DecorationBinding);
			const GraphicsBinding binding = { set, slot, GraphicsResourceType::ShaderResourceView };

			if (set >= m_Reflection.BindingLayouts.size())
				m_Reflection.BindingLayouts.resize(set + 1);

			auto& layout = m_Reflection.BindingLayouts[set];
			CHECK_BINDING(binding, "t");

			auto& image = layout.Images[slot];
			image.Name = name;
			image.Slot = slot;
			image.Stage = stage;

			if (!type.array.empty())
				image.ArraySize = type.array[0];

			ShaderInputType inputType = ShaderInputType::None;
			switch (type.image.dim)
			{
				case spv::Dim2D:
					inputType = ShaderInputType::Image2D;
					image.Dimension = 2;
					break;
				case spv::DimCube:
					inputType = ShaderInputType::ImageCube;
					image.Dimension = 3;
					break;
				default: SK_CORE_VERIFY(false); break;
			}

			layout.Stage |= stage;
			layout.InputInfos[name] = {
				.Name = name,
				.Set = set,
				.Slot = slot,
				.Count = image.ArraySize,
				.GraphicsType = GraphicsResourceType::ShaderResourceView,
				.Type = inputType
			};

			//m_ShaderResources[binding] = &image;
			//m_SpirvCrossIDs[binding] = resource.id;
		}

		for (const auto& resource : shaderResources.storage_images)
		{
			const auto& type = compiler.get_type(resource.type_id);
			const auto& name = compiler.get_name(resource.id);
			SK_CORE_VERIFY(!name.empty());

			const uint32_t set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
			const uint32_t slot = compiler.get_decoration(resource.id, spv::DecorationBinding);
			const GraphicsBinding binding = { set, slot, GraphicsResourceType::UnorderedAccessView };

			if (set >= m_Reflection.BindingLayouts.size())
				m_Reflection.BindingLayouts.resize(set + 1);

			auto& layout = m_Reflection.BindingLayouts[set];
			CHECK_BINDING(binding, "u");

			auto& storageImage = layout.StorageImages[slot];
			storageImage.Name = name;
			storageImage.Slot = slot;
			storageImage.Stage = stage;

			if (!type.array.empty())
				storageImage.ArraySize = type.array[0];

			ShaderInputType inputType = ShaderInputType::None;
			switch (type.image.dim)
			{
				case spv::Dim2D:
					inputType = ShaderInputType::StorageImage2D;
					break;
				case spv::DimCube:
					inputType = ShaderInputType::StorageImageCube;
					break;
				default: SK_CORE_VERIFY(false); break;
			}

			layout.Stage |= stage;
			layout.InputInfos[name] = {
				.Name = name,
				.Set = set,
				.Slot = slot,
				.Count = storageImage.ArraySize,
				.GraphicsType = GraphicsResourceType::UnorderedAccessView,
				.Type = inputType
			};

			//m_ShaderResources[binding] = &storageImage;
			//m_SpirvCrossIDs[binding] = resource.id;
		}

		for (const auto& resource : shaderResources.separate_samplers)
		{
			const auto& type = compiler.get_type(resource.type_id);
			const auto& name = compiler.get_name(resource.id);
			SK_CORE_VERIFY(!name.empty());

			const uint32_t set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
			const uint32_t slot = compiler.get_decoration(resource.id, spv::DecorationBinding);
			const GraphicsBinding binding = { set, slot, GraphicsResourceType::Sampler };

			if (set >= m_Reflection.BindingLayouts.size())
				m_Reflection.BindingLayouts.resize(set + 1);

			auto& layout = m_Reflection.BindingLayouts[set];
			CHECK_BINDING(binding, "s");

			auto& sampler = layout.Samplers[slot];
			sampler.Name = name;
			sampler.Slot = slot;
			sampler.Stage = stage;

			if (!type.array.empty())
				sampler.ArraySize = type.array[0];

			layout.Stage |= stage;
			layout.InputInfos[name] = {
				.Name = name,
				.Set = set,
				.Slot = slot,
				.Count = sampler.ArraySize,
				.GraphicsType = GraphicsResourceType::Sampler,
				.Type = ShaderInputType::Sampler
			};

			//m_ShaderResources[binding] = &sampler;
			//m_SpirvCrossIDs[binding] = resource.id;
		}

#undef CHECK_BINDING
	}

	void ShaderCompiler::MapBindings()
	{
		D3D11BindingSetOffsets bindingOffsets = {};

		if (m_Reflection.PushConstant && !m_Reflection.BindingLayouts.empty())
		{
			auto& layout = m_Reflection.BindingLayouts[0];
			const uint32_t min = layout.ConstantBuffers.empty() ? 0 : std::ranges::min(layout.ConstantBuffers | std::views::values | std::views::transform(&ShaderResource::Buffer::Slot));

			if (min == 0)
			{
				bindingOffsets.ConstantBuffer = 1;
				layout.BindingOffsets.ConstantBuffer = 1;
			}
		}

		for (uint32_t set = 1; set < m_Reflection.BindingLayouts.size(); set++)
		{
			auto& layout = m_Reflection.BindingLayouts[set - 1];

			const int maxCBSlot      = layout.ConstantBuffers.empty() ? -1 : std::ranges::max(layout.ConstantBuffers | std::views::values | std::views::transform(&ShaderResource::Buffer::Slot));
			const int maxSBSlot      = layout.StorageBuffers.empty()  ? -1 : std::ranges::max(layout.StorageBuffers  | std::views::values | std::views::transform(&ShaderResource::Buffer::Slot));
			const int maxImageSlot   = layout.Images.empty()          ? -1 : std::ranges::max(layout.Images          | std::views::values | std::views::transform(&ShaderResource::Image::Slot));
			const int maxSImageSlot  = layout.StorageImages.empty()   ? -1 : std::ranges::max(layout.StorageImages   | std::views::values | std::views::transform(&ShaderResource::Image::Slot));
			const int maxSamplerSlot = layout.Samplers.empty()        ? -1 : std::ranges::max(layout.Samplers        | std::views::values | std::views::transform(&ShaderResource::Sampler::Slot));

			bindingOffsets.ConstantBuffer  += maxCBSlot + 1;
			bindingOffsets.ShaderResource  += std::max(maxSBSlot, maxImageSlot) + 1;
			bindingOffsets.UnorderedAccess += maxSImageSlot + 1;
			bindingOffsets.Sampler         += maxSamplerSlot + 1;

			m_Reflection.BindingLayouts[set].BindingOffsets = bindingOffsets;
		}

	}

}
