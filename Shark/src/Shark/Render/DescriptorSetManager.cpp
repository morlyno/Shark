#include "skpch.h"
#include "DescriptorSetManager.h"

#include "Shark/Utils/Utilities.h"
#include "Shark/Utils/std.h"
#include "Shark/Render/Renderer.h"
#include "Shark/Utils/String.h"

namespace Shark {

	namespace utils {

		static std::string_view GetBindingPrefix(GraphicsResourceType type)
		{
			switch (type)
			{
				case GraphicsResourceType::ConstantBuffer: return "b";
				case GraphicsResourceType::ShaderResourceView: return "t";
				case GraphicsResourceType::UnorderedAccessView: return "u";
				case GraphicsResourceType::Sampler: return "s";
			}
			return "<UNKNOWN>";
		}

		static bool IsSamplerDescriptor(const nvrhi::BindingSetItem& descriptor)
		{
			if (descriptor.type != nvrhi::ResourceType::Sampler)
				return false;

			auto sampler = dynamic_cast<nvrhi::ISampler*>(descriptor.resourceHandle);
			return sampler != nullptr;
		}

		static bool InputCompadible(const nvrhi::BindingSetItem& descriptor, const ShaderInputInfo& info)
		{
			switch (info.Type)
			{
				case ShaderInputType::None:
				{
					return descriptor.type == nvrhi::ResourceType::None;
				}
				case ShaderInputType::ConstantBuffer:
				{
					if (descriptor.type != nvrhi::ResourceType::ConstantBuffer)
						return false;

					auto buffer = dynamic_cast<nvrhi::IBuffer*>(descriptor.resourceHandle);
					return buffer && buffer->getDesc().isConstantBuffer;
				}
				case ShaderInputType::StorageBuffer:
				{
					if (descriptor.type != nvrhi::ResourceType::StructuredBuffer_SRV)
						return false;

					auto buffer = dynamic_cast<nvrhi::IBuffer*>(descriptor.resourceHandle);
					return buffer && buffer->getDesc().structStride > 0;
				}
				case ShaderInputType::Sampler:
				{
					return IsSamplerDescriptor(descriptor);
				}
				case ShaderInputType::Image:
				case ShaderInputType::Texture:
				{
					if (descriptor.type != nvrhi::ResourceType::Texture_SRV)
						return false;

					auto texture = dynamic_cast<nvrhi::ITexture*>(descriptor.resourceHandle);
					return texture && texture->getDesc().isShaderResource;
				}
				case ShaderInputType::StorageImage:
				{
					if (descriptor.type != nvrhi::ResourceType::Texture_UAV)
						return false;

					auto texture = dynamic_cast<nvrhi::ITexture*>(descriptor.resourceHandle);
					return texture && texture->getDesc().isUAV;
				}
			}
			SK_CORE_ASSERT(false, "Unkown ShaderInputType");
			return false;
		}

		static GraphicsResourceType GetRegisterTypeFromResourceType(nvrhi::ResourceType type)
		{
			switch (type)
			{
				case nvrhi::ResourceType::Texture_SRV:
				case nvrhi::ResourceType::TypedBuffer_SRV:
				case nvrhi::ResourceType::StructuredBuffer_SRV:
				case nvrhi::ResourceType::RawBuffer_SRV:
				case nvrhi::ResourceType::RayTracingAccelStruct:
					return GraphicsResourceType::ShaderResourceView;

				case nvrhi::ResourceType::Texture_UAV:
				case nvrhi::ResourceType::TypedBuffer_UAV:
				case nvrhi::ResourceType::StructuredBuffer_UAV:
				case nvrhi::ResourceType::RawBuffer_UAV:
				case nvrhi::ResourceType::SamplerFeedbackTexture_UAV:
					return GraphicsResourceType::UnorderedAccessView;

				case nvrhi::ResourceType::ConstantBuffer:
				case nvrhi::ResourceType::VolatileConstantBuffer:
				case nvrhi::ResourceType::PushConstants:
					return GraphicsResourceType::ConstantBuffer;

				case nvrhi::ResourceType::Sampler:
					return GraphicsResourceType::Sampler;
			}

			SK_CORE_ASSERT(false, "Unkown nvrhi::ResourceType");
			return GraphicsResourceType::None;
		}

	}

	DescriptorSetManager::DescriptorSetManager(Ref<Shader> shader, uint32_t set, const std::string& debugName)
		: m_Shader(shader), m_Set(set), m_DebugName(debugName)
	{
		ScopedTimer timer("DescriptorSetManager ctor");

		const auto& reflection = shader->GetReflectionData();
		SK_CORE_VERIFY(shader->HasLayout(m_Set));

		const auto& layout = reflection.BindingLayouts[set];
		auto infos = layout.InputInfos |
			std::views::values |
			std::views::transform(Projection::ToAddress) |
			std::ranges::to<std::vector>();

		std::ranges::sort(infos, std::ranges::less{}, [](const ShaderInputInfo* info)
		{
			return std::tie(info->GraphicsType, info->Slot);
		});

		uint32_t descriptorCount = 0;
		for (const auto& info : infos)
		{
			descriptorCount += info->Count;

			// A sampled image (Texture) has 2 descriptors
			if (info->Type == ShaderInputType::Texture)
				descriptorCount += info->Count;
		}

		const bool includePushConstant = set == 0 && reflection.PushConstant;
		if (includePushConstant)
			m_Descriptors.bindings.reserve(descriptorCount + 1);

		m_Descriptors.bindings.resize(descriptorCount);

		D3D11BindingSetOffsets offsets = {};
		if (Renderer::GetDeviceManager()->GetGraphicsAPI() == nvrhi::GraphicsAPI::D3D11)
			offsets = layout.BindingOffsets;

		uint32_t index = 0;
		for (const auto& info : infos)
		{
			m_InputInfos[{ info->GraphicsType, info->Slot }] = info;
			m_InputIndex[{ info->GraphicsType, info->Slot }] = index;

			switch (info->Type)
			{
				case ShaderInputType::Sampler:        m_Descriptors.bindings[index] = nvrhi::BindingSetItem::Sampler(info->Slot + offsets.Sampler, nullptr);                     break;
				case ShaderInputType::Image:          m_Descriptors.bindings[index] = nvrhi::BindingSetItem::Texture_SRV(info->Slot + offsets.ShaderResource, nullptr);          break;
				case ShaderInputType::Texture:        m_Descriptors.bindings[index] = nvrhi::BindingSetItem::Texture_SRV(info->Slot + offsets.ShaderResource, nullptr);          break;
				case ShaderInputType::StorageImage:   m_Descriptors.bindings[index] = nvrhi::BindingSetItem::Texture_UAV(info->Slot + offsets.UnorderedAccess, nullptr);         break;
				case ShaderInputType::ConstantBuffer: m_Descriptors.bindings[index] = nvrhi::BindingSetItem::ConstantBuffer(info->Slot + offsets.ConstantBuffer, nullptr);       break;
				case ShaderInputType::StorageBuffer:  m_Descriptors.bindings[index] = nvrhi::BindingSetItem::StructuredBuffer_SRV(info->Slot + offsets.ShaderResource, nullptr); break;
			}

			for (uint32_t i = 1; i < info->Count; i++)
			{
				m_Descriptors.bindings[index + i] = m_Descriptors.bindings[index];
				m_Descriptors.bindings[index + i].arrayElement = i;
			}

			index += info->Count;

			if (info->Type == ShaderInputType::Texture)
			{
				m_Descriptors.bindings[index] = nvrhi::BindingSetItem::Sampler(info->Slot + offsets.Sampler, nullptr);

				for (uint32_t i = 1; i < info->Count; i++)
				{
					m_Descriptors.bindings[index + i] = m_Descriptors.bindings[index];
					m_Descriptors.bindings[index + i].arrayElement = i;
				}

				index += info->Count;
			}

		}

		if (includePushConstant)
		{
			const auto& push = *reflection.PushConstant;
			m_Descriptors.bindings.push_back(
				nvrhi::BindingSetItem::PushConstants(
					push.Slot,
					push.StructSize
				)
			);
		}

	}

	void DescriptorSetManager::Bake()
	{
		auto device = Renderer::GetGraphicsDevice();

		nvrhi::IBindingLayout* layout = m_Shader->GetBindingLayout(m_Set);
		m_Handle = device->createBindingSet(m_Descriptors, layout);
		SK_CORE_TRACE_TAG("Renderer", "[DescriptorSetManager '{}'{}] Binding set created", m_DebugName, m_Set);

		m_Pending = false;
	}

	bool DescriptorSetManager::Validate()
	{
		const auto& layout = m_Shader->GetReflectionData().BindingLayouts[m_Set];
		const auto& descriptors = m_Descriptors.bindings;

		fmt::memory_buffer error;
		fmt::appender stream = error;
		
		for (const auto& inputInfo : m_InputInfos | std::views::values)
		{
			const uint32_t index = m_InputIndex.at({ inputInfo->GraphicsType, inputInfo->Slot });
			const auto& descriptor = descriptors[index];

			if (!descriptor.resourceHandle)
			{
				if (inputInfo->Type == ShaderInputType::Texture)
				{
					const auto& sampledImage = layout.SampledImages.at(inputInfo->Slot);
					fmt::format_to(stream, " - Descriptor for (t{} s{}) index {} is null\n", sampledImage.SeparateImage.Slot, sampledImage.SeparateSampler.Slot, descriptor.arrayElement);
					fmt::format_to(stream, "   Required is Texture '{}'\n", inputInfo->Name);
				}
				else
				{
					fmt::format_to(stream, " - Descriptor for {}{} index {} is null\n", utils::GetBindingPrefix(inputInfo->GraphicsType), inputInfo->Slot, descriptor.arrayElement);
					fmt::format_to(stream, "   Required is {} '{}'\n", inputInfo->Type, inputInfo->Name);
				}
				continue;
			}

			if (!utils::InputCompadible(descriptor, *inputInfo))
			{
				if (inputInfo->Type == ShaderInputType::Texture)
				{
					const auto& sampledImage = layout.SampledImages.at(inputInfo->Slot);
					fmt::format_to(stream, " - Incompatible type for '{}' (t{}, s{}) index {}\n", sampledImage.Name, sampledImage.SeparateImage.Slot, sampledImage.SeparateSampler.Slot, descriptor.arrayElement);
					fmt::format_to(stream, "   Required is Texture\n");
				}
				else
				{
					fmt::format_to(stream, " - Incompatible type for '{}' slot {}{} index {}\n", inputInfo->Name, utils::GetBindingPrefix(inputInfo->GraphicsType), inputInfo->Slot, descriptor.arrayElement);
					fmt::format_to(stream, "   Required is {}\n", inputInfo->Type);
				}
			}

			if (inputInfo->Type == ShaderInputType::Texture)
			{
				const auto& samplerDescriptor = descriptors[index + inputInfo->Count];

				if (!samplerDescriptor.resourceHandle)
				{
					const auto& sampledImage = layout.SampledImages.at(inputInfo->Slot);
					fmt::format_to(stream, " - Input for (t{} s{}) index {} is null\n", sampledImage.SeparateImage.Slot, sampledImage.SeparateSampler.Slot, descriptor.arrayElement);
					fmt::format_to(stream, "   Required is Texture '{}' but Sampler is null\n", inputInfo->Name);
				}

				if (!utils::IsSamplerDescriptor(samplerDescriptor))
				{
					const auto& sampledImage = layout.SampledImages.at(inputInfo->Slot);
					fmt::format_to(stream, " - Incompatible type for '{}' (t{}, s{}) index {}\n", sampledImage.Name, sampledImage.SeparateImage.Slot, sampledImage.SeparateSampler.Slot, descriptor.arrayElement);
					fmt::format_to(stream, "   Required is Texture but Sampler is Invalid");
				}
			}
		}

		if (error.size())
		{
			std::string msg = fmt::to_string(error);
			String::StripBack(msg, "\n");
			SK_CORE_ERROR_TAG("Renderer", "[DescriptorSetManager '{}'] Invalid descriptors for shader '{}' in set {}\n{}", m_DebugName, m_Shader->GetName(), m_Set, msg);
			return false;
		}

		return true;
	}

	bool DescriptorSetManager::Update(bool force)
	{
		if (!force && !m_Pending)
			return false;

		Bake();
		return true;
	}

	InputKey DescriptorSetManager::GetInputKey(std::string_view name, uint32_t arrayIndex)
	{
		auto info = GetInputInfo(name);
		if (!info)
		{
			SK_CORE_WARN_TAG("Renderer", "[DescriptorSetManager '{}'] Input '{}' not found", m_DebugName, name);
			return {
				~0u,
				GraphicsResourceType::None,
				arrayIndex
			};
		}

		return {
			.Slot = info->Slot,
			.Type = info->GraphicsType,
			.ArrayIndex = arrayIndex
		};
	}

	nvrhi::BindingSetItem* DescriptorSetManager::GetDescriptor(std::string_view name)
	{
		const auto* info = GetInputInfo(name);
		if (!info)
			return nullptr;

		return &m_Descriptors.bindings[GetIndexFor(info)];
	}

	nvrhi::BindingSetItem* DescriptorSetManager::GetDescriptor(std::string_view name, uint32_t arrayIndex)
	{
		const auto* info = GetInputInfo(name);
		if (!info)
			return nullptr;

		return &m_Descriptors.bindings[GetIndexFor(info, arrayIndex)];
	}

	nvrhi::BindingSetItem* DescriptorSetManager::GetDescriptor(uint32_t slot, GraphicsResourceType registerType, uint32_t arrayIndex)
	{
		const auto* info = GetInputInfo(slot, registerType);
		if (!info)
			return nullptr;

		return &m_Descriptors.bindings[GetIndexFor(info, arrayIndex)];
	}

	nvrhi::BindingSetItem* DescriptorSetManager::GetSamplerDescriptor(uint32_t slot, GraphicsResourceType registerType, uint32_t arrayIndex)
	{
		const auto* info = GetInputInfo(slot, registerType);
		if (!info)
			return nullptr;

		return &m_Descriptors.bindings[GetIndexFor(info, arrayIndex, true)];
	}

	void DescriptorSetManager::SetDescriptor(std::string_view name, const nvrhi::BindingSetItem& descriptor)
	{
		auto index = GetInputIndex(name);
		if (!index)
			return;

		auto& currentDescriptor = m_Descriptors.bindings[*index];
		if (currentDescriptor != descriptor)
		{
			currentDescriptor = descriptor;
			m_Pending = true;
		}
	}

	void DescriptorSetManager::SetDescriptor(std::string_view name, const nvrhi::TextureSubresourceSet& subresource)
	{
		auto index = GetInputIndex(name);
		if (!index)
			return;

		auto& descriptor = m_Descriptors.bindings[*index];
		if (descriptor.subresources != subresource)
		{
			descriptor.subresources = subresource;
			m_Pending = true;
		}
	}

	void DescriptorSetManager::SetDescriptor(std::string_view name, const nvrhi::TextureSubresourceSet& subresource, nvrhi::Format format, nvrhi::TextureDimension dimension)
	{
		auto index = GetInputIndex(name);
		if (!index)
			return;

		auto& descriptor = m_Descriptors.bindings[*index];
		if (descriptor.format != format || descriptor.dimension != dimension || descriptor.subresources != subresource)
		{
			descriptor.format       = format;
			descriptor.dimension    = dimension;
			descriptor.subresources = subresource;
			m_Pending = true;
		}
	}

	void DescriptorSetManager::SetDescriptor(const InputKey& key, const InputViewArgs& viewArgs)
	{
		auto info = GetInputInfo(key.Slot, key.Type);
		if (!info)
		{
			SK_CORE_WARN_TAG("Renderer", "[DescriptorSetManager '{}'] Input '{}{}' not found", m_DebugName, utils::GetBindingPrefix(key.Type), key.Slot);
			return;
		}

		const uint32_t index = GetIndexFor(info, key.ArrayIndex);
		auto& descriptor = m_Descriptors.bindings[index];

		if (viewArgs.Format && *viewArgs.Format != descriptor.format)
		{
			descriptor.format = *viewArgs.Format;
			m_Pending = true;
		}

		if (viewArgs.Dimension && *viewArgs.Dimension != descriptor.dimension)
		{
			descriptor.dimension = *viewArgs.Dimension;
			m_Pending = true;
		}

		if (viewArgs.SubresourceSet && *viewArgs.SubresourceSet != descriptor.subresources)
		{
			descriptor.subresources = *viewArgs.SubresourceSet;
			m_Pending = true;
		}
	}

	void DescriptorSetManager::SetDescriptor(const InputKey& key, const nvrhi::TextureSubresourceSet& subresource, nvrhi::Format format, nvrhi::TextureDimension dimension)
	{
		auto info = GetInputInfo(key.Slot, key.Type);
		if (!info)
		{
			SK_CORE_WARN_TAG("Renderer", "[DescriptorSetManager '{}'] Input '{}{}' not found", m_DebugName, utils::GetBindingPrefix(key.Type), key.Slot);
			return;
		}

		const uint32_t index = GetIndexFor(info, key.ArrayIndex);
		auto& descriptor = m_Descriptors.bindings[index];
		if (descriptor.format != format || descriptor.dimension != dimension || descriptor.subresources != subresource)
		{
			descriptor.format = format;
			descriptor.dimension = dimension;
			descriptor.subresources = subresource;
			m_Pending = true;
		}
	}

	void DescriptorSetManager::SetInput(std::string_view name, nvrhi::TextureHandle texture, uint32_t arrayIndex)
	{
		const auto* info = GetInputInfo(name);
		if (!info)
		{
			SK_CORE_WARN_TAG("Renderer", "[DescriptorSetManager '{}'] Input '{}' not found", m_DebugName, name);
			return;
		}

		const uint32_t index = GetIndexFor(info, arrayIndex);
		m_Pending |= m_Descriptors.bindings[index].resourceHandle != texture;
		m_Descriptors.bindings[index].resourceHandle = texture;
	}

	void DescriptorSetManager::SetInput(std::string_view name, nvrhi::BufferHandle buffer, uint32_t arrayIndex)
	{
		const auto* info = GetInputInfo(name);
		if (!info)
		{
			SK_CORE_WARN_TAG("Renderer", "[DescriptorSetManager '{}'] Input '{}' not found", m_DebugName, name);
			return;
		}

		const uint32_t index = GetIndexFor(info, arrayIndex);
		m_Pending |= m_Descriptors.bindings[index].resourceHandle != buffer;
		m_Descriptors.bindings[index].resourceHandle = buffer;
	}

	void DescriptorSetManager::SetInput(std::string_view name, nvrhi::SamplerHandle sampler, uint32_t arrayIndex)
	{
		const auto* info = GetInputInfo(name);
		if (!info)
		{
			SK_CORE_WARN_TAG("Renderer", "[DescriptorSetManager '{}'] Input '{}' not found", m_DebugName, name);
			return;
		}

		const uint32_t index = GetIndexFor(info, arrayIndex, info->Type == ShaderInputType::Texture);

		m_Pending |= m_Descriptors.bindings[index].resourceHandle != sampler;
		m_Descriptors.bindings[index].resourceHandle = sampler;
	}

	void DescriptorSetManager::SetInput(std::string_view name, nvrhi::SamplerHandle sampler, uint32_t arrayIndex, bool textureSampler)
	{
		const auto* info = GetInputInfo(name);
		if (!info)
		{
			SK_CORE_WARN_TAG("Renderer", "[DescriptorSetManager '{}'] Input '{}' not found", m_DebugName, name);
			return;
		}


		const uint32_t index = GetIndexFor(info, arrayIndex, textureSampler);
		m_Pending |= m_Descriptors.bindings[index].resourceHandle != sampler;
		m_Descriptors.bindings[index].resourceHandle = sampler;
	}

	void DescriptorSetManager::SetInput(const InputKey& key, nvrhi::ResourceHandle resource, bool isTextureSampler)
	{
		const auto* info = GetInputInfo(key.Slot, key.Type);
		if (!info)
		{
			SK_CORE_WARN_TAG("Renderer", "[DescriptorSetManager '{}'] Input '{}{}' not found", m_DebugName, utils::GetBindingPrefix(key.Type), key.Slot);
			return;
		}

		const uint32_t index = GetIndexFor(info, key.ArrayIndex, isTextureSampler);
		m_Pending |= m_Descriptors.bindings[index].resourceHandle != resource;
		m_Descriptors.bindings[index].resourceHandle = resource;
	}

	const ShaderInputInfo* DescriptorSetManager::GetInputInfo(std::string_view name) const
	{
		const auto& layout = GetReflectionLayout();
		const auto i = layout.InputInfos.find(std::string(name));
		if (i != layout.InputInfos.end())
			return &i->second;
		return nullptr;
	}

	const ShaderInputInfo* DescriptorSetManager::GetInputInfo(uint32_t slot, GraphicsResourceType graphicsType) const
	{
		if (m_InputInfos.contains({ graphicsType, slot }))
			return m_InputInfos.at({ graphicsType, slot });
		return nullptr;
	}

	std::optional<uint32_t> DescriptorSetManager::GetInputIndex(std::string_view name) const
	{
		const auto* info = GetInputInfo(name);
		if (info)
			return GetIndexFor(info);
		return std::nullopt;
	}

	std::pair<GraphicsResourceType, uint32_t> DescriptorSetManager::GetBindingFromIndex(uint32_t index) const
	{
		if (Renderer::GetDeviceManager()->GetGraphicsAPI() == nvrhi::GraphicsAPI::D3D11)
		{
			// #renderer-d3d11 
			const auto& offsets = m_Shader->GetReflectionData().BindingLayouts[m_Set].BindingOffsets;
			const auto& descriptor = m_Descriptors.bindings[index];
			auto type = utils::GetRegisterTypeFromResourceType(descriptor.type);

			uint32_t offset = 0;
			switch (type)
			{
				case GraphicsResourceType::ShaderResourceView: offset = offsets.ShaderResource; break;
				case GraphicsResourceType::Sampler: offset = offsets.Sampler; break;
				case GraphicsResourceType::UnorderedAccessView: offset = offsets.UnorderedAccess; break;
				case GraphicsResourceType::ConstantBuffer: offset = offsets.ConstantBuffer; break;
			}

			return { type, descriptor.slot - offset };
		}

		const auto& descriptor = m_Descriptors.bindings[index];
		return { utils::GetRegisterTypeFromResourceType(descriptor.type), descriptor.slot };
	}

	uint32_t DescriptorSetManager::GetIndexFor(const ShaderInputInfo* info) const
	{
		return m_InputIndex.at({ info->GraphicsType, info->Slot });
	}

	uint32_t DescriptorSetManager::GetIndexFor(const ShaderInputInfo* info, uint32_t arrayIndex, bool isTextureSampler) const
	{
		const uint32_t baseIndex = GetIndexFor(info);
		if (isTextureSampler)
			return baseIndex + info->Count + arrayIndex;
		return baseIndex + arrayIndex;
	}

}
