#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Hash.h"
#include "Shark/Render/Shader.h"

#include <nvrhi/nvrhi.h>

namespace Shark {

	struct InputViewArgs
	{
		std::optional<nvrhi::Format>                Format;
		std::optional<nvrhi::TextureDimension>      Dimension;
		std::optional<nvrhi::TextureSubresourceSet> SubresourceSet;
	};

	struct DescriptorArgs
	{
		std::optional<nvrhi::TextureSubresourceSet> Subresource;
		std::optional<nvrhi::Format> Format;
	};

	struct InputKey
	{
		uint32_t Slot;
		GraphicsResourceType Type;
		uint32_t ArrayIndex = 0;

		InputKey WithIndex(uint32_t arrayIndex) const { return { Slot, Type, arrayIndex }; }
		InputKey& SetIndex(uint32_t arrayIndex) { ArrayIndex = arrayIndex; return *this; }
	};

	class DescriptorSetManager
	{
	public:
		DescriptorSetManager(Ref<Shader> shader, uint32_t set, const std::string& debugName = {});

		void Bake();
		bool Validate();
		bool Update(bool force = false);

		nvrhi::BindingSetHandle GetHandle() const { return m_Handle; }

		void SetModified() { m_Pending = true; }
		InputKey GetInputKey(std::string_view name, uint32_t arrayIndex = 0);
		nvrhi::BindingSetItem* GetDescriptor(uint32_t slot, GraphicsResourceType registertype, uint32_t arrayIndex);
		nvrhi::BindingSetItem* GetSamplerDescriptor(uint32_t slot, GraphicsResourceType registertype, uint32_t arrayIndex);

		nvrhi::BindingSetItem* GetDescriptor(std::string_view name);
		nvrhi::BindingSetItem* GetDescriptor(std::string_view name, uint32_t arrayIndex);
		void SetDescriptor(std::string_view name, const nvrhi::BindingSetItem& descriptor);
		void SetDescriptor(std::string_view name, const nvrhi::TextureSubresourceSet& subresource);
		void SetDescriptor(std::string_view name, const nvrhi::TextureSubresourceSet& subresource, nvrhi::Format format, nvrhi::TextureDimension dimension);

		void SetDescriptor(const InputKey& key, const InputViewArgs& viewArgs);
		void SetDescriptor(const InputKey& key, const nvrhi::TextureSubresourceSet& subresource, nvrhi::Format format, nvrhi::TextureDimension dimension);

		void SetInput(std::string_view name, nvrhi::BufferHandle buffer,   uint32_t arrayIndex);
		void SetInput(std::string_view name, nvrhi::TextureHandle texture, uint32_t arrayIndex);
		void SetInput(std::string_view name, nvrhi::SamplerHandle sampler, uint32_t arrayIndex);
		void SetInput(std::string_view name, nvrhi::SamplerHandle sampler, uint32_t arrayIndex, bool textureSampler);

		void SetInput(const InputKey& key, nvrhi::ResourceHandle resource, bool isTextureSampler = false);
		
	public:
		const ShaderInputInfo* GetInputInfo(std::string_view name) const;
		const ShaderInputInfo* GetInputInfo(uint32_t slot, GraphicsResourceType graphicsType) const;

	private:
		const ShaderBindingLayout& GetReflectionLayout() const { return m_Shader->GetReflectionData().BindingLayouts[m_Set]; }
		std::optional<uint32_t> GetInputIndex(std::string_view name) const;

		std::pair<GraphicsResourceType, uint32_t> GetBindingFromIndex(uint32_t index) const;
		
		uint32_t GetIndexFor(const ShaderInputInfo* info) const;
		uint32_t GetIndexFor(const ShaderInputInfo* info, uint32_t arrayIndex, bool isTextureSampler = false) const;

	private:
		uint32_t m_Set;
		Ref<Shader> m_Shader;
		std::string m_DebugName;

		bool m_Pending = false;

		nvrhi::BindingSetHandle m_Handle;
		nvrhi::BindingSetDesc m_Descriptors;

		std::map<std::pair<GraphicsResourceType, uint32_t>, const ShaderInputInfo*> m_InputInfos;
		std::map<std::pair<GraphicsResourceType, uint32_t>, uint32_t> m_InputIndex;
	};

}
