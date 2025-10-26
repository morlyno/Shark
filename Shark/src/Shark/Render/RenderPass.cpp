#include "skpch.h"
#include "RenderPass.h"

namespace Shark {

	RenderPass::RenderPass(const RenderPassSpecification& specification)
		: m_Specification(specification), m_InputManager({ .Shader = specification.Shader, .DebugName = specification.DebugName })
	{
	}

	RenderPass::~RenderPass()
	{

	}

	void RenderPass::Bake()
	{
		m_InputManager.Bake();
	}

	bool RenderPass::Validate() const
	{
		return m_InputManager.Validate();
	}

	void RenderPass::Update()
	{
		//SK_NOT_IMPLEMENTED();
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

	Ref<FrameBuffer> RenderPass::GetTargetFramebuffer() const
	{
		return m_Specification.TargetFramebuffer;
	}

	void RenderPass::SetTargetFramebuffer(Ref<FrameBuffer> targetFramebuffer)
	{
		m_Specification.TargetFramebuffer = targetFramebuffer;
	}

}
