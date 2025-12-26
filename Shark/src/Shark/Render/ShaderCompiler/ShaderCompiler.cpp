#include "skpch.h"
#include "ShaderCompiler.h"

#include "Shark/Core/Memory.h"
#include "Shark/File/FileSystem.h"

#include "Shark/Render/Renderer.h"
#include "Shark/Render/ShaderCompiler/ShaderCache.h"
#include "Shark/Render/ShaderCompiler/DirectX11ShaderCompiler.h"

#include "Shark/Utils/String.h"
#include "Shark/Utils/std.h"

#include <spirv_cross/spirv_cross.hpp>

namespace Shark {

	namespace DxcInstances {
		ATL::CComPtr<IDxcCompiler3> g_Compiler = nullptr;
		ATL::CComPtr<IDxcUtils> g_Utils = nullptr;
		ATL::CComPtr<IDxcIncludeHandler> g_IncludeHandler = nullptr;
	}

	namespace utils {

		static LPCWSTR ShaderStageToMakro(nvrhi::ShaderType stage)
		{
			switch (stage)
			{
				case nvrhi::ShaderType::Vertex: return L"__VERTEX_STAGE__";
				case nvrhi::ShaderType::Pixel: return L"__PIXEL_STAGE__";
				case nvrhi::ShaderType::Compute: return L"__COMPUTE_STAGE__";
			}

			SK_CORE_ASSERT(false, "Unknown ShaderType");
			return L"__UNKNOWN_STAGE__";
		}

		static void SetupDXC()
		{
			if (!DxcInstances::g_Compiler)
			{
				DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&DxcInstances::g_Compiler));
				DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&DxcInstances::g_Utils));
				DxcInstances::g_Utils->CreateDefaultIncludeHandler(&DxcInstances::g_IncludeHandler);
			}
		}

	}

	ShaderCompiler::ShaderCompiler(const std::filesystem::path& sourcePath, const CompilerOptions& options)
		: m_Options(options)
	{
		m_Info.SourcePath = FileSystem::Relative(std::filesystem::canonical(sourcePath)).generic_wstring();
		m_Info.ShaderID = Hash::GenerateFNV(m_Info.SourcePath.generic_string());

		m_PlatformCompilers.push_back(Scope<D3D11::ShaderCompiler>::Create(options));
	}

	/*Ref<ShaderCompiler> ShaderCompiler::Load(const std::filesystem::path& sourcePath, const CompilerOptions& options)
	{
		auto compiler = Ref<ShaderCompiler>::Create(sourcePath, options);
		if (!compiler->Reload())
			return nullptr;
		return compiler;
	}*/

	bool ShaderCompiler::Reload()
	{
		utils::SetupDXC();

		auto& shaderCache = Renderer::GetShaderCache();
		m_Result = Scope<CompilerResult>::Create();

		std::vector<StageInfo> stageInfo;
		m_Options.Force = m_Options.Force || !shaderCache.LoadStageInfo(m_Info, stageInfo);

		//////////////////////////////////////////////////////////////////////////
		//// Try load from cache
		//////////////////////////////////////////////////////////////////////////

		if (!m_Options.Force && shaderCache.ShaderUpToDate(m_Info))
		{
			for (const auto& info : stageInfo)
			{
				std::vector<uint32_t> binary;
				if (shaderCache.LoadSpirv(m_Info, info.Stage, binary))
				{
					m_Result->SpirvBinary[info.Stage] = std::move(binary);
					SK_CORE_INFO_TAG("ShaderCompiler", "Loaded {} shader '{}' from cache", info.Stage, m_Info.SourcePath);
				}
			}
		}

		//////////////////////////////////////////////////////////////////////////
		//// Preprocess and Compile
		//////////////////////////////////////////////////////////////////////////

		// Preprocess source if necessary
		if (m_Options.Force || stageInfo.size() != m_Result->SpirvBinary.size())
		{
			bool success = Preprocess(m_Options.Force);
			SK_CORE_VERIFY(success);
		}

		// Compile missing stages
		for (const auto& [stage, result] : m_PreprocessedResult)
		{
			CacheStatus status = shaderCache.GetCacheStatus(m_Info, result.Info);
			if (!m_Options.Force && status == CacheStatus::OK)
			{
				std::vector<uint32_t> binary;
				if (shaderCache.LoadSpirv(m_Info, stage, binary))
				{
					m_Result->SpirvBinary[stage] = std::move(binary);
					SK_CORE_INFO_TAG("ShaderCompiler", "Loaded {} shader '{}' from cache after preprocess", stage, m_Info.SourcePath);
					continue;
				}
			}

			if (!CompileStage(stage) && status == CacheStatus::OutOfDate)
			{
				SK_CORE_WARN_TAG("ShaderCompiler", "Loading older shader version from cache");
				shaderCache.LoadSpirv(m_Info, stage, m_Result->SpirvBinary[stage]);
			}

			if (!m_Result->SpirvBinary.contains(stage) || m_Result->SpirvBinary.at(stage).empty())
			{
				// At this point the compilation of this stage failed and the cache is missing.
				SK_CORE_ERROR_TAG("ShaderCompiler", "Compiling shader '{}' failed!", m_Info.SourcePath.filename());
				SK_DEBUG_BREAK_CONDITIONAL(BREAK_ON_FAILED_COMPILATION);
				m_Result->SpirvBinary.erase(stage);
				return false;
			}

			shaderCache.SaveSpirv(m_Info, stage, m_Result->SpirvBinary.at(stage));
		}

		if (m_CompiledStages != nvrhi::ShaderType::None)
		{
			auto stages = m_PreprocessedResult | std::views::values | std::views::transform(&PreprocessedResult::Info) | std::ranges::to<std::vector>();
			auto includes = m_Preprocessor.Includes | std::views::transform(&IncludeData::Filepath) | std::ranges::to<std::vector>();
			shaderCache.SaveShaderInfo(m_Info, stages, includes);
		}

		//////////////////////////////////////////////////////////////////////////
		//// Reflection
		//////////////////////////////////////////////////////////////////////////
		
		bool reflectionLoaded = false;
		if (m_CompiledStages == nvrhi::ShaderType::None)
		{
			reflectionLoaded = shaderCache.LoadReflection(m_Info, m_Result->Reflection, m_Result->RequestedBindingSets, m_Result->LayoutMode);
		}

		if (!reflectionLoaded)
		{
			Reflect();

			shaderCache.SaveReflection(m_Info, m_Result->Reflection, m_Result->RequestedBindingSets, m_Result->LayoutMode);
		}

		//////////////////////////////////////////////////////////////////////////
		//// Compile platform specific binaries
		//////////////////////////////////////////////////////////////////////////

		bool anyFailed = false;
		for (auto& compiler : m_PlatformCompilers)
		{
			anyFailed = !compiler->Reload(m_Info, m_CompiledStages, *m_Result);
		}

		if (anyFailed)
		{
			auto deviceManager = Renderer::GetDeviceManager();
			if (!m_Result->PlatformBinary.contains(deviceManager->GetGraphicsAPI()))
				return false;

			SK_CORE_WARN_TAG("ShaderCompiler", "A platform compiler failed but the binary for the current graphics api is present, execution can continue");
		}

		return true;
	}

	void ShaderCompiler::GetResult(Scope<CompilerResult>& result)
	{
		result = std::move(m_Result);
	}

	bool ShaderCompiler::Preprocess(bool processAll)
	{
		m_Preprocessor.PreprocessFile(m_Info.SourcePath);
		if (!m_Preprocessor.Errors.empty())
		{
			SK_CORE_ERROR_TAG("ShaderCompiler", "Failed to preprocess file '{}'\n{}", m_Info.SourcePath, fmt::join(m_Preprocessor.Errors, "\n"));
			return false;
		}

		m_IncludeHandler = std::make_shared<HLSLIncludeHandler>(&m_Preprocessor, DxcInstances::g_Utils);

		for (auto& stage : m_Preprocessor.Stages)
		{
			if (!processAll && m_Result->SpirvBinary.contains(stage))
				continue;

			bool success = PreprocessStage(stage);
			SK_CORE_VERIFY(success);
		}

		return true;
	}

	bool ShaderCompiler::PreprocessStage(nvrhi::ShaderType stage)
	{
		CComPtr<IDxcBlobEncoding> sourceBlob;
		DxcInstances::g_Utils->CreateBlob(m_Preprocessor.Source.data(), static_cast<UINT32>(m_Preprocessor.Source.size()), CP_UTF8, &sourceBlob);

		const wchar_t* version = s_ShaderTypeMappings.at(stage).DXC;
		std::wstring filename = m_Info.SourcePath.stem().wstring();

		std::vector<LPCWSTR> arguments = {
			filename.c_str(),

			L"-P",
			L"-I Resources/Shaders/",
			L"-I Resources/Shaders/EnvMap",
			L"-D", utils::ShaderStageToMakro(stage)
		};


		DxcBuffer buffer = {};
		buffer.Encoding = 0;
		buffer.Ptr = sourceBlob->GetBufferPointer();
		buffer.Size = sourceBlob->GetBufferSize();


		CComPtr<IDxcResult> result = nullptr;
		HRESULT hResult = DxcInstances::g_Compiler->Compile(&buffer, arguments.data(), static_cast<UINT32>(arguments.size()),
															m_IncludeHandler.get(), IID_PPV_ARGS(&result));


		result->GetStatus(&hResult);
		if (FAILED(hResult))
		{
			CComPtr<IDxcBlobEncoding> errorBlob;
			hResult = result->GetErrorBuffer(&errorBlob);
			std::string errorMessage = (const char*)errorBlob->GetBufferPointer();
			SK_CORE_ERROR_TAG("ShaderCompiler", "Failed to preprocessor source by DXC!\n{}", errorMessage);
			return false;
		}

		CComPtr<IDxcBlob> code;
		result->GetResult(&code);

		auto& processed = m_PreprocessedResult[stage];
		processed.Source = { static_cast<const char*>(code->GetBufferPointer()), code->GetBufferSize() };
		processed.Info.HashCode = Hash::GenerateFNV(processed.Source);
		processed.Info.Stage = stage;
		return true;
	}

	bool ShaderCompiler::CompileStage(nvrhi::ShaderType stage)
	{
		std::string errorMessage = HLSLCompileStage(stage);

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
		const std::string& shaderSource = m_PreprocessedResult.at(stage).Source;

		// 
		// #TODO #Renderer #Investigate dxc -fspv-preserve-bindings bug
		// 
		// dxc has a bug were -fspv-preserve-bindings don't work.
		// it looks like it is caused by image query operations(?).
		// the only occasion where this effect caused issues where avoided.
		// 
		// Solution 1:
		//  dxc bug is resolved
		// 
		// Solution 2:
		//  user the cli interface
		// 

		const wchar_t* version = s_ShaderTypeMappings.at(stage).DXC;
		std::wstring filename = m_Info.SourcePath.stem().wstring();

		std::vector<LPCWSTR> arguments = {
			filename.c_str(),
			L"-E", L"main",
			L"-T", version,

			L"-I Resources/Shaders/",
			L"-I Resources/Shaders/EnvMap",

			L"-D", L"__HLSL__",
			L"-D", utils::ShaderStageToMakro(stage),

			L"-spirv",
			L"-fspv-reflect",
			L"-fspv-target-env=vulkan1.2",
			L"-fspv-preserve-bindings",
			L"-fspv-preserve-interface",
		};

		if (m_Options.GenerateDebugInfo)
		{
			arguments.emplace_back(L"-Qembed_debug");
			arguments.emplace_back(DXC_ARG_DEBUG);
		}

		if (m_Options.Optimize)
			arguments.emplace_back(DXC_ARG_OPTIMIZATION_LEVEL3);



		DxcBuffer buffer = {};
		buffer.Encoding = CP_UTF8;
		buffer.Ptr = shaderSource.data();
		buffer.Size = shaderSource.size();

		CComPtr<IDxcResult> result = nullptr;
		HRESULT hResult = DxcInstances::g_Compiler->Compile(&buffer, arguments.data(), static_cast<UINT32>(arguments.size()),
															m_IncludeHandler.get(), IID_PPV_ARGS(&result));

		result->GetStatus(&hResult);
		if (FAILED(hResult))
		{
			CComPtr<IDxcBlobEncoding> errorBlob;
			hResult = result->GetErrorBuffer(&errorBlob);
			return (const char*)errorBlob->GetBufferPointer();
		}

		CComPtr<IDxcBlob> code;
		result->GetResult(&code);

		auto& spirvBinary = m_Result->SpirvBinary[stage];
		Memory::Write(spirvBinary, code->GetBufferPointer(), code->GetBufferSize());

		return {};
	}

	void ShaderCompiler::Reflect()
	{
		for (const auto& [stage, binary] : m_Result->SpirvBinary)
			ReflectStage(stage);

		//////////////////////////////////////////////////////////////////////////
		//// Map Bindings
		//////////////////////////////////////////////////////////////////////////

		D3D11BindingSetOffsets bindingOffsets = {};

		if (m_Result->Reflection.PushConstant && !m_Result->Reflection.BindingLayouts.empty())
		{
			auto& layout = m_Result->Reflection.BindingLayouts[0];
			const uint32_t min = layout.ConstantBuffers.empty() ? 0 : std::ranges::min(layout.ConstantBuffers | std::views::values | std::views::transform(&ShaderResource::Buffer::Slot));

			if (min == 0)
			{
				bindingOffsets.ConstantBuffer = 1;
				layout.BindingOffsets.ConstantBuffer = 1;
			}
		}

		for (uint32_t set = 1; set < m_Result->Reflection.BindingLayouts.size(); set++)
		{
			auto& layout = m_Result->Reflection.BindingLayouts[set - 1];
			
			const int maxCBSlot      = layout.ConstantBuffers.empty() ? -1 : std::ranges::max(layout.ConstantBuffers | std::views::values | std::views::transform(&ShaderResource::Buffer::Slot));
			const int maxSBSlot      = layout.StorageBuffers.empty()  ? -1 : std::ranges::max(layout.StorageBuffers  | std::views::values | std::views::transform(&ShaderResource::Buffer::Slot));
			const int maxImageSlot   = layout.Images.empty()          ? -1 : std::ranges::max(layout.Images          | std::views::values | std::views::transform(&ShaderResource::Image::Slot));
			const int maxSImageSlot  = layout.StorageImages.empty()   ? -1 : std::ranges::max(layout.StorageImages   | std::views::values | std::views::transform(&ShaderResource::Image::Slot));
			const int maxSamplerSlot = layout.Samplers.empty()        ? -1 : std::ranges::max(layout.Samplers        | std::views::values | std::views::transform(&ShaderResource::Sampler::Slot));
			
			bindingOffsets.ConstantBuffer  += maxCBSlot + 1;
			bindingOffsets.ShaderResource  += std::max(maxSBSlot, maxImageSlot) + 1;
			bindingOffsets.UnorderedAccess += maxSImageSlot + 1;
			bindingOffsets.Sampler         += maxSamplerSlot + 1;

			m_Result->Reflection.BindingLayouts[set].BindingOffsets = bindingOffsets;
		}

		//////////////////////////////////////////////////////////////////////////
		//// Process Compiler Instructions
		//////////////////////////////////////////////////////////////////////////

		for (const auto& pragma : m_Preprocessor.CompilerInstructions)
		{
			switch (pragma.Instruction)
			{
				case CompilerInstruction::Type::Combine:
				{
					SK_CORE_VERIFY(pragma.Arguments.size() == 2);
					BuildCombinedImageSampler(pragma.Arguments[0], pragma.Arguments[1]);
					break;
				}
				case CompilerInstruction::Type::Bind:
				{
					SK_CORE_VERIFY(pragma.Arguments.size() == 1);
					m_Result->RequestedBindingSets.emplace_back(pragma.Arguments[0]);

					if (pragma.Arguments[0] == "samplers")
					{
						if (m_Result->Reflection.BindingLayouts.size() < 4)
						{
							SK_CORE_ERROR_TAG("ShaderCompiler", "Samplers where requested with bind but the layout at set 3 doesn't exists!");
							continue;
						}

						size_t inputs = m_Result->Reflection.BindingLayouts[3].InputInfos.size();
						if (inputs < 6)
						{
							SK_CORE_ERROR_TAG("ShaderCompiler", "Samplers where requested but the layout has only {} inputs.", inputs);
							SK_CORE_ERROR_TAG("ShaderCompiler", "I don't know the exact reason but removing 'Image queries' could help.", inputs);
							SK_CORE_ERROR_TAG("ShaderCompiler", "Image queries => u_Texture.GetDimensions()", inputs);
						}

						SK_CORE_ASSERT(m_Result->Reflection.BindingLayouts[3].InputInfos.size() == 6);
					}

					break;
				}
				case CompilerInstruction::Type::Layout:
				{
					SK_CORE_VERIFY(pragma.Arguments.size() == 1);

					const auto& arg = pragma.Arguments[0];
					if (String::Compare(arg, "renderpass", String::Case::Ignore))
						m_Result->LayoutMode = LayoutShareMode::PassOnly;
					else if (String::Compare(arg, "material", String::Case::Ignore))
						m_Result->LayoutMode = LayoutShareMode::MaterialOnly;
					else if (String::Compare(arg, "share", String::Case::Ignore))
						m_Result->LayoutMode = LayoutShareMode::PassAndMaterial;
					else if (String::Compare(arg, "default", String::Case::Ignore))
						m_Result->LayoutMode = LayoutShareMode::Default;
					else
					{
						SK_CORE_WARN_TAG("ShaderCompiler", "Failed to parse Layout mode '{}'. Arguments are 'renderpass', 'material', 'share', 'default'.", arg);
						m_Result->LayoutMode = LayoutShareMode::Default;
					}
				}
			}
		}
	}

	void ShaderCompiler::ReflectStage(nvrhi::ShaderType stage)
	{
		const auto& spirvBinary = m_Result->SpirvBinary.at(stage);
		const spirv_cross::Compiler compiler(spirvBinary.data(), spirvBinary.size());
		const spirv_cross::ShaderResources shaderResources = compiler.get_shader_resources();

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

#define LOG_REFLECTION(_set, _slot, _register, _name, _type) SK_CORE_INFO_TAG("ShaderCompiler", " - {} [{} {}{}] {}", _name, _set, _register, _slot, _type)

		SK_CORE_INFO_TAG("ShaderCompiler", "=== Reflection {} ===", stage);

		SK_CORE_VERIFY(shaderResources.push_constant_buffers.size() <= 1);
		for (const auto& pushConstant : shaderResources.push_constant_buffers)
		{
			const auto& type = compiler.get_type(pushConstant.type_id);
			const auto& name = compiler.get_name(pushConstant.id);
			SK_CORE_VERIFY(!name.empty());

			if (!m_Result->Reflection.PushConstant)
			{
				m_Result->Reflection.PushConstant = ShaderResource::PushConstant{
					//.Slot = 0,
					.StructSize = (uint32_t)compiler.get_declared_struct_size(type),
					.Stage = stage
				};
			}
			else
			{
				m_Result->Reflection.PushConstant->Stage |= stage;
				SK_CORE_VERIFY(m_Result->Reflection.PushConstant->StructSize == (uint32_t)compiler.get_declared_struct_size(type));
			}

			SK_CORE_INFO_TAG("ShaderCompiler", " - {} PushConstant", name);
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

			if (set >= m_Result->Reflection.BindingLayouts.size())
				m_Result->Reflection.BindingLayouts.resize(set + 1);

			auto& layout = m_Result->Reflection.BindingLayouts[set];
			CHECK_BINDING(binding, "b");

			if (layout.ConstantBuffers.contains(slot))
			{
				auto& buffer = layout.ConstantBuffers.at(slot);
				SK_CORE_VERIFY(buffer.Name == name);
				SK_CORE_VERIFY(buffer.StructSize == (uint32_t)compiler.get_declared_struct_size(type));
				buffer.Stage |= stage;

				LOG_REFLECTION(set, slot, "b", name, "ConstantBuffer Shared");
				continue;
			}

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

			LOG_REFLECTION(set, slot, "b", name, "ConstantBuffer");
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

			if (set >= m_Result->Reflection.BindingLayouts.size())
				m_Result->Reflection.BindingLayouts.resize(set + 1);

			auto& layout = m_Result->Reflection.BindingLayouts[set];
			CHECK_BINDING(binding, "t");

			if (layout.StorageBuffers.contains(slot))
			{
				auto& buffer = layout.StorageBuffers.at(slot);
				SK_CORE_VERIFY(buffer.Name == name);
				SK_CORE_VERIFY(buffer.StructSize == (uint32_t)compiler.get_declared_struct_size(type));
				buffer.Stage |= stage;

				LOG_REFLECTION(set, slot, "t", name, "StorageBuffer Shared");
				continue;
			}

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

			LOG_REFLECTION(set, slot, "t", name, "StorageBuffer");
		}

		for (const auto& resource : shaderResources.separate_images)
		{
			const auto& type = compiler.get_type(resource.type_id);
			const auto& name = compiler.get_name(resource.id);
			SK_CORE_VERIFY(!name.empty());

			const uint32_t set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
			const uint32_t slot = compiler.get_decoration(resource.id, spv::DecorationBinding);
			const GraphicsBinding binding = { set, slot, GraphicsResourceType::ShaderResourceView };

			if (set >= m_Result->Reflection.BindingLayouts.size())
				m_Result->Reflection.BindingLayouts.resize(set + 1);

			auto& layout = m_Result->Reflection.BindingLayouts[set];
			CHECK_BINDING(binding, "t");

			if (layout.Images.contains(slot))
			{
				auto& image = layout.Images.at(slot);
				SK_CORE_VERIFY(image.Name == name);
				SK_CORE_VERIFY(image.ArraySize == !type.array.empty() ? type.array[0] : 1);
				image.Stage |= stage;

				LOG_REFLECTION(set, slot, "t", name, "SeparateImage Shared");
				continue;
			}

			auto& image = layout.Images[slot];
			image.Name = name;
			image.Slot = slot;
			image.Stage = stage;

			if (!type.array.empty())
				image.ArraySize = type.array[0];

			switch (type.image.dim)
			{
				case spv::Dim2D:
					image.Dimension = 2;
					break;
				case spv::DimCube:
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
				.Type = ShaderInputType::Image
			};

			LOG_REFLECTION(set, slot, "t", name, "SeparateImage");
		}

		for (const auto& resource : shaderResources.storage_images)
		{
			const auto& type = compiler.get_type(resource.type_id);
			const auto& name = compiler.get_name(resource.id);
			SK_CORE_VERIFY(!name.empty());

			const uint32_t set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
			const uint32_t slot = compiler.get_decoration(resource.id, spv::DecorationBinding);
			const GraphicsBinding binding = { set, slot, GraphicsResourceType::UnorderedAccessView };

			if (set >= m_Result->Reflection.BindingLayouts.size())
				m_Result->Reflection.BindingLayouts.resize(set + 1);

			auto& layout = m_Result->Reflection.BindingLayouts[set];
			CHECK_BINDING(binding, "u");

			if (layout.StorageImages.contains(slot))
			{
				auto& storageImage = layout.StorageImages.at(slot);
				SK_CORE_VERIFY(storageImage.Name == name);
				SK_CORE_VERIFY(storageImage.ArraySize == !type.array.empty() ? type.array[0] : 1);
				storageImage.Stage |= stage;

				LOG_REFLECTION(set, slot, "u", name, "StorageImage Shared");
				continue;
			}

			auto& storageImage = layout.StorageImages[slot];
			storageImage.Name = name;
			storageImage.Slot = slot;
			storageImage.Stage = stage;

			if (!type.array.empty())
				storageImage.ArraySize = type.array[0];

			switch (type.image.dim)
			{
				case spv::Dim2D:
					storageImage.Dimension = 2;
					break;
				case spv::DimCube:
					storageImage.Dimension = 3;
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
				.Type = ShaderInputType::StorageImage
			};

			LOG_REFLECTION(set, slot, "u", name, "StorageImage");
		}

		for (const auto& resource : shaderResources.separate_samplers)
		{
			const auto& type = compiler.get_type(resource.type_id);
			const auto& name = compiler.get_name(resource.id);
			SK_CORE_VERIFY(!name.empty());

			const uint32_t set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
			const uint32_t slot = compiler.get_decoration(resource.id, spv::DecorationBinding);
			const GraphicsBinding binding = { set, slot, GraphicsResourceType::Sampler };

			if (set >= m_Result->Reflection.BindingLayouts.size())
				m_Result->Reflection.BindingLayouts.resize(set + 1);

			auto& layout = m_Result->Reflection.BindingLayouts[set];
			CHECK_BINDING(binding, "s");

			if (layout.Samplers.contains(slot))
			{
				auto& sampler = layout.Samplers.at(slot);
				SK_CORE_VERIFY(sampler.Name == name);
				sampler.Stage |= stage;

				LOG_REFLECTION(set, slot, "s", name, "Sampler Shared");
				continue;
			}

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

			LOG_REFLECTION(set, slot, "s", name, "Sampler");
		}
#undef CHECK_BINDING
	}

	void ShaderCompiler::BuildCombinedImageSampler(const std::string& imageName, const std::string& samplerName)
	{
		ShaderInputInfo* imageInfo = FindInputInfo(imageName);
		ShaderInputInfo* samplerInfo = FindInputInfo(samplerName);

		if (!imageInfo || !samplerInfo)
		{
			std::string errorMessage;
			if (!imageInfo)   errorMessage += fmt::format("\n - Image input '{}' not found", imageName);
			if (!samplerInfo) errorMessage += fmt::format("\n - Sampler input '{}' not found", samplerName);

			SK_CORE_WARN_TAG("ShaderCompiler", "Failed to build combined image sampler!{}", errorMessage);
			return;
		}

		if (imageInfo->Set != samplerInfo->Set)
		{
			SK_CORE_WARN_TAG("ShaderCompiler", "Failed to build combined image sampler because Image and Sampler are in different sets!\n"
							 " - Image '{}' {}\n - Sampler '{}' {}",
							 imageInfo->Name, imageInfo->Set,
							 samplerInfo->Name, samplerInfo->Set);
			return;
		}

		auto& layout = m_Result->Reflection.BindingLayouts[imageInfo->Set];
		auto& image = layout.Images.at(imageInfo->Slot);
		auto& sampler = layout.Samplers.at(imageInfo->Slot);

		auto& sampledImage = layout.SampledImages[imageInfo->Slot];
		sampledImage.Name = image.Name;
		sampledImage.SeparateImage = image;
		sampledImage.SeparateSampler = sampler;

		ShaderInputInfo info = {
			.Name = imageInfo->Name,
			.Set = imageInfo->Set,
			.Slot = imageInfo->Slot,
			.Count = imageInfo->Count,
			.GraphicsType = GraphicsResourceType::ShaderResourceView,
			.Type = ShaderInputType::Texture
		};

		layout.InputInfos.erase(imageInfo->Name);
		layout.InputInfos.erase(samplerInfo->Name);
		layout.InputInfos[info.Name] = info;
	}

	Shark::ShaderInputInfo* ShaderCompiler::FindInputInfo(const std::string& name)
	{
		for (auto& layout : m_Result->Reflection.BindingLayouts)
		{
			if (!layout.InputInfos.contains(name))
				continue;

			return &layout.InputInfos.at(name);
		}

		return nullptr;
	}

}
