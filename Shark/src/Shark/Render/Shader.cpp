#include "skpch.h"
#include "Shader.h"

#include "Shark/Core/Application.h"
#include "Shark/Render/Renderer.h"
#include "Shark/Render/ShaderCompiler/ShaderCompiler.h"

namespace Shark {

	template<auto Value>
	struct Equals
	{
		constexpr bool operator()(const auto& other) const { return Value == other; }
	};

	namespace utils {

		nvrhi::BindingSetHandle CreateSamplerBindingSet(nvrhi::IBindingLayout* layout)
		{
			const auto& samplers = Renderer::GetSamplers();
			const auto& desc = *layout->getDesc();

			SK_CORE_VERIFY(desc.bindings.size() == 6);
			SK_CORE_VERIFY(std::ranges::all_of(desc.bindings, Equals<nvrhi::ResourceType::Sampler>{}, [](const auto& b) { return b.type; }));

			nvrhi::BindingSetDesc set;
			set.trackLiveness = false;
			set.bindings = {
				nvrhi::BindingSetItem::Sampler(desc.bindings[0].slot, samplers.NearestRepeat->GetHandle()),
				nvrhi::BindingSetItem::Sampler(desc.bindings[1].slot, samplers.NearestClamp->GetHandle()),
				nvrhi::BindingSetItem::Sampler(desc.bindings[2].slot, samplers.NearestMirrorRepeat->GetHandle()),
				nvrhi::BindingSetItem::Sampler(desc.bindings[3].slot, samplers.LinearRepeat->GetHandle()),
				nvrhi::BindingSetItem::Sampler(desc.bindings[4].slot, samplers.LinearClamp->GetHandle()),
				nvrhi::BindingSetItem::Sampler(desc.bindings[5].slot, samplers.LinearMirrorRepeat->GetHandle()),
			};

			auto device = Renderer::GetGraphicsDevice();
			return device->createBindingSet(set, layout);
		}

	}

	Shader::Shader(Scope<CompilerResult> result, const std::string& name)
		: m_Name(name)
	{
		m_ReflectionData = result->Reflection;
		m_LayoutMode = result->LayoutMode;

		CreateBindingLayout();

		// #TODO #Renderer bad implementation because d3d11...
		for (const auto& bindingName : result->RequestedBindingSets)
		{
			if (bindingName == "samplers")
			{
				m_RequestedBindingSets[3] = utils::CreateSamplerBindingSet(GetBindingLayout(3));
				SK_CORE_VERIFY(m_RequestedBindingSets[3]);
				continue;
			}

			SK_CORE_ASSERT(false, "Invalid bind '{}'", bindingName);
		}

		auto deviceManager = Application::Get().GetDeviceManager();
		auto device = deviceManager->GetDevice();

		const auto& platformBinary = result->PlatformBinary.at(deviceManager->GetGraphicsAPI());

		for (const auto& [stage, binary] : platformBinary)
		{
			auto shaderDesc = nvrhi::ShaderDesc()
				.setShaderType(stage)
				.setDebugName(m_Name);

			nvrhi::ShaderHandle shader = device->createShader(shaderDesc, binary.Data, binary.Size);
			m_ShaderHandles[stage] = shader;
		}
	}

	Shader::~Shader()
	{
	}

	nvrhi::ShaderHandle Shader::GetHandle(nvrhi::ShaderType stage) const
	{
		if (m_ShaderHandles.contains(stage))
			return m_ShaderHandles.at(stage);
		return nullptr;
	}

	void Shader::CreateBindingLayout()
	{
		m_LayoutMapping.fill(-1);

		auto deviceManager = Application::Get().GetDeviceManager();
		auto device = deviceManager->GetDevice();

		const bool usesOffsets = deviceManager->GetGraphicsAPI() == nvrhi::GraphicsAPI::D3D11;

		static D3D11BindingSetOffsets s_NullOffsets = {};
		const D3D11BindingSetOffsets* bindingOffsets = &s_NullOffsets;

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
				layoutDesc.addItem(nvrhi::BindingLayoutItem::Texture_SRV(layoutItem.Slot + bindingOffsets->ShaderResource).setSize(layoutItem.ArraySize));
			}

			for (const auto& [slot, layoutItem] : layout.StorageImages)
			{
				layoutDesc.visibility |= layoutItem.Stage;
				layoutDesc.addItem(nvrhi::BindingLayoutItem::Texture_UAV(layoutItem.Slot + bindingOffsets->UnorderedAccess).setSize(layoutItem.ArraySize));
			}

			for (const auto& [slot, layoutItem] : layout.Samplers)
			{
				layoutDesc.visibility |= layoutItem.Stage;
				layoutDesc.addItem(nvrhi::BindingLayoutItem::Sampler(layoutItem.Slot + bindingOffsets->Sampler).setSize(layoutItem.ArraySize));
			}

			if (layoutDesc.bindings.empty())
				continue;

			nvrhi::BindingLayoutHandle bindingLayout = device->createBindingLayout(layoutDesc);
			m_BindingLayouts.push_back(bindingLayout);
			m_LayoutMapping[set] = static_cast<int>(m_BindingLayouts.size()) - 1;
		}

		if (m_BindingLayouts.empty() && m_ReflectionData.PushConstant)
		{
			nvrhi::BindingLayoutDesc layoutDesc;
			layoutDesc.visibility = m_ReflectionData.PushConstant->Stage;
			layoutDesc.addItem(nvrhi::BindingLayoutItem::PushConstants(m_ReflectionData.PushConstant->Slot, m_ReflectionData.PushConstant->StructSize));

			nvrhi::BindingLayoutHandle bindingLayout = device->createBindingLayout(layoutDesc);
			m_BindingLayouts.push_back(bindingLayout);
			m_LayoutMapping[0] = 0;
		}

	}

	//////////////////////////////////////////////////////////////////////////
	//// Shader Library
	//////////////////////////////////////////////////////////////////////////

	Ref<Shader> ShaderLibrary::Load(const std::filesystem::path& filepath, const LoadArgs& options)
	{
		CompilerOptions compilerOptions;
		compilerOptions.Force = options.ForceCompile.value_or(m_DefaultOptions.ForceCompile);
		compilerOptions.Optimize = options.Optimize.value_or(m_DefaultOptions.Optimize);
		compilerOptions.GenerateDebugInfo = options.GenerateDebugInfo.value_or(m_DefaultOptions.GenerateDebugInfo);

		ShaderCompiler compiler(filepath, compilerOptions);
		if (!compiler.Reload())
			return nullptr;

		Scope<CompilerResult> result;
		compiler.GetResult(result);

		auto shader = Shader::Create(std::move(result), compiler.GetName());

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