#include "skpch.h"
#include "D3D11ShaderCompiler.h"

#include "Shark/Utils/String.h"
#include "Shark/Render/ShaderCompiler/ShaderCompiler.h"

#include <d3dcompiler.h>
#include <shaderc/shaderc.h>
#include <spirv_cross/spirv_hlsl.hpp>

namespace Shark {
	
	D3D11ShaderCompilerInterface::D3D11ShaderCompilerInterface(ShaderCompiler& compiler)
		: m_Compiler(compiler)
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

	std::string D3D11ShaderCompilerInterface::CompileStage(nvrhi::ShaderType stage)
	{
		CrossCompileStage(stage);

		UINT flags = 0;
		flags |= D3DCOMPILE_ALL_RESOURCES_BOUND;

		if (m_Compiler.m_Options.Optimize)
			flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;

		if (m_Compiler.m_Options.GenerateDebugInfo)
			flags |= D3DCOMPILE_DEBUG;

		ID3DBlob* shaderBinary = nullptr;
		ID3DBlob* errorMessage = nullptr;
		const std::string name = fmt::format("{}{}", m_Compiler.m_Info.SourcePath.stem().string(), s_ShaderTypeMappings.at(stage).Extension);
		const auto& hlslSource = m_HLSLSource.at(stage);

		if (FAILED(D3DCompile(hlslSource.data(), hlslSource.size(), name.c_str(), nullptr, nullptr, "main", s_ShaderTypeMappings.at(stage).D3D11, flags, 0, &shaderBinary, &errorMessage)))
		{
			std::string errorStr = fmt::format("Failed to Compile HLSL (Stage: {0})\n{1}", stage, (char*)errorMessage->GetBufferPointer());
			errorMessage->Release();
			return errorStr;
		}

		m_D3D11Binary[stage] = Buffer::Copy(shaderBinary->GetBufferPointer(), shaderBinary->GetBufferSize());
		shaderBinary->Release();
		return {};
	}

	void D3D11ShaderCompilerInterface::ReflectStage(nvrhi::ShaderType stage)
	{
		auto binary = Buffer(m_D3D11Binary.at(stage));
		D3DReflect(binary.As<const void>(), binary.Size, IID_PPV_ARGS(&m_CurrentReflector));

		if (m_Compiler.m_ReflectionData.HasPushConstant)
		{
			auto& resource = m_Compiler.m_ReflectionData.PushConstant;
			ReflectResource(resource);
		}

		for (auto& [set, bindingSet] : m_Compiler.m_ReflectionData.Resources)
		{
			for (auto& [binding, resource] : bindingSet)
			{
				ReflectResource(resource);
			}
		}

		m_CurrentReflector->Release();
		m_CurrentReflector = nullptr;
	}

	bool D3D11ShaderCompilerInterface::LoadOrCompileStage(nvrhi::ShaderType stage, ShaderCache& shaderCache)
	{
		const auto& sourceInfo = m_Compiler.m_SourceInfo.at(stage);

		auto syncState = shaderCache.GetD3D11CacheSyncState(sourceInfo);
		if (syncState == ShaderCacheState::UpToDate)
		{
			shaderCache.LoadD3D11(sourceInfo, GetBinary(stage));
			return true;
		}

		std::string errorMessage = CompileStage(stage);
		if (!errorMessage.empty())
		{
			SK_CORE_ERROR_TAG("Renderer", "Failed to compile d3d11 {} shader '{}'.\n{}", stage, m_Compiler.m_Info.SourcePath, errorMessage);

			if (syncState == ShaderCacheState::Missing)
				return false;

			SK_CORE_WARN_TAG("Renderer", "Loading older d3d11 shader version from cache");
			return shaderCache.LoadD3D11(sourceInfo, GetBinary(stage));
		}

		return true;
	}

	void D3D11ShaderCompilerInterface::CrossCompileStage(nvrhi::ShaderType stage)
	{
		spirv_cross::CompilerHLSL compiler(m_Compiler.m_SpirvBinary.at(stage));
		spirv_cross::CompilerHLSL::Options options;

		options.shader_model = 50;

		compiler.set_hlsl_options(options);
		compiler.set_resource_binding_flags(spirv_cross::HLSL_BINDING_AUTO_ALL);

		if (stage == nvrhi::ShaderType::Vertex)
		{
			spirv_cross::ShaderResources shaderResources = compiler.get_shader_resources();
			for (const auto& resource : shaderResources.stage_inputs)
			{
				std::string name = resource.name.substr(m_Compiler.m_Info.Language == ShaderLanguage::HLSL ? 7 : 2);
				uint32_t location = compiler.get_decoration(resource.id, spv::DecorationLocation);
				compiler.add_vertex_attribute_remap({ location, name });
			}
		}

		m_HLSLSource[stage] = compiler.compile();
	}

	void D3D11ShaderCompilerInterface::ReflectResource(ShaderReflection::Resource& resource)
	{
		const auto& d3dName = GetReflectionName(resource.Type, resource.Name, resource.ArraySize);

		D3D11_SHADER_INPUT_BIND_DESC bindingDesc;
		m_CurrentReflector->GetResourceBindingDescByName(d3dName.c_str(), &bindingDesc);

		resource.DXBinding = bindingDesc.BindPoint;

		if (resource.Type == ShaderReflection::ResourceType::Sampler)
			resource.DXSamplerBinding = bindingDesc.BindPoint;

		if (ShaderReflection::IsTextureType(resource.Type))
		{
			auto samplerName = resource.ArraySize ? fmt::format("_{}_sampler[{}]", resource.Name, 0) : fmt::format("_{}_sampler", resource.Name);

			m_CurrentReflector->GetResourceBindingDescByName(samplerName.c_str(), &bindingDesc);
			resource.DXSamplerBinding = bindingDesc.BindPoint;
		}
	}

	std::string D3D11ShaderCompilerInterface::GetReflectionName(ShaderReflection::ResourceType type, const std::string& name, uint32_t arraySize)
	{
		if (type == ShaderReflection::ResourceType::ConstantBuffer)
		{
			if (m_Compiler.m_Info.Language == ShaderLanguage::GLSL)
				return name.substr(2);

			std::string temp = name;
			String::Replace(temp, ".", "_");
			return temp;
		}

		// TODO(moro): investiage arrays of other types
		if (arraySize > 0 && ShaderReflection::IsTextureType(type))
		{
			return fmt::format("{}[{}]", name, 0);
		}

		return name;
	}

}
