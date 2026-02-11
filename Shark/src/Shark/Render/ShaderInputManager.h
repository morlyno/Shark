#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/Shader.h"
#include "Shark/Render/ConstantBuffer.h"
#include "Shark/Render/StorageBuffer.h"
#include "Shark/Render/Image.h"
#include "Shark/Render/Texture.h"
#include "Shark/Render/DescriptorSetManager.h"

#include <nvrhi/nvrhi.h>

namespace Shark {
	 

#if 0
	// Defined in DescriptorSetManager.h
	struct InputViewArgs
	{
		std::optional<nvrhi::Format>                Format;
		std::optional<nvrhi::TextureDimension>      Dimension;
		std::optional<nvrhi::TextureSubresourceSet> SubresourceSet;
	};
#endif

	enum class RenderInputType : uint8_t
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
		RenderInputType       Type = RenderInputType::None;
		InputViewArgs         ViewArgs;
	};

	struct BindingSetInput
	{
		ShaderInputType Type = ShaderInputType::None;
		std::vector<InputResource> Items;

		void Set(Ref<RendererResource> resource, RenderInputType type, uint32_t arrayIndex) { Items[arrayIndex].Item = resource, Items[arrayIndex].Type = type; }
		void Set(const InputViewArgs& args, uint32_t arrayIndex)                            { Items[arrayIndex].ViewArgs = args; }

		void Set(uint32_t arrayIndex, Ref<ConstantBuffer> constantBuffer, const InputViewArgs& viewArgs = {}) { Items[arrayIndex].Item = constantBuffer; Items[arrayIndex].Type = RenderInputType::ConstantBuffer; Items[arrayIndex].ViewArgs = viewArgs; }
		void Set(uint32_t arrayIndex, Ref<StorageBuffer> storageBuffer,   const InputViewArgs& viewArgs = {}) { Items[arrayIndex].Item = storageBuffer;  Items[arrayIndex].Type = RenderInputType::StorageBuffer;  Items[arrayIndex].ViewArgs = viewArgs; }
		void Set(uint32_t arrayIndex, Ref<Sampler> sampler,               const InputViewArgs& viewArgs = {}) { Items[arrayIndex].Item = sampler;        Items[arrayIndex].Type = RenderInputType::Sampler;        Items[arrayIndex].ViewArgs = viewArgs; }
		void Set(uint32_t arrayIndex, Ref<ViewableResource> viewable,     const InputViewArgs& viewArgs = {}) { Items[arrayIndex].Item = viewable;       Items[arrayIndex].Type = RenderInputType::Viewable;       Items[arrayIndex].ViewArgs = viewArgs; }
		void Set(uint32_t arrayIndex, Ref<Image2D> image,                 const InputViewArgs& viewArgs = {}) { Items[arrayIndex].Item = image;          Items[arrayIndex].Type = RenderInputType::Image2D;        Items[arrayIndex].ViewArgs = viewArgs; }
		void Set(uint32_t arrayIndex, Ref<ImageView> imageView,           const InputViewArgs& viewArgs = {}) { Items[arrayIndex].Item = imageView;      Items[arrayIndex].Type = RenderInputType::ImageView;      Items[arrayIndex].ViewArgs = viewArgs; }
		void Set(uint32_t arrayIndex, Ref<Texture2D> texture,             const InputViewArgs& viewArgs = {}) { Items[arrayIndex].Item = texture;        Items[arrayIndex].Type = RenderInputType::Texture2D;      Items[arrayIndex].ViewArgs = viewArgs; }
		void Set(uint32_t arrayIndex, Ref<TextureCube> textureCube,       const InputViewArgs& viewArgs = {}) { Items[arrayIndex].Item = textureCube;    Items[arrayIndex].Type = RenderInputType::TextureCube;    Items[arrayIndex].ViewArgs = viewArgs; }

		bool IsSame(uint32_t arrayIndex, Ref<RendererResource> resource, const InputViewArgs& viewArgs = {}) const
		{
			return
				Items[arrayIndex].Item                    == resource &&
				Items[arrayIndex].ViewArgs.Format         == viewArgs.Format &&
				Items[arrayIndex].ViewArgs.Dimension      == viewArgs.Dimension &&
				Items[arrayIndex].ViewArgs.SubresourceSet == viewArgs.SubresourceSet;
		}

		BindingSetInput() = default;
	};

	struct InputUpdate
	{
		GraphicsBinding       Binding;
		uint32_t              ArrayIndex;
		Ref<RendererResource> Input;
		RenderInputType       Type;

		InputViewArgs         ViewArgs;
	};

#if TODO
	struct InputUpdatePacked
	{
		Ref<RendererResource> Input;
		uint32_t Slot;
		uint32_t ArrayElement;
		
		uint8_t                 Set;
		RenderInputType         Type : 8;
		nvrhi::Format           Format : 8;
		nvrhi::TextureDimension Dimension : 8;

		nvrhi::TextureSubresourceSet SubresourceSet;

		bool HasFormat;
		bool HasDimension;
		bool HasSubresourceSet;
	};
#endif

	struct ShaderInputManagerSpecification
	{
		Ref<Shader> Shader;

		uint32_t StartSet = 1;
		uint32_t EndSet = nvrhi::c_MaxBindingLayouts;

		std::string DebugName;
	};

	// 
	// ActiveThread:
	//  - SetInput
	//  - Package
	//  - PrepareAll
	//  - Validate
	// 
	//  - GetUpdates
	// 
	// RenderThread:
	//  - Update (with updates from package)
	// 
	// Optional use DescriptorSetManager on the Render Thread
	// 

	class ShaderInputManager
	{
	public:
		ShaderInputManager();
		ShaderInputManager(const ShaderInputManagerSpecification& specification);
		~ShaderInputManager();

		void Initialize(const ShaderInputManagerSpecification& specification);

		bool Package(std::vector<InputUpdate>& outUpdates);
		void PrepareAll();

		void Update(std::span<const InputUpdate> updates, bool force = false);
		bool Validate() const;

		void SetInput(const std::string& name, Ref<ConstantBuffer>   constantBuffer, uint32_t arrayIndex = 0);
		void SetInput(const std::string& name, Ref<StorageBuffer>    storageBuffer,  uint32_t arrayIndex = 0);
		void SetInput(const std::string& name, Ref<ViewableResource> viewable,       uint32_t arrayIndex = 0);
		void SetInput(const std::string& name, Ref<Image2D>          image,          uint32_t arrayIndex = 0);
		void SetInput(const std::string& name, Ref<ImageView>        imageView,      uint32_t arrayIndex = 0);
		void SetInput(const std::string& name, Ref<Texture2D>        texture,        uint32_t arrayIndex = 0);
		void SetInput(const std::string& name, Ref<TextureCube>      textureCube,    uint32_t arrayIndex = 0);
		void SetInput(const std::string& name, Ref<Sampler>          sampler,        uint32_t arrayIndex = 0);

		void SetInput(const std::string& name, Ref<ViewableResource> viewable,    const InputViewArgs& viewArgs, uint32_t arrayIndex = 0);
		void SetInput(const std::string& name, Ref<Image2D>          image,       const InputViewArgs& viewArgs, uint32_t arrayIndex = 0);
		void SetInput(const std::string& name, Ref<ImageView>        imageView,   const InputViewArgs& viewArgs, uint32_t arrayIndex = 0);
		void SetInput(const std::string& name, Ref<Texture2D>        texture,     const InputViewArgs& viewArgs, uint32_t arrayIndex = 0);
		void SetInput(const std::string& name, Ref<TextureCube>      textureCube, const InputViewArgs& viewArgs, uint32_t arrayIndex = 0);

		uint32_t GetUpdateCount() const { return static_cast<uint32_t>(m_Updates.size()); }
		uint32_t GetInputCount() const { return static_cast<uint32_t>(m_InputInfos.size()); }

		uint32_t GetStartSet() const { return m_Specification.StartSet; }
		uint32_t GetEndSet() const { return m_Specification.EndSet; }
		nvrhi::BindingSetHandle GetHandle(uint32_t set) const { return m_Handles[set]; }

		Ref<Shader> GetShader() const { return m_Specification.Shader; }

	private:
		const ShaderInputInfo* GetInputInfo(const std::string& name) const;
		bool IsWritable(const InputResource& input) const;

	private:
		template<typename T>
		using LayoutArray = std::array<T, nvrhi::c_MaxBindingLayouts>;

	private:
		ShaderInputManagerSpecification m_Specification;
		bool m_EnableValidation = false;
		uint32_t m_SetCount = 0;
		uint32_t m_InputCount = 0;

		LayoutArray<Scope<DescriptorSetManager>> m_Managers;
		LayoutArray<nvrhi::BindingSetHandle> m_Handles;

		std::map<GraphicsBinding, BindingSetInput> m_Inputs;
		std::vector<InputUpdate> m_Updates;

		std::unordered_map<std::string, const ShaderInputInfo*> m_InputInfos;
	};

}
