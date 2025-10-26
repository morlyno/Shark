#include "skpch.h"
#include "D3D11ShaderCompiler.h"

#include "Shark/Utils/String.h"
#include "Shark/Render/ShaderCompiler/ShaderCompiler.h"
#include "Shark/Utils/std.h"

#include <d3dcompiler.h>
#include <shaderc/shaderc.h>
#include <spirv_cross/spirv_hlsl.hpp>
#include "Shark/Math/Math.h"
#include "Shark/Render/Renderer.h"

namespace Shark {
	
	D3D11ShaderCompilerInterface::D3D11ShaderCompilerInterface()
	{
	}
	
	D3D11ShaderCompilerInterface::~D3D11ShaderCompilerInterface()
	{
		for (auto& [stage, binary] : m_D3D11Binary)
		{
			binary.Release();
		}
	}

	void D3D11ShaderCompilerInterface::ClearState()
	{
		for (auto& [stage, binary] : m_D3D11Binary)
			binary.Release();

		m_HLSLSource.clear();
		m_D3D11Binary.clear();
	}

	bool D3D11ShaderCompilerInterface::CompileOrLoad(const ShaderCompiler& compiler, ShaderCache& cache)
	{
		const nvrhi::ShaderType compiledStages = compiler.GetCompiledStages();

		const auto& spirvBinaries = compiler.GetSpirvBinaries();
		for (const auto& [stage, binary] : spirvBinaries)
		{
			const auto& sourceInfo = compiler.GetSourceInfo(stage);
			ShaderCacheState cacheState = cache.GetD3D11CacheSyncState(sourceInfo);

			if (!(stage & compiledStages) && cacheState == ShaderCacheState::UpToDate &&
				cache.LoadD3D11(sourceInfo, m_D3D11Binary[stage]))
			{
				SK_CORE_TRACE_TAG("ShaderCompiler", "Loaded d3d11 {} shader '{}' from cache", stage, m_Info.SourcePath);
				continue;
			}

			CrossCompileStage(stage, binary, compiler);
			std::string error = CompileStage(stage);
			if (!error.empty())
			{
				SK_CORE_ERROR_TAG("ShaderCompiler", "Failed to compile d3d11 {} shader '{}'.\n{}", stage, m_Info.SourcePath, error);

				if (cacheState == ShaderCacheState::OutOfDate)
				{
					SK_CORE_WARN_TAG("ShaderCompiler", "Loading older shader version from cache");
					cache.LoadD3D11(sourceInfo, m_D3D11Binary[stage]);
				}
			}

			if (!m_D3D11Binary.contains(stage) || !m_D3D11Binary.at(stage))
			{
				SK_CORE_ERROR_TAG("ShaderCompiler", "Compiling shader '{}' failed!", m_Info.SourcePath.filename());
				SK_CORE_VERIFY(false, "Compiling shader '{}' failed!", m_Info.SourcePath.filename());
				return false;
			}
		}
		return true;
	}

	std::string D3D11ShaderCompilerInterface::CompileStage(nvrhi::ShaderType stage)
	{
		UINT flags = 0;
		flags |= D3DCOMPILE_ALL_RESOURCES_BOUND;

		if (m_Options.Optimize)
			flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;

		if (m_Options.GenerateDebugInfo)
			flags |= D3DCOMPILE_DEBUG;

		ID3DBlob* shaderBinary = nullptr;
		ID3DBlob* errorMessage = nullptr;
		const std::string name = fmt::format("{}{}", m_Info.SourcePath.stem().string(), s_ShaderTypeMappings.at(stage).Extension);
		const auto& hlslSource = m_HLSLSource.at(stage);

		if (FAILED(D3DCompile(hlslSource.data(), hlslSource.size(), name.c_str(), nullptr, nullptr, "main", s_ShaderTypeMappings.at(stage).D3D11, flags, 0, &shaderBinary, &errorMessage)))
		{
			std::string errorStr = fmt::format("Failed to Compile HLSL (Stage: {0})\n{1}", stage, (char*)errorMessage->GetBufferPointer());
			errorMessage->Release();
			return errorStr;
		}

		m_D3D11Binary[stage] = Buffer::Copy(shaderBinary->GetBufferPointer(), shaderBinary->GetBufferSize());
		shaderBinary->Release();

		SK_CORE_WARN_TAG("ShaderCompiler", "Compiled d3d11 {} shader '{}'", stage, m_Info.SourcePath);
		return {};
	}

	void D3D11ShaderCompilerInterface::CrossCompileStage(nvrhi::ShaderType stage, std::span<const uint32_t> spirvBinary, const ShaderCompiler& shaderCompiler)
	{
		spirv_cross::CompilerHLSL compiler(spirvBinary.data(), spirvBinary.size());
		spirv_cross::CompilerHLSL::Options options;

		options.shader_model = 50;

		compiler.set_hlsl_options(options);

		/// Vertex attribute remapping

		if (stage == nvrhi::ShaderType::Vertex)
		{
			spirv_cross::ShaderResources shaderResources = compiler.get_shader_resources();
			for (const auto& resource : shaderResources.stage_inputs)
			{
				std::string name = resource.name.substr(m_Info.Language == ShaderLanguage::HLSL ? 7 : 2);
				uint32_t location = compiler.get_decoration(resource.id, spv::DecorationLocation);
				compiler.add_vertex_attribute_remap({ location, name });
			}
		}

		/// Binding set offsets

		const auto& reflection = shaderCompiler.GetReflectionData();
		const auto applyOffsets = [&compiler, &reflection](const spirv_cross::Resource& resource, uint32_t D3D11BindingSetOffsets::* offsets)
		{
			const uint32_t set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
			const uint32_t slot = compiler.get_decoration(resource.id, spv::DecorationBinding);

			const auto& layout = reflection.BindingLayouts[set];
			compiler.set_decoration(resource.id, spv::DecorationBinding, slot + layout.BindingOffsets.*offsets);
			compiler.set_decoration(resource.id, spv::DecorationDescriptorSet, 0);
		};

		const auto& shaderResources = compiler.get_shader_resources();
		std::ranges::for_each(shaderResources.uniform_buffers, std::bind(applyOffsets, std::placeholders::_1, &D3D11BindingSetOffsets::ConstantBuffer));
		std::ranges::for_each(shaderResources.storage_buffers, std::bind(applyOffsets, std::placeholders::_1, &D3D11BindingSetOffsets::ShaderResource));
		std::ranges::for_each(shaderResources.separate_images, std::bind(applyOffsets, std::placeholders::_1, &D3D11BindingSetOffsets::ShaderResource));
		std::ranges::for_each(shaderResources.storage_images, std::bind(applyOffsets, std::placeholders::_1, &D3D11BindingSetOffsets::UnorderedAccess));
		std::ranges::for_each(shaderResources.separate_samplers, std::bind(applyOffsets, std::placeholders::_1, &D3D11BindingSetOffsets::Sampler));

		if (!shaderResources.push_constant_buffers.empty())
		{
			const auto& resource = shaderResources.push_constant_buffers[0];
			const auto& pushConstant = *reflection.PushConstant;

			compiler.set_decoration(resource.id, spv::DecorationBinding, pushConstant.Slot);
			compiler.set_decoration(resource.id, spv::DecorationDescriptorSet, 0);
		}

		m_HLSLSource[stage] = compiler.compile();
		SK_CORE_TRACE_TAG("ShaderCompiler", "Cross compiled {} shader '{}':\n{}", stage, m_Info.SourcePath, m_HLSLSource.at(stage));
	}

}
