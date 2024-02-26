#include "skpch.h"
#include "DirectXRenderPass.h"

#include "Shark/Render/Renderer.h"
#include "Platform/DirectX11/DirectXShader.h"

namespace Shark {

	DirectXRenderPass::DirectXRenderPass(const RenderPassSpecification& specification)
		: m_Specification(specification), m_ShaderInputManager(specification.Pipeline->GetSpecification().Shader)
	{
		Renderer::AcknowledgeShaderDependency(m_Specification.Pipeline->GetSpecification().Shader, this);
	}

	DirectXRenderPass::~DirectXRenderPass()
	{
	}

	void DirectXRenderPass::Bake()
	{
		m_ShaderInputManager.Update();
	}

	bool DirectXRenderPass::Validate() const
	{
		return m_ShaderInputManager.ValidateRenderPassInputs();
	}

	void DirectXRenderPass::Set(const std::string& name, Ref<ConstantBuffer> constantBuffer)
	{
		m_ShaderInputManager.SetInput(name, constantBuffer);
	}

	void DirectXRenderPass::Set(const std::string& name, Ref<StorageBuffer> storageBuffer)
	{
		m_ShaderInputManager.SetInput(name, storageBuffer);
	}

	void DirectXRenderPass::Set(const std::string& name, Ref<Image2D> image)
	{
		m_ShaderInputManager.SetInput(name, image);
	}

	void DirectXRenderPass::Set(const std::string& name, Ref<Texture2D> texture)
	{
		m_ShaderInputManager.SetInput(name, texture);
	}

	void DirectXRenderPass::Set(const std::string& name, Ref<TextureCube> textureCube)
	{
		m_ShaderInputManager.SetInput(name, textureCube);
	}

	void DirectXRenderPass::Set(const std::string& name, Ref<SamplerWrapper> sampler)
	{
		m_ShaderInputManager.SetInput(name, sampler);
	}

	Ref<Image2D> DirectXRenderPass::GetOutput(uint32_t index) const
	{
		return m_Specification.Pipeline->GetSpecification().TargetFrameBuffer->GetImage(index);
	}

	Ref<Image2D> DirectXRenderPass::GetDepthOutput() const
	{
		return m_Specification.Pipeline->GetSpecification().TargetFrameBuffer->GetDepthImage();
	}

}
