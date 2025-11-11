#include "skpch.h"
#include "ComputePass.h"

namespace Shark {

	ComputePass::ComputePass(const ComputePassSpecification& specification)
		: m_Specification(specification)
	{
		if (m_Specification.ShareMode == LayoutShareMode::MaterialOnly)
			return;

		ShaderInputManagerSpecification inputManagerSpec;
		inputManagerSpec.Shader = m_Specification.ComputeShader;
		inputManagerSpec.DebugName = m_Specification.DebugName;
		if (m_Specification.ShareMode == LayoutShareMode::PassOnly)
			inputManagerSpec.StartSet = 0;

		m_InputManager = ShaderInputManager(inputManagerSpec);
	}

	ComputePass::ComputePass(Ref<Shader> computeShader, const std::string& debugName)
		: ComputePass({ .ComputeShader = computeShader, .DebugName = debugName })
	{
	}

	ComputePass::ComputePass(Ref<Shader> computeShader, LayoutShareMode shareMode, const std::string& debugName)
		: ComputePass({ .ComputeShader = computeShader, .ShareMode = shareMode, .DebugName = debugName })
	{
	}

	ComputePass::~ComputePass()
	{

	}

	void ComputePass::Bake()
	{
		m_InputManager.Bake();
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
