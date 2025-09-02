#include "skpch.h"
#include "DirectX11Shader.h"

#include "Shark/Core/Application.h"
#include "Shark/Render/ShaderCompiler/ShaderCompiler.h"

namespace Shark {

	DirectX11Shader::DirectX11Shader()
	{
	}

	DirectX11Shader::DirectX11Shader(Ref<ShaderCompiler> compiler)
	{
		m_Compiler = compiler;
		m_Info = compiler->GetInfo();
		m_Name = m_Info.SourcePath.stem().string();
		InitializeFromCompiler();
	}

	DirectX11Shader::~DirectX11Shader()
	{
	}

	bool DirectX11Shader::Reload(bool forceCompile, bool disableOptimization)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	nvrhi::ShaderHandle DirectX11Shader::GetHandle(nvrhi::ShaderType stage) const
	{
		if (m_ShaderHandles.contains(stage))
			return m_ShaderHandles.at(stage);
		return nullptr;
	}

	const std::filesystem::path& DirectX11Shader::GetFilePath() const
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	bool DirectX11Shader::HasResource(const std::string& name) const
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	bool DirectX11Shader::HasPushConstant() const
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	bool DirectX11Shader::HasMember(const std::string& name) const
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	const Shark::ShaderReflection::Resource& DirectX11Shader::GetResourceInfo(const std::string& name) const
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	const Shark::ShaderReflection::Resource& DirectX11Shader::GetResourceInfo(uint32_t set, uint32_t binding) const
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	std::pair<uint32_t, uint32_t> DirectX11Shader::GetResourceBinding(const std::string& name) const
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	std::tuple<uint32_t, uint32_t, uint32_t> DirectX11Shader::GetMemberBinding(const std::string& name) const
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	const Shark::ShaderReflection::Resource& DirectX11Shader::GetPushConstantInfo() const
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	const Shark::ShaderReflection::Resource& DirectX11Shader::GetMembersResourceInfo(const std::string& name) const
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	const Shark::ShaderReflection::MemberDeclaration& DirectX11Shader::GetMemberInfo(const std::string& name) const
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	const std::string& DirectX11Shader::GetResourceName(uint32_t set, uint32_t binding) const
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void DirectX11Shader::InitializeFromCompiler()
	{
		nvrhi::IDevice* device = Application::Get().GetDeviceManager()->GetDevice();

		auto shaderStages = { nvrhi::ShaderType::Vertex, nvrhi::ShaderType::Pixel, nvrhi::ShaderType::Compute };
		for (auto stage : shaderStages | std::views::filter([this](auto stage) { return m_Compiler->HasStage(stage); }))
		{
			auto shaderDesc = nvrhi::ShaderDesc()
				.setShaderType(stage)
				.setDebugName(m_Name);

			const Buffer binary = m_Compiler->GetD3D11Compiler().GetBinary(stage);
			nvrhi::ShaderHandle shader = device->createShader(shaderDesc, binary.Data, binary.Size);
			m_ShaderHandles[stage] = shader;
		}

		m_ReflectionData = m_Compiler->GetRelfectionData();
	}

}
