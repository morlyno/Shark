#include "skpch.h"
#include "Shader.h"

#include "Shark/Core/Application.h"
#include "Shark/Render/ShaderCompiler/ShaderCompiler.h"

namespace Shark {

	Shader::Shader()
	{

	}

	Shader::Shader(Ref<ShaderCompiler> compiler)
	{
		m_Compiler = compiler;
		m_Info = compiler->GetInfo();
		m_Name = m_Info.SourcePath.stem().string();
		InitializeFromCompiler();
		CreateBindingLayout();
	}

	Shader::~Shader()
	{

	}

	bool Shader::Reload(bool forceCompile, bool disableOptimization)
	{
		// #Renderer #TODO Shader::Reload
		SK_NOT_IMPLEMENTED();
		return false;
	}

	nvrhi::ShaderHandle Shader::GetHandle(nvrhi::ShaderType stage) const
	{
		if (m_ShaderHandles.contains(stage))
			return m_ShaderHandles.at(stage);
		return nullptr;
	}

	void Shader::InitializeFromCompiler()
	{
		auto deviceManager = Application::Get().GetDeviceManager();
		auto device = deviceManager->GetDevice();

		auto shaderStages = { nvrhi::ShaderType::Vertex, nvrhi::ShaderType::Pixel, nvrhi::ShaderType::Compute };
		for (auto stage : shaderStages | std::views::filter([this](auto stage) { return m_Compiler->HasStage(stage); }))
		{
			auto shaderDesc = nvrhi::ShaderDesc()
				.setShaderType(stage)
				.setDebugName(m_Name);

			const Buffer binary = m_Compiler->GetBinary(stage, deviceManager->GetGraphicsAPI());
			nvrhi::ShaderHandle shader = device->createShader(shaderDesc, binary.Data, binary.Size);
			m_ShaderHandles[stage] = shader;
		}

		m_ReflectionData = m_Compiler->GetReflectionData();
	}

	void Shader::CreateBindingLayout()
	{
		auto deviceManager = Application::Get().GetDeviceManager();
		auto device = deviceManager->GetDevice();

		const bool usesOffsets = deviceManager->GetGraphicsAPI() == nvrhi::GraphicsAPI::D3D11;

		static D3D11BindingSetOffsets s_NullOffsets = {};
		const D3D11BindingSetOffsets* bindingOffsets = &s_NullOffsets;

		m_BindingLayouts.resize(m_ReflectionData.BindingLayouts.size());
		for (uint32_t set = 0; set < m_ReflectionData.BindingLayouts.size(); set++)
		{
			const auto& layout = m_ReflectionData.BindingLayouts[set];

			nvrhi::BindingLayoutDesc layoutDesc;
			layoutDesc.registerSpace = set;
			layoutDesc.registerSpaceIsDescriptorSet = true;

			if (usesOffsets)
				bindingOffsets = &layout.BindingOffsets;

			if (set == 0 && m_ReflectionData.PushConstant)
			{
				auto& pushConstant = *m_ReflectionData.PushConstant;
				layoutDesc.visibility |= pushConstant.Stage;
				layoutDesc.addItem(nvrhi::BindingLayoutItem::PushConstants(pushConstant.Slot, pushConstant.StructSize));
			}

			for (const auto& [slot, layoutItem] : layout.ConstantBuffers)
			{
				layoutDesc.visibility |= layoutItem.Stage;
				layoutDesc.addItem(nvrhi::BindingLayoutItem::ConstantBuffer(layoutItem.Slot + bindingOffsets->ConstantBuffer));
			}

			for (const auto& [slot, layoutItem] : layout.StorageBuffers)
			{
				layoutDesc.visibility |= layoutItem.Stage;
				layoutDesc.addItem(nvrhi::BindingLayoutItem::StructuredBuffer_SRV(layoutItem.Slot + bindingOffsets->ShaderResource));
			}

			for (const auto& [slot, layoutItem] : layout.Images)
			{
				layoutDesc.visibility |= layoutItem.Stage;
				layoutDesc.addItem(nvrhi::BindingLayoutItem::Texture_SRV(layoutItem.Slot + bindingOffsets->ShaderResource));
			}

			for (const auto& [slot, layoutItem] : layout.StorageImages)
			{
				layoutDesc.visibility |= layoutItem.Stage;
				layoutDesc.addItem(nvrhi::BindingLayoutItem::Texture_UAV(layoutItem.Slot + bindingOffsets->UnorderedAccess));
			}

			for (const auto& [slot, layoutItem] : layout.Samplers)
			{
				layoutDesc.visibility |= layoutItem.Stage;
				layoutDesc.addItem(nvrhi::BindingLayoutItem::Sampler(layoutItem.Slot + bindingOffsets->Sampler));
			}

			// #Investigate empty binding layouts
			// set visibility to all when the layout is empty, because visibility none is not allowed
			if (layoutDesc.bindings.empty())
				layoutDesc.visibility = nvrhi::ShaderType::All;

			nvrhi::BindingLayoutHandle bindingLayout = device->createBindingLayout(layoutDesc);
			m_BindingLayouts[set] = bindingLayout;
		}

		if (m_BindingLayouts.empty() && m_ReflectionData.PushConstant)
		{
			nvrhi::BindingLayoutDesc layoutDesc;
			layoutDesc.visibility = m_ReflectionData.PushConstant->Stage;
			layoutDesc.addItem(nvrhi::BindingLayoutItem::PushConstants(m_ReflectionData.PushConstant->Slot, m_ReflectionData.PushConstant->StructSize));

			nvrhi::BindingLayoutHandle bindingLayout = device->createBindingLayout(layoutDesc);
			m_BindingLayouts.push_back(bindingLayout);
		}

	}

	//////////////////////////////////////////////////////////////////////////
	//// Shader Library
	//////////////////////////////////////////////////////////////////////////

	Ref<Shader> ShaderLibrary::Load(const std::filesystem::path& filepath, bool forceCompile, bool disableOptimization)
	{
		auto compiler = ShaderCompiler::Load(filepath, { .Force = forceCompile, .Optimize = !disableOptimization });
		if (!compiler)
			return nullptr;

		auto shader = Shader::Create(compiler);

		SK_CORE_VERIFY(!Exists(shader->GetName()));
		m_ShaderMap[shader->GetName()] = shader;
		return shader;
	}

	Ref<Shader> ShaderLibrary::Get(const std::string& name)
	{
		SK_CORE_VERIFY(Exists(name), "The shader {} is not loaded!", name);
		return m_ShaderMap.at(name);
	}

	Ref<Shader> ShaderLibrary::TryGet(const std::string& name)
	{
		if (Exists(name))
			return m_ShaderMap.at(name);
		return nullptr;
	}

}