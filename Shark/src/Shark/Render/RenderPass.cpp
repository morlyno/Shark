#include "skpch.h"
#include "RenderPass.h"
#include "Renderer.h"

namespace Shark {

	RenderPass::RenderPass(const RenderPassSpecification& specification)
		: m_Specification(specification)
	{
		const auto layoutMode = m_Specification.Shader->GetLayoutMode();
		if (layoutMode == LayoutShareMode::MaterialOnly)
			return;

		ShaderInputManagerSpecification inputManagerSpec;
		inputManagerSpec.Shader = m_Specification.Shader;
		inputManagerSpec.DebugName = m_Specification.DebugName;
		if (layoutMode == LayoutShareMode::PassOnly)
			inputManagerSpec.StartSet = 0;

		m_InputManager.Initialize(inputManagerSpec);
	}

	RenderPass::~RenderPass()
	{

	}

	void RenderPass::Bake()
	{
		std::vector<InputUpdate> updates;
		m_InputManager.Package(updates);

		SK_CORE_ASSERT(!updates.empty());

		Ref instance = this;
		Renderer::Submit([instance, temp = std::move(updates)]()
		{
			instance->m_InputManager.Update(temp, true);
		});
	}

	bool RenderPass::Validate() const
	{
		return m_InputManager.Validate();
	}

	void RenderPass::Update()
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

	void RenderPass::UpdateDescriptors()
	{
		m_InputManager.PrepareAll();
		Bake(); // Force updates
	}

	void RenderPass::SetInput(const std::string& name, Ref<ConstantBuffer> constantBuffer)
	{
		m_InputManager.SetInput(name, constantBuffer);
	}

	void RenderPass::SetInput(const std::string& name, Ref<StorageBuffer> storageBuffer)
	{
		m_InputManager.SetInput(name, storageBuffer);
	}

	void RenderPass::SetInput(const std::string& name, Ref<Image2D> image, uint32_t arrayIndex)
	{
		m_InputManager.SetInput(name, image, arrayIndex);
	}

	void RenderPass::SetInput(const std::string& name, Ref<Texture2D> texture, uint32_t arrayIndex)
	{
		m_InputManager.SetInput(name, texture, arrayIndex);
	}

	void RenderPass::SetInput(const std::string& name, Ref<TextureCube> textureCube, uint32_t arrayIndex)
	{
		m_InputManager.SetInput(name, textureCube, arrayIndex);
	}

	void RenderPass::SetInput(const std::string& name, Ref<Sampler> sampler, uint32_t arrayIndex)
	{
		m_InputManager.SetInput(name, sampler, arrayIndex);
	}

	Ref<Image2D> RenderPass::GetOutput(uint32_t index) const
	{
		return m_Specification.TargetFramebuffer->GetImage(index);
	}

	Ref<Image2D> RenderPass::GetDepthOutput() const
	{
		return m_Specification.TargetFramebuffer->GetDepthImage();
	}

	Ref<Shader> RenderPass::GetShader() const
	{
		return m_Specification.Shader;
	}

	Ref<FrameBuffer> RenderPass::GetTargetFramebuffer() const
	{
		return m_Specification.TargetFramebuffer;
	}

	void RenderPass::SetTargetFramebuffer(Ref<FrameBuffer> targetFramebuffer)
	{
		m_Specification.TargetFramebuffer = targetFramebuffer;
	}

}
