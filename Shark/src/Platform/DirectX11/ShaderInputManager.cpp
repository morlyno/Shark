#include "skpch.h"
#include "ShaderInputManager.h"

namespace Shark {

	namespace utils {

		static bool IsTypeCompadible(InputResourceType inputType, ShaderReflection::ResourceType resourceType)
		{
			switch (inputType)
			{
				case InputResourceType::None: return resourceType == ShaderReflection::ResourceType::None;
				case InputResourceType::Image2D: return resourceType == ShaderReflection::ResourceType::Image2D;
				case InputResourceType::Texture2D: return resourceType == ShaderReflection::ResourceType::Texture2D;
				case InputResourceType::TextureCube: return resourceType == ShaderReflection::ResourceType::TextureCube;
				case InputResourceType::Sampler: return resourceType == ShaderReflection::ResourceType::Sampler;
				case InputResourceType::ConstantBuffer: return resourceType == ShaderReflection::ResourceType::ConstantBuffer;
			}

			SK_CORE_ASSERT(false, "Unkown InputResourceType");
			return false;
		}

	}

	ShaderInputManager::ShaderInputManager(Ref<Shader> shader)
		: m_Shader(shader)
	{
	}

	void ShaderInputManager::Update()
	{
		m_BoundResources.clear();

		for (const auto& [name, input] : m_InputResources)
		{
			if (!m_Shader->HasResource(name))
				continue;

			const auto& [set, binding] = m_Shader->GetResourceBinding(name);
			const auto& info = m_Shader->GetResourceInfo(name);

			// Input declared in shader but not used
			if (info.DXBinding == (uint32_t)-1)
				continue;

			BoundResource& bound = m_BoundResources.emplace_back();
			bound.Input = input.Input;
			bound.Type = input.Type;
			bound.Set = set;
			bound.Binding = binding;
		}
	}

	bool ShaderInputManager::ValidateMaterialInputs() const
	{
		const auto& refelectionData = m_Shader->GetReflectionData();

		if (!refelectionData.Resources.contains(0))
			return true;

		const auto& set0Resources = refelectionData.Resources.at(0);

		for (const auto& [binding, info] : set0Resources)
		{
			if (!m_InputResources.contains(info.Name))
			{
				SK_CORE_ERROR_TAG("Renderer", "[ShaderInputManager] No input resource for 0.{}", binding);
				SK_CORE_ERROR_TAG("Renderer", "[ShaderInputManager] Required resource is {} ({})", info.Name, ToString(info.Type));
				return false;
			}

			const auto& input = m_InputResources.at(info.Name);
			if (input.Input == nullptr)
			{
				SK_CORE_ERROR_TAG("Renderer", "[ShaderInputManager] Input is null for 0.{}", binding);
				SK_CORE_ERROR_TAG("Renderer", "[ShaderInputManager] Required input is {} ({})", info.Name, ToString(info.Type));
				return false;
			}

			if (!utils::IsTypeCompadible(input.Type, info.Type))
			{
				SK_CORE_ERROR_TAG("Renderer", "[ShaderInputManager] Incompadible Type for 0.{}", binding);
				SK_CORE_ERROR_TAG("Renderer", "[ShaderInputManager] Required is {} but Input is {}", ToString(info.Type), ToString(input.Type));
				return false;
			}
		}

		if (m_InputResources.size() != set0Resources.size())
		{
			SK_CORE_WARN_TAG("Renderer", "[RenderPass] More inputs set than the shader expects");
			SK_CORE_WARN_TAG("Renderer", "[RenderPass] {} inputs expected but {} provided", set0Resources.size(), m_InputResources.size());
		}

		return true;
	}

	bool ShaderInputManager::ValidateRenderPassInputs() const
	{
		const auto& refelectionData = m_Shader->GetReflectionData();

		uint32_t resourceCount = 0;

		for (const auto& [set, setResources] : refelectionData.Resources)
		{
			if (set < 1)
				continue;

			for (const auto& [binding, info] : setResources)
			{
				if (!m_InputResources.contains(info.Name))
				{
					SK_CORE_ERROR_TAG("Renderer", "[ShaderInputManager] No input resource for {}.{}", set, binding);
					SK_CORE_ERROR_TAG("Renderer", "[ShaderInputManager] Required resource is {} ({})", info.Name, ToString(info.Type));
					return false;
				}

				const auto& input = m_InputResources.at(info.Name);
				if (input.Input == nullptr)
				{
					SK_CORE_ERROR_TAG("Renderer", "[ShaderInputManager] Input is null for {}.{}", set, binding);
					SK_CORE_ERROR_TAG("Renderer", "[ShaderInputManager] Required input is {} ({})", info.Name, ToString(info.Type));
					return false;
				}

				if (!utils::IsTypeCompadible(input.Type, info.Type))
				{
					SK_CORE_ERROR_TAG("Renderer", "[ShaderInputManager] Incompadible Type for {}.{}", set, binding);
					SK_CORE_ERROR_TAG("Renderer", "[ShaderInputManager] Required is {} but Input is {}", ToString(info.Type), ToString(input.Type));
					return false;
				}

				resourceCount++;
			}
		}

		if (m_InputResources.size() != resourceCount)
		{
			SK_CORE_WARN_TAG("Renderer", "[RenderPass] More inputs set than the shader expects");
			SK_CORE_WARN_TAG("Renderer", "[RenderPass] {} inputs expected but {} provided", resourceCount, m_InputResources.size());
		}

		return true;
	}

	void ShaderInputManager::SetInput(const std::string& name, Ref<ConstantBuffer> constantBuffer)
	{
		auto& input = m_InputResources[name];
		input.Input = constantBuffer;
		input.Type = InputResourceType::ConstantBuffer;
	}

	void ShaderInputManager::SetInput(const std::string& name, Ref<Image2D> image)
	{
		auto& input = m_InputResources[name];
		input.Input = image;
		input.Type = InputResourceType::Image2D;
	}

	void ShaderInputManager::SetInput(const std::string& name, Ref<Texture2D> texture)
	{
		auto& input = m_InputResources[name];
		input.Input = texture;
		input.Type = InputResourceType::Texture2D;
	}

	void ShaderInputManager::SetInput(const std::string& name, Ref<TextureCube> textureCube)
	{
		auto& input = m_InputResources[name];
		input.Input = textureCube;
		input.Type = InputResourceType::TextureCube;
	}

	void ShaderInputManager::SetInput(const std::string& name, Ref<SamplerWrapper> sampler)
	{
		auto& input = m_InputResources[name];
		input.Input = sampler;
		input.Type = InputResourceType::Sampler;
	}

	bool ShaderInputManager::HasInput(const std::string& name) const
	{
		return m_InputResources.contains(name);
	}

	void ShaderInputManager::GetResource(const std::string& name, Ref<ConstantBuffer>& outConstantBuffer) const
	{
		if (m_InputResources.contains(name))
		{
			auto& input = m_InputResources.at(name);
			SK_CORE_ASSERT(input.Type == InputResourceType::ConstantBuffer);
			outConstantBuffer = input.Input.As<ConstantBuffer>();
		}
	}

	void ShaderInputManager::GetResource(const std::string& name, Ref<Image2D>& outImage2D) const
	{
		if (m_InputResources.contains(name))
		{
			auto& input = m_InputResources.at(name);
			SK_CORE_ASSERT(input.Type == InputResourceType::Image2D);
			outImage2D = input.Input.As<Image2D>();
		}
	}

	void ShaderInputManager::GetResource(const std::string& name, Ref<Texture2D>& outTexture2D) const
	{
		if (m_InputResources.contains(name))
		{
			auto& input = m_InputResources.at(name);
			SK_CORE_ASSERT(input.Type == InputResourceType::Texture2D);
			outTexture2D = input.Input.As<Texture2D>();
		}
	}

	void ShaderInputManager::GetResource(const std::string& name, Ref<TextureCube>& outTextureCube) const
	{
		if (m_InputResources.contains(name))
		{
			auto& input = m_InputResources.at(name);
			SK_CORE_ASSERT(input.Type == InputResourceType::TextureCube);
			outTextureCube = input.Input.As<TextureCube>();
		}
	}

	void ShaderInputManager::GetResource(const std::string& name, Ref<SamplerWrapper>& outSampler) const
	{
		if (m_InputResources.contains(name))
		{
			auto& input = m_InputResources.at(name);
			SK_CORE_ASSERT(input.Type == InputResourceType::Sampler);
			outSampler = input.Input.As<SamplerWrapper>();
		}
	}

	std::string ToString(InputResourceType type)
	{
		switch (type)
		{
			case InputResourceType::None: return "None";
			case InputResourceType::Image2D: return "Image2D";
			case InputResourceType::Texture2D: return "Texture2D";
			case InputResourceType::TextureCube: return "TextureCube";
			case InputResourceType::Sampler: return "Sampler";
			case InputResourceType::ConstantBuffer: return "ConstantBuffer";
		}
		SK_CORE_ASSERT(false, "Unkown InputResourceType");
		return "Unkown";
	}

}