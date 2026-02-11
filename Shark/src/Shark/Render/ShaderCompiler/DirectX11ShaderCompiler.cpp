#include "skpch.h"
#include "DirectX11ShaderCompiler.h"

#include "Shark/Core/Memory.h"
#include "Shark/Render/Renderer.h"
#include "Shark/Render/ShaderCompiler/ShaderCache.h"

#include <spirv_cross/spirv_hlsl.hpp>

#include <dxc/dxcapi.h>
#include <d3dcompiler.h>

namespace Shark {

	namespace DxcInstances {
		extern nvrhi::RefCountPtr<IDxcCompiler3> g_Compiler;
		extern nvrhi::RefCountPtr<IDxcUtils> g_Utils;
		extern nvrhi::RefCountPtr<IDxcIncludeHandler> g_IncludeHandler;
	}

	D3D11::ShaderCompiler::ShaderCompiler(const CompilerOptions& options)
		: m_Options(options)
	{
	}

	bool D3D11::ShaderCompiler::Reload(const ShaderInfo& info, nvrhi::ShaderType compiledStages, CompilerResult& result)
	{
		auto& shaderCache = Renderer::GetShaderCache();
		auto& platformBinary = result.PlatformBinary[nvrhi::GraphicsAPI::D3D11];

		for (const auto& [stage, spirv] : result.SpirvBinary)
		{
			CacheStatus status = shaderCache.GetCacheStatus(info, stage, nvrhi::GraphicsAPI::D3D11);
			if (!m_Options.Force && status == CacheStatus::OK)
			{
				Buffer binary;
				if (shaderCache.LoadBinary(info, stage, nvrhi::GraphicsAPI::D3D11, binary))
				{
					platformBinary[stage] = std::move(binary);
					SK_CORE_TRACE_TAG("ShaderCompiler", "Loaded d3d11 {} shader '{}' from cache", stage, info.SourcePath);
					continue;
				}
			}

			if (!CompileStage(info, stage, result) && status == CacheStatus::OutOfDate)
			{
				SK_CORE_WARN_TAG("ShaderCompiler", "[d3d11] Loading older shader version from cache");
				shaderCache.LoadBinary(info, stage, nvrhi::GraphicsAPI::D3D11, platformBinary[stage]);
			}

			if (!platformBinary.contains(stage) || !platformBinary.at(stage))
			{
				SK_CORE_ERROR_TAG("ShaderCompiler", "Compiling shader '{}' failed!", info.SourcePath.filename());
				SK_DEBUG_BREAK_CONDITIONAL(BREAK_ON_FAILED_COMPILATION);
				
				for (auto& [stage, binary] : platformBinary)
					binary.Release();
				result.PlatformBinary.erase(nvrhi::GraphicsAPI::D3D11);
				return false;
			}

			shaderCache.SaveBinary(info, stage, nvrhi::GraphicsAPI::D3D11, platformBinary.at(stage));
		}

		return true;
	}

	bool D3D11::ShaderCompiler::CompileStage(const ShaderInfo& info, nvrhi::ShaderType stage, CompilerResult& result)
	{
		CrossCompile(info, stage, result);

		std::string errorMessage = CompileHLSL(info, stage, result);

		if (!errorMessage.empty())
		{
			SK_CORE_ERROR_TAG("ShaderCompiler", "[d3d11] Failed to compile {} shader '{}'!\n{}", stage, info.SourcePath, errorMessage);
			return false;
		}

		SK_CORE_WARN_TAG("ShaderCompiler", "[d3d11] Compiled {} shader '{}'", stage, info.SourcePath);
		return true;
	}

	std::string D3D11::ShaderCompiler::CompileHLSL(const ShaderInfo& info, nvrhi::ShaderType stage, CompilerResult& result)
	{
		UINT flags = 0;
		flags |= D3DCOMPILE_ALL_RESOURCES_BOUND;

		if (m_Options.Optimize)
			flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;

		if (m_Options.GenerateDebugInfo)
			flags |= D3DCOMPILE_DEBUG;

		ID3DBlob* shaderBinary = nullptr;
		ID3DBlob* errorMessage = nullptr;
		const std::string name = fmt::format("{}{}", info.SourcePath.stem().string(), s_ShaderTypeMappings.at(stage).Extension);

		const std::string& shaderSource = m_HLSLSource.at(stage);

		if (FAILED(D3DCompile(shaderSource.data(), shaderSource.size(), name.c_str(), nullptr, nullptr, "main", s_ShaderTypeMappings.at(stage).D3D11, flags, 0, &shaderBinary, &errorMessage)))
		{
			std::string errorStr = fmt::format("Failed to Compile HLSL (Stage: {0})\n{1}", stage, (char*)errorMessage->GetBufferPointer());
			errorMessage->Release();
			return errorStr;
		}

		auto& binary = result.PlatformBinary[nvrhi::GraphicsAPI::D3D11][stage];
		Memory::Write(binary, shaderBinary->GetBufferPointer(), shaderBinary->GetBufferSize());

		shaderBinary->Release();
		return {};
	}

	void D3D11::ShaderCompiler::CrossCompile(const ShaderInfo& info, nvrhi::ShaderType stage, CompilerResult& result)
	{
		const auto& spirvBinary = result.SpirvBinary.at(stage);

		spirv_cross::CompilerHLSL compiler(spirvBinary.data(), spirvBinary.size());
		spirv_cross::CompilerHLSL::Options options;

		options.shader_model = 50;
		options.preserve_structured_buffers = true;
		options.use_entry_point_name = true;

		compiler.set_hlsl_options(options);

		/// Vertex attribute remapping
		const auto shaderResources = compiler.get_shader_resources();

		if (stage == nvrhi::ShaderType::Vertex)
		{
			for (const auto& resource : shaderResources.stage_inputs)
			{
				if (resource.name.starts_with("in.var."))
				{
					uint32_t location = compiler.get_decoration(resource.id, spv::DecorationLocation);
					compiler.add_vertex_attribute_remap({ location, resource.name.substr(7) });
				}
			}
		}

		/// Binding set offsets

		const auto& reflection = result.Reflection;
		const auto applyOffsets = [&compiler, &reflection](const spirv_cross::Resource& resource, uint32_t D3D11BindingSetOffsets::* offsets)
		{
			const uint32_t set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
			const uint32_t slot = compiler.get_decoration(resource.id, spv::DecorationBinding);

			const auto& layout = reflection.BindingLayouts[set];
			compiler.set_decoration(resource.id, spv::DecorationBinding, slot + layout.BindingOffsets.*offsets);
			compiler.set_decoration(resource.id, spv::DecorationDescriptorSet, 0);
		};

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
		SK_CORE_TRACE_TAG("ShaderCompiler", "[d3d11] Cross compiled {} shader '{}':\n{}", stage, info.SourcePath, m_HLSLSource.at(stage));
	}

}
