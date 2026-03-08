#include "skpch.h"
#include "VulkanShaderCompiler.h"

#include "Shark/Render/Renderer.h"

#include <spirv_cross/spirv_cross.hpp>
#include <nvrhi/nvrhi.h>

namespace Shark {

	Vulkan::ShaderCompiler::ShaderCompiler(const CompilerOptions& options)
		: m_Options(options)
	{
	}

	bool Vulkan::ShaderCompiler::Reload(const ShaderInfo& info, nvrhi::ShaderType compiledStages, CompilerResult& result)
	{
		auto& shaderCache = Renderer::GetShaderCache();
		auto& platformBinary = result.PlatformBinary[nvrhi::GraphicsAPI::VULKAN];

		for (const auto& [stage, spirv] : result.SpirvBinary)
		{
			CacheStatus status = shaderCache.GetCacheStatus(info, stage, nvrhi::GraphicsAPI::VULKAN);
			if (!m_Options.Force && status == CacheStatus::OK)
			{
				Buffer binary;
				if (shaderCache.LoadBinary(info, stage, nvrhi::GraphicsAPI::VULKAN, binary))
				{
					platformBinary[stage] = std::move(binary);
					SK_CORE_TRACE_TAG("ShaderCompiler", "Loaded vulkan {} shader '{}' from cache", stage, info.SourcePath);
					continue;
				}
			}

			if (!CompileStage(info, stage, result) && status == CacheStatus::OutOfDate)
			{
				SK_CORE_WARN_TAG("ShaderCompiler", "[vulkan] Loading older shader version from cache");
				shaderCache.LoadBinary(info, stage, nvrhi::GraphicsAPI::VULKAN, platformBinary[stage]);
			}

			if (!platformBinary.contains(stage) || !platformBinary.at(stage))
			{
				SK_CORE_ERROR_TAG("ShaderCompiler", "Compiling shader '{}' failed!", info.SourcePath.filename());
				SK_DEBUG_BREAK_CONDITIONAL(BREAK_ON_FAILED_COMPILATION);

				for (auto& [stage, binary] : platformBinary)
					binary.Release();
				result.PlatformBinary.erase(nvrhi::GraphicsAPI::VULKAN);
				return false;
			}

			shaderCache.SaveBinary(info, stage, nvrhi::GraphicsAPI::VULKAN, platformBinary.at(stage));
		}

		return true;
	}

	bool Vulkan::ShaderCompiler::CompileStage(const ShaderInfo& info, nvrhi::ShaderType stage, CompilerResult& result)
	{
		std::vector<uint32_t> binary = result.SpirvBinary.at(stage);

		spirv_cross::Compiler compiler(binary);
		const spirv_cross::ShaderResources shaderResources = compiler.get_shader_resources();

		const nvrhi::VulkanBindingOffsets defaultOffsets;

		const auto& reflection = result.Reflection;
		const auto applyOffsets = [&compiler, &reflection, &defaultOffsets, &binary](const spirv_cross::Resource& resource, uint32_t nvrhi::VulkanBindingOffsets::* offsets)
		{
			const uint32_t set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
			const uint32_t slot = compiler.get_decoration(resource.id, spv::DecorationBinding);

			uint32_t wordOffset;
			if (!compiler.get_binary_offset_for_decoration(resource.id, spv::DecorationBinding, wordOffset))
				return;

			binary[wordOffset] = slot + defaultOffsets.*offsets;
		};

		std::ranges::for_each(shaderResources.uniform_buffers, std::bind(applyOffsets, std::placeholders::_1, &nvrhi::VulkanBindingOffsets::constantBuffer));
		std::ranges::for_each(shaderResources.storage_buffers, std::bind(applyOffsets, std::placeholders::_1, &nvrhi::VulkanBindingOffsets::shaderResource));
		std::ranges::for_each(shaderResources.separate_images, std::bind(applyOffsets, std::placeholders::_1, &nvrhi::VulkanBindingOffsets::shaderResource));
		std::ranges::for_each(shaderResources.storage_images, std::bind(applyOffsets, std::placeholders::_1, &nvrhi::VulkanBindingOffsets::unorderedAccess));
		std::ranges::for_each(shaderResources.separate_samplers, std::bind(applyOffsets, std::placeholders::_1, &nvrhi::VulkanBindingOffsets::sampler));

		Buffer& platformBinary = result.PlatformBinary[nvrhi::GraphicsAPI::VULKAN][stage];

		const uint64_t byteSize = binary.size() * sizeof(uint32_t);
		platformBinary.Allocate(byteSize);
		platformBinary.Write(binary.data(), byteSize);
		return true;
	}

}
