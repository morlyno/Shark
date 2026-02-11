#include "skpch.h"
#include "ComputePass.h"

#include "Shark/Render/Renderer.h"

namespace Shark {

	ComputePass::ComputePass(const ComputePassSpecification& specification)
		: m_Specification(specification)
	{
		const auto layoutMode = m_Specification.ComputeShader->GetLayoutMode();
		if (layoutMode == LayoutShareMode::MaterialOnly)
			return;

		ShaderInputManagerSpecification inputManagerSpec;
		inputManagerSpec.Shader = m_Specification.ComputeShader;
		inputManagerSpec.DebugName = m_Specification.DebugName;
		if (layoutMode == LayoutShareMode::PassOnly)
			inputManagerSpec.StartSet = 0;

		m_InputManager.Initialize(inputManagerSpec);
	}

	ComputePass::ComputePass(Ref<Shader> computeShader, const std::string& debugName)
		: ComputePass({ .ComputeShader = computeShader, .DebugName = debugName })
	{
	}

	ComputePass::~ComputePass()
	{

	}

	void ComputePass::Bake()
	{
		std::vector<InputUpdate> updates;
		m_InputManager.Package(updates);

		Ref instance = this;
		Renderer::Submit([instance, temp = std::move(updates)]()
		{
			instance->m_InputManager.Update(temp, true);
		});
	}

	void ComputePass::Update()
	{
		std::vector<InputUpdate> updates;
		if (!m_InputManager.Package(updates))
			return; // No update needed

		Ref instance = this;
		Renderer::Submit([instance, temp = std::move(updates)]()
		{
			instance->m_InputManager.Update(temp);
		});
	}

	bool ComputePass::Validate() const
	{
		return m_InputManager.Validate();
	}

	void ComputePass::SetInput(const std::string& name, Ref<ConstantBuffer> constantBuffer)
	{
		m_InputManager.SetInput(name, constantBuffer);
	}

	void ComputePass::SetInput(const std::string& name, Ref<StorageBuffer> storageBuffer)
	{
		m_InputManager.SetInput(name, storageBuffer);
	}

	void ComputePass::SetInput(const std::string& name, Ref<Image2D> image, uint32_t arrayIndex)
	{
		m_InputManager.SetInput(name, image, arrayIndex);
	}

	void ComputePass::SetInput(const std::string& name, Ref<Image2D> image, const nvrhi::TextureSubresourceSet& subresource, uint32_t arrayIndex)
	{
		m_InputManager.SetInput(name, image, { .SubresourceSet = subresource }, arrayIndex);
	}

	void ComputePass::SetInput(const std::string& name, Ref<Texture2D> image, uint32_t arrayIndex)
	{
		m_InputManager.SetInput(name, image, arrayIndex);
	}

	void ComputePass::SetInput(const std::string& name, Ref<TextureCube> textureCube, uint32_t arrayIndex)
	{
		m_InputManager.SetInput(name, textureCube, arrayIndex);
	}

	void ComputePass::SetInput(const std::string& name, Ref<Sampler> sampler, uint32_t arrayIndex)
	{
		m_InputManager.SetInput(name, sampler, arrayIndex);
	}

}
