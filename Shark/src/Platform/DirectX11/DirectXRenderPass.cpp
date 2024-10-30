#include "skpch.h"
#include "DirectXRenderPass.h"

#include "Shark/Render/Renderer.h"
#include "Platform/DirectX11/DirectXShader.h"

#include "Shark/Debug/Profiler.h"

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
		SK_PROFILE_FUNCTION();
		SK_PERF_SCOPED("DirectXRenderPass::Bake");
		m_ShaderInputManager.Update();
	}

	bool DirectXRenderPass::Validate() const
	{
		SK_PROFILE_FUNCTION();
		SK_PERF_SCOPED("DirectXRenderPass::Validate");
		return m_ShaderInputManager.ValidateRenderPassInputs();
	}

	void DirectXRenderPass::Update()
	{
		SK_PROFILE_FUNCTION();
		SK_PERF_SCOPED("DirectXRenderPass::Update");
		m_ShaderInputManager.Update();
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

	Ref<ConstantBuffer> DirectXRenderPass::GetConstantBuffer(const std::string& name) const
	{
		return m_ShaderInputManager.GetResource<ConstantBuffer>(name);
	}

	Ref<StorageBuffer> DirectXRenderPass::GetStorageBuffer(const std::string& name) const
	{
		return m_ShaderInputManager.GetResource<StorageBuffer>(name);
	}

	Ref<Image2D> DirectXRenderPass::GetImage2D(const std::string& name) const
	{
		return m_ShaderInputManager.GetResource<Image2D>(name);
	}

	Ref<Texture2D> DirectXRenderPass::GetTexture2D(const std::string& name) const
	{
		return m_ShaderInputManager.GetResource<Texture2D>(name);
	}

	Ref<TextureCube> DirectXRenderPass::GetTextureCube(const std::string& name) const
	{
		return m_ShaderInputManager.GetResource<TextureCube>(name);
	}

	Ref<RendererResource> DirectXRenderPass::GetInput(const std::string& name) const
	{
		return m_ShaderInputManager.GetResource(name);
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
