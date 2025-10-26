#pragma once

#include "Shark/Core/Base.h"

#include "Shark/Render/Pipeline.h"
#include "Shark/Render/ConstantBuffer.h"
#include "Shark/Render/StorageBuffer.h"
#include "Shark/Render/Texture.h"
#include "Shark/Render/Image.h"
#include "Shark/Render/ShaderInputManager.h"

namespace Shark {

	struct RenderPassSpecification
	{
		Ref<Shader> Shader;
		Ref<FrameBuffer> TargetFramebuffer;
		std::string DebugName;
	};

	class RenderPass : public RefCount
	{
	public:
		static Ref<RenderPass> Create(const RenderPassSpecification& specification) { return Ref<RenderPass>::Create(specification); }

		void Bake();
		bool Validate() const;
		void Update();

		void SetInput(const std::string& name, Ref<ConstantBuffer> constantBuffer);
		void SetInput(const std::string& name, Ref<StorageBuffer> storageBuffer);
		void SetInput(const std::string& name, Ref<Image2D> image, uint32_t arrayIndex = 0);
		void SetInput(const std::string& name, Ref<Texture2D> image, uint32_t arrayIndex = 0);
		void SetInput(const std::string& name, Ref<TextureCube> textureCube, uint32_t arrayIndex = 0);
		void SetInput(const std::string& name, Ref<Sampler> sampler, uint32_t arrayIndex = 0);

		Ref<Image2D> GetOutput(uint32_t index) const;
		Ref<Image2D> GetDepthOutput() const;

		Ref<FrameBuffer> GetTargetFramebuffer() const;
		void SetTargetFramebuffer(Ref<FrameBuffer> targetFramebuffer);

		const ShaderInputManager& GetInputManager() const { return m_InputManager; }
		const RenderPassSpecification& GetSpecification() const { return m_Specification; }

	public:
		RenderPass(const RenderPassSpecification& specification);
		~RenderPass();

	private:
		RenderPassSpecification m_Specification;
		ShaderInputManager m_InputManager;
	};

}
