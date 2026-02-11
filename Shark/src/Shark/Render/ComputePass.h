#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/Shader.h"
#include "Shark/Render/ShaderInputManager.h"

namespace Shark {

	struct ComputePassSpecification
	{
		Ref<Shader> ComputeShader;
		std::string DebugName;
	};

	class ComputePass : public RefCount
	{
	public:
		static Ref<ComputePass> Create(const ComputePassSpecification& specification) { return Ref<ComputePass>::Create(specification); }
		static Ref<ComputePass> Create(Ref<Shader> computeShader, const std::string& debugName = {}) { return Ref<ComputePass>::Create(computeShader, debugName); }

	public:
		void Bake();
		void Update();
		bool Validate() const;

		void SetInput(const std::string& name, Ref<ConstantBuffer> constantBuffer);
		void SetInput(const std::string& name, Ref<StorageBuffer> storageBuffer);
		void SetInput(const std::string& name, Ref<Image2D> image, uint32_t arrayIndex = 0);
		void SetInput(const std::string& name, Ref<Image2D> image, const nvrhi::TextureSubresourceSet& subresource, uint32_t arrayIndex = 0);
		void SetInput(const std::string& name, Ref<Texture2D> image, uint32_t arrayIndex = 0);
		void SetInput(const std::string& name, Ref<TextureCube> textureCube, uint32_t arrayIndex = 0);
		void SetInput(const std::string& name, Ref<Sampler> sampler, uint32_t arrayIndex = 0);

		const ShaderInputManager& GetInputManager() const { return m_InputManager; }
		const ComputePassSpecification& GetSpecification() const { return m_Specification; }
		Ref<Shader> GetShader() const { return m_Specification.ComputeShader; }

	public:
		ComputePass(const ComputePassSpecification& specification);
		ComputePass(Ref<Shader> computeShader, const std::string& debugName);
		ComputePass(Ref<Shader> computeShader, LayoutShareMode shareMode, const std::string& debugName);
		~ComputePass();

	private:
		ComputePassSpecification m_Specification;
		ShaderInputManager m_InputManager;

	};

}
