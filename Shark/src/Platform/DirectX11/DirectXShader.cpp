#include "skpch.h"
#include "DirectXShader.h"

#include "Shark/Core/Application.h"
#include "Shark/Core/Timer.h"

#include "Shark/Render/Renderer.h"
#include "Shark/File/FileSystem.h"
#include "Shark/Utils/String.h"
#include "Shark/Debug/Profiler.h"

#include "Platform/DirectX11/DirectXAPI.h"
#include "Platform/DirectX11/DirectXContext.h"
#include "Platform/DirectX11/DirectXRenderCommandBuffer.h"
#include "Platform/DirectX11/DirectXShaderCompiler.h"

#include <d3dcompiler.h>

namespace Shark {

	DirectXShader::DirectXShader()
	{
	}

	DirectXShader::~DirectXShader()
	{
		Release();
	}

	void DirectXShader::Release()
	{
		Renderer::SubmitResourceFree([pixel = m_PixelShader, vertex = m_VertexShader, compute = m_ComputeShader]()
		{
			if (pixel)
				pixel->Release();
			if (vertex)
				vertex->Release();
			if (compute)
				compute->Release();
		});

		m_PixelShader = nullptr;
		m_VertexShader = nullptr;
		m_ComputeShader = nullptr;
	}

	void DirectXShader::RT_Release()
	{
		if (m_PixelShader)
		{
			m_PixelShader->Release();
			m_PixelShader = nullptr;
		}

		if (m_VertexShader)
		{
			m_VertexShader->Release();
			m_VertexShader = nullptr;
		}

		if (m_ComputeShader)
		{
			m_ComputeShader->Release();
			m_ComputeShader = nullptr;
		}
	}

	bool DirectXShader::Reload(bool forceCompile, bool disableOptimization)
	{
		Ref<DirectXShaderCompiler> compiler = Ref<DirectXShaderCompiler>::Create(m_FilePath, disableOptimization);
		if (!compiler->Reload(forceCompile))
			return false;

		LoadShader(compiler->GetShaderBinary());
		SetReflectionData(compiler->GetRefectionData());
		SetHash(compiler->GetHash());

		Renderer::ShaderReloaded(this);
		return true;
	}

	bool DirectXShader::HasResource(const std::string& name) const
	{
		return m_ReflectionData.NameCache.contains(name);
	}

	bool DirectXShader::HasPushConstant() const
	{
		return m_ReflectionData.HasPushConstant;
	}

	bool DirectXShader::HasMember(const std::string& name) const
	{
		return m_ReflectionData.MemberNameCache.contains(name);
	}

	const ShaderReflection::Resource& DirectXShader::GetResourceInfo(const std::string& name) const
	{
		SK_CORE_VERIFY(HasResource(name));
		const auto [set, binding] = m_ReflectionData.NameCache.at(name);
		return m_ReflectionData.Resources.at(set).at(binding);
	}

	const ShaderReflection::Resource& DirectXShader::GetResourceInfo(uint32_t set, uint32_t binding) const
	{
		return m_ReflectionData.Resources.at(set).at(binding);
	}

	const ShaderReflection::Resource& DirectXShader::GetPushConstantInfo() const
	{
		return m_ReflectionData.PushConstant;
	}

	const ShaderReflection::Resource& DirectXShader::GetMembersResourceInfo(const std::string& name) const
	{
		SK_CORE_VERIFY(HasMember(name));
		const auto [set, binding, index] = m_ReflectionData.MemberNameCache.at(name);
		return m_ReflectionData.Resources.at(set).at(binding);
	}

	const ShaderReflection::MemberDeclaration& DirectXShader::GetMemberInfo(const std::string& name) const
	{
		SK_CORE_VERIFY(HasMember(name));
		const auto [set, binding, index] = m_ReflectionData.MemberNameCache.at(name);
		return m_ReflectionData.Members.at(set).at(binding).at(index);
	}

	const std::string& DirectXShader::GetResourceName(uint32_t set, uint32_t binding) const
	{
		SK_CORE_VERIFY(m_ReflectionData.Resources.contains(set) && m_ReflectionData.Resources.at(set).contains(binding));
		return m_ReflectionData.Resources.at(set).at(binding).Name;
	}

	std::pair<uint32_t, uint32_t> DirectXShader::GetResourceBinding(const std::string& name) const
	{
		SK_CORE_VERIFY(HasResource(name));
		return m_ReflectionData.NameCache.at(name);
	}

	std::tuple<uint32_t, uint32_t, uint32_t> DirectXShader::GetMemberBinding(const std::string& name) const
	{
		SK_CORE_VERIFY(HasMember(name));
		return m_ReflectionData.MemberNameCache.at(name);
	}

	void DirectXShader::LoadShader(const std::unordered_map<ShaderUtils::ShaderStage, std::vector<byte>>& shaderBinary)
	{
		m_ShaderBinary = shaderBinary;

		Ref<DirectXShader> instance = this;
		Renderer::Submit([instance]()
		{
			auto device = DirectXContext::GetCurrentDevice();
			auto dxDevice = device->GetDirectXDevice();

			instance->RT_Release();

			for (const auto& [stage, binary] : instance->m_ShaderBinary)
			{
				switch (stage)
				{
					case ShaderUtils::ShaderStage::Vertex:
						DirectXAPI::CreateVertexShader(dxDevice, Buffer::FromArray(binary), nullptr, instance->m_VertexShader);
						DirectXAPI::SetDebugName(instance->m_VertexShader, instance->m_Name);
						break;
					case ShaderUtils::ShaderStage::Pixel:
						DirectXAPI::CreatePixelShader(dxDevice, Buffer::FromArray(binary), nullptr, instance->m_PixelShader);
						DirectXAPI::SetDebugName(instance->m_PixelShader, instance->m_Name);
						break;
					case ShaderUtils::ShaderStage::Compute:
						DirectXAPI::CreateComputeShader(dxDevice, Buffer::FromArray(binary), nullptr, instance->m_ComputeShader);
						DirectXAPI::SetDebugName(instance->m_ComputeShader, instance->m_Name);
						break;
					default:
						SK_CORE_VERIFY(false, "Unkown Shader Stage");
						break;
				}
			}
		});
	}

}