#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/Shader.h"
#include "Shark/Render/ConstantBuffer.h"
#include "Shark/Render/StorageBuffer.h"
#include "Shark/Render/Image.h"
#include "Shark/Render/Texture.h"

#include <nvrhi/nvrhi.h>

namespace Shark {

	enum class RenderInputType
	{
		None = 0,
		ConstantBuffer,
		StorageBuffer,
		Sampler,
		Viewable,
		Image2D,
		ImageView,
		Texture2D,
		TextureCube
	};

	struct InputResource
	{
		Ref<RendererResource> Item;
		RenderInputType Type;

		std::optional<nvrhi::TextureSubresourceSet> SubresourceSet;
	};

	struct BindingSetInput
	{
		ShaderInputType Type = ShaderInputType::None;
		std::vector<InputResource> Items;

		void Set(Ref<ConstantBuffer> constantBuffer, uint32_t arrayIndex)
		{
			Items[arrayIndex].Item = constantBuffer;
			Items[arrayIndex].Type = RenderInputType::ConstantBuffer;
		}

		void Set(Ref<StorageBuffer> storageBuffer, uint32_t arrayIndex)
		{
			Items[arrayIndex].Item = storageBuffer;
			Items[arrayIndex].Type = RenderInputType::StorageBuffer;
		}

		void Set(Ref<Sampler> sampler, uint32_t arrayIndex)
		{
			Items[arrayIndex].Item = sampler;
			Items[arrayIndex].Type = RenderInputType::Sampler;
		}

		void Set(Ref<ViewableResource> viewable, uint32_t arrayIndex)
		{
			Items[arrayIndex].Item = viewable;
			Items[arrayIndex].Type = RenderInputType::Viewable;
		}

		void Set(Ref<Image2D> image, uint32_t arrayIndex)
		{
			Items[arrayIndex].Item = image;
			Items[arrayIndex].Type = RenderInputType::Image2D;
		}

		void Set(Ref<ImageView> imageView, uint32_t arrayIndex)
		{
			Items[arrayIndex].Item = imageView;
			Items[arrayIndex].Type = RenderInputType::ImageView;
		}

		void Set(Ref<Texture2D> texture, uint32_t arrayIndex)
		{
			Items[arrayIndex].Item = texture;
			Items[arrayIndex].Type = RenderInputType::Texture2D;
		}

		void Set(Ref<TextureCube> textureCube, uint32_t arrayIndex)
		{
			Items[arrayIndex].Item = textureCube;
			Items[arrayIndex].Type = RenderInputType::TextureCube;
		}

		void Set(const nvrhi::TextureSubresourceSet& subresourceSet, uint32_t arrayIndex)
		{
			Items[arrayIndex].SubresourceSet = subresourceSet;
		}

		BindingSetInput() = default;
	};

	struct ShaderInputManagerSpecification
	{
		Ref<Shader> Shader;
		bool Mutable = true;

		uint32_t StartSet = 1;
		uint32_t EndSet = nvrhi::c_MaxBindingLayouts;

		std::string DebugName;
	};

	class ShaderInputManager
	{
	public:
		ShaderInputManager();
		ShaderInputManager(const ShaderInputManagerSpecification& specification);
		~ShaderInputManager();

		void Bake();
		void Update();
		bool Validate() const;

		void SetInput(const std::string& name, Ref<ConstantBuffer> constantBuffer, uint32_t arrayIndex = 0);
		void SetInput(const std::string& name, Ref<StorageBuffer> storageBuffer, uint32_t arrayIndex = 0);
		void SetInput(const std::string& name, Ref<ViewableResource> viewable, uint32_t arrayIndex = 0);
		void SetInput(const std::string& name, Ref<Image2D> image, uint32_t arrayIndex = 0);
		void SetInput(const std::string& name, Ref<Image2D> image, const nvrhi::TextureSubresourceSet& subresource, uint32_t arrayIndex = 0);
		void SetInput(const std::string& name, Ref<ImageView> imageView, uint32_t arrayIndex = 0);
		void SetInput(const std::string& name, Ref<Texture2D> texture, uint32_t arrayIndex = 0);
		void SetInput(const std::string& name, Ref<TextureCube> textureCube, uint32_t arrayIndex = 0);
		void SetInput(const std::string& name, Ref<Sampler> sampler, uint32_t arrayIndex = 0);

		void SetInputSubresourceSet(const std::string& name, const nvrhi::TextureSubresourceSet& subresourceSet, uint32_t arrayIndex = 0);

		uint32_t GetStartSet() const { return m_Specification.StartSet; }
		uint32_t GetEndSet() const { return m_Specification.EndSet; }
		nvrhi::BindingSetHandle GetHandle(uint32_t set) const { return m_BackedSets[set - m_Specification.StartSet]; }

		Ref<Shader> GetShader() const { return m_Specification.Shader; }

	private:
		const ShaderInputInfo* GetInputInfo(const std::string& name) const;
		bool IsWritable(const InputResource& input) const;

	private:
		template<typename T>
		using LayoutArray = nvrhi::static_vector<T, nvrhi::c_MaxBindingLayouts>;

	private:
		ShaderInputManagerSpecification m_Specification;

		bool m_Backed = false;
		LayoutArray<nvrhi::BindingSetHandle> m_BackedSets;

		std::bitset<nvrhi::c_MaxBindingLayouts> m_PendingSets;
		LayoutArray<std::map<GraphicsBinding, BindingSetInput>> m_InputSetItems;
		std::unordered_map<std::string, ShaderInputInfo> m_InputInfos;
	};

}
