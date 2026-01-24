#include "skpch.h"
#include "Material.h"

namespace Shark {

	namespace utils {

		static const std::string& GetMaterialName(Ref<Shader> shader, const std::string& name)
		{
			if (name.empty() && shader->GetName().empty())
				return "<UNNAMED>";

			if (name.empty())
				shader->GetName();
			return name;
		}

	}

	Material::Material(Ref<Shader> shader, const std::string& name)
		: m_Shader(shader), m_InputManager({ .Shader = shader, .StartSet = 0, .EndSet = 0, .DebugName = utils::GetMaterialName(shader, name) }), m_Name(utils::GetMaterialName(shader, name))
	{
		SK_CORE_VERIFY(shader->GetLayoutMode() != LayoutShareMode::PassOnly);

		const auto& layout = shader->GetReflectionData();
		if (layout.BindingLayouts.empty())
			return;

		for (const auto& [slot, buffer] : layout.BindingLayouts[0].ConstantBuffers)
		{
			auto constantBuffer = ConstantBuffer::Create(buffer.StructSize, fmt::format("{}.{}", name, buffer.Name));
			m_ConstantBuffers[buffer.Name] = constantBuffer;
			m_InputManager.SetInput(buffer.Name, constantBuffer);
		}

		SK_CORE_TRACE_TAG("Renderer", "Material {}:'{}' created", shader->GetName(), name);
	}

	Material::~Material()
	{

	}

	void Material::Prepare()
	{
		m_InputManager.Bake();
	}

	bool Material::Validate() const
	{
		return m_InputManager.Validate();
	}

	void Material::Set(const std::string& name, Ref<Image2D> image, uint32_t arrayIndex)
	{
		m_InputManager.SetInput(name, image, arrayIndex);
	}

	void Material::Set(const std::string& name, Ref<Texture2D> texture, uint32_t arrayIndex)
	{
		m_InputManager.SetInput(name, texture, arrayIndex);
	}

	void Material::Set(const std::string& name, Ref<TextureCube> textureCube, uint32_t arrayIndex)
	{
		m_InputManager.SetInput(name, textureCube, arrayIndex);
	}

	void Material::Set(const std::string& name, Ref<Sampler> sampler, uint32_t arrayIndex)
	{
		m_InputManager.SetInput(name, sampler, arrayIndex);
	}

	void Material::Set(const std::string& name, const Buffer data)
	{
		if (!m_ConstantBuffers.contains(name))
		{
			SK_CORE_WARN_TAG("Renderer", "[Material '{}'] Input '{}' not found", m_Name, name);
			return;
		}

		auto constantBuffer = m_ConstantBuffers.at(name);
		constantBuffer->Upload(data);
	}

}
