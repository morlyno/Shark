#pragma once

#include "Shark/Render/RendererResource.h"

#include "Shark/Render/Shader.h"
#include "Shark/Render/ConstantBuffer.h"
#include "Shark/Render/StorageBuffer.h"
#include "Shark/Render/Texture.h"

namespace Shark {

	enum class InputResourceType
	{
		None = 0,
		Image2D,
		Texture2D,
		TextureCube,
		Sampler,
		ConstantBuffer,
		StorageBuffer
	};


	struct InputResource
	{
		InputResourceType Type;
		Ref<RendererResource> Input;
	};

	struct BoundResource
	{
		uint32_t Set, Binding;

		InputResourceType Type;
		Ref<RendererResource> Input;
	};

	class ShaderInputManager
	{
	public:
		ShaderInputManager(Ref<Shader> shader);

		void Update();
		bool ValidateMaterialInputs() const;
		bool ValidateRenderPassInputs() const;

		void SetInput(const std::string& name, Ref<ConstantBuffer> constantBuffer);
		void SetInput(const std::string& name, Ref<StorageBuffer> storageBuffer);
		void SetInput(const std::string& name, Ref<Image2D> image);
		void SetInput(const std::string& name, Ref<Texture2D> texture);
		void SetInput(const std::string& name, Ref<TextureCube> textureCube);
		void SetInput(const std::string& name, Ref<SamplerWrapper> sampler);

		bool HasInput(const std::string& name) const;

		template<typename TResource>
		Ref<TResource> GetResource(const std::string& name) const
		{
			Ref<TResource> resource;
			GetResource(name, resource);
			return resource;
		}

		const std::vector<BoundResource>& GetBoundResources() const { return m_BoundResources; }
		const std::unordered_map<std::string, InputResource>& GetInputResources() const { return m_InputResources; }

	private:
		void GetResource(const std::string& name, Ref<ConstantBuffer>& outConstantBuffer) const;
		void GetResource(const std::string& name, Ref<StorageBuffer>& outStorageBuffer) const;
		void GetResource(const std::string& name, Ref<Image2D>& outImage2D) const;
		void GetResource(const std::string& name, Ref<Texture2D>& outTexture2D) const;
		void GetResource(const std::string& name, Ref<TextureCube>& outTextureCube) const;
		void GetResource(const std::string& name, Ref<SamplerWrapper>& outSampler) const;

	private:
		Ref<Shader> m_Shader;

		std::unordered_map<std::string, InputResource> m_InputResources;
		std::vector<BoundResource> m_BoundResources;
	};

	std::string ToString(InputResourceType type);

}
