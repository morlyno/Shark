#include "skpch.h"
#include "ShaderInputManager.h"

#include "Shark/Render/Renderer.h"
#include "Shark/Utils/String.h"

namespace Shark {

#if 1
	#define SK_LOG_INPUT(_item, _name, _arrayIndex) SK_CORE_TRACE_TAG("Renderer", "[ShaderInputManager '{}'] Input set '{}':{} => {}", m_Specification.DebugName, _name, _arrayIndex, fmt::ptr(_item.Raw()));
#else
	#define SK_LOG_INPUT(_item, _name, _arrayIndex) (void)0
#endif

	namespace utils {

		static bool IsImageType(RenderInputType type)
		{
			switch (type)
			{
				case RenderInputType::ConstantBuffer:
				case RenderInputType::StorageBuffer:
				case RenderInputType::Sampler:
					return false;

				case RenderInputType::Image2D:
				case RenderInputType::Texture2D:
				case RenderInputType::TextureCube:
					return true;
			}
			SK_CORE_ASSERT(false, "Unknown RenderInputType");
			return false;
		}

	}

	ShaderInputManager::ShaderInputManager()
	{
		m_Specification.StartSet = 1;
		m_Specification.EndSet = 0;
	}

	ShaderInputManager::ShaderInputManager(const ShaderInputManagerSpecification& specification)
		: m_Specification(specification)
	{
		if (m_Specification.DebugName.empty())
			m_Specification.DebugName = "<UNNAMED>";

		const auto& reflection = m_Specification.Shader->GetReflectionData();
		if (reflection.BindingLayouts.empty())
		{
			m_Specification.StartSet = 1;
			m_Specification.EndSet = 0;
			return;
		}

		m_Specification.EndSet = std::min(m_Specification.EndSet, (uint32_t)reflection.BindingLayouts.size() - 1);
		m_InputSetItems.resize((size_t)(m_Specification.EndSet - m_Specification.StartSet + 1));
		m_BackedSets.resize(reflection.BindingLayouts.size());

		const auto& preBackedSets = m_Specification.Shader->GetRequestedBindingSets();

		for (uint32_t set = m_Specification.StartSet; set <= m_Specification.EndSet; set++)
		{
			if (preBackedSets.contains(set))
			{
				nvrhi::BindingSetHandle bindingSet = preBackedSets.at(set);
				m_BackedSets[set - m_Specification.StartSet] = bindingSet;
				m_PendingSets.set(set, false);
				continue;
			}

			const auto& layout = reflection.BindingLayouts[set];
			for (const auto& [name, inputInfo] : layout.InputInfos)
			{
				m_InputInfos[name] = inputInfo;

				BindingSetInput& input = m_InputSetItems[inputInfo.Set - m_Specification.StartSet][inputInfo.GetGraphicsBinding()];
				input.Type = inputInfo.Type;
				input.Items.resize(inputInfo.Count);
			}
		}

		m_PendingSets.set(0, reflection.PushConstant.has_value());
	}

	ShaderInputManager::~ShaderInputManager()
	{
	}

	void ShaderInputManager::Bake()
	{
		if (m_Backed && !m_Specification.Mutable)
		{
			SK_CORE_ERROR_TAG("Renderer", "[ShaderInputManager '{}'] Calling Bake multiple times without setting Mutable = true is not allowed.", m_Specification.DebugName);
			return;
		}

		auto deviceManager = Renderer::GetDeviceManager();
		auto device = Renderer::GetGraphicsDevice();

		static constexpr D3D11BindingSetOffsets s_NullOffsets = {};
		const D3D11BindingSetOffsets* bindingOffsets = &s_NullOffsets;

		// #Renderer #TODO This should happen on the render thread
		for (uint32_t inputSetIndex = 0; inputSetIndex < m_InputSetItems.size(); inputSetIndex++)
		{
			const uint32_t set = inputSetIndex + m_Specification.StartSet;
			if (!m_PendingSets.test(set) || !m_Specification.Shader->HasLayout(set))
			{
				m_PendingSets.reset(set);
				continue;
			}

			const auto& inputSet = m_InputSetItems[inputSetIndex];

			if (deviceManager->GetGraphicsAPI() == nvrhi::GraphicsAPI::D3D11)
			{
				const auto& layouts = m_Specification.Shader->GetReflectionData().BindingLayouts;
				bindingOffsets = &layouts[set].BindingOffsets;
			}

			nvrhi::BindingSetDesc bindingSetDesc;

			for (const auto& [binding, input] : inputSet)
			{
				const uint32_t slot = binding.Slot;
				switch (input.Type)
				{
					case ShaderInputType::ConstantBuffer:
					{
						auto constantBuffer = input.Items[0].Item.As<ConstantBuffer>();
						bindingSetDesc.addItem(nvrhi::BindingSetItem::ConstantBuffer(slot + bindingOffsets->ConstantBuffer, constantBuffer->GetHandle()));
						break;
					}
					case ShaderInputType::StorageBuffer:
					{
						auto storageBuffer = input.Items[0].Item.As<StorageBuffer>();
						bindingSetDesc.addItem(nvrhi::BindingSetItem::StructuredBuffer_SRV(slot + bindingOffsets->ShaderResource, storageBuffer->GetHandle()));
						break;
					}
					case ShaderInputType::Sampler:
					{
						for (uint32_t i = 0; i < input.Items.size(); i++)
						{
							auto sampler = input.Items[i].Item.As<Sampler>();
							bindingSetDesc.addItem(
								nvrhi::BindingSetItem::Sampler(slot + bindingOffsets->Sampler, sampler->GetHandle())
									.setArrayElement(i)
							);
						}
						break;
					}
					case ShaderInputType::Image:
					{
						for (uint32_t i = 0; i < input.Items.size(); i++)
						{
							const auto& item = input.Items[i];
							Ref<ViewableResource> viewable = item.Item.As<ViewableResource>();
							const auto& viewInfo = viewable->GetViewInfo();

							bindingSetDesc.addItem(
								nvrhi::BindingSetItem::Texture_SRV(slot + bindingOffsets->ShaderResource, viewInfo.Handle, viewInfo.Format, item.SubresourceSet.value_or(viewInfo.SubresourceSet), viewInfo.Dimension)
									.setArrayElement(i)
							);
						}
						break;
					}
					case ShaderInputType::Texture:
					{
						for (uint32_t i = 0; i < input.Items.size(); i++)
						{
							const auto& item = input.Items[i];
							Ref<ViewableResource> viewable = input.Items[i].Item.As<ViewableResource>();
							const auto& viewInfo = viewable->GetViewInfo();
							SK_CORE_ASSERT(viewInfo.TextureSampler, "Sampler is null indicating that the provided input is not a combined texture but the Shader expects one.");

							const auto& reflection = m_Specification.Shader->GetReflectionData();
							const auto& sampledImage = reflection.BindingLayouts[set].SampledImages.at(slot);

							bindingSetDesc.addItem(
								nvrhi::BindingSetItem::Texture_SRV(sampledImage.SeparateImage.Slot + bindingOffsets->ShaderResource, viewInfo.Handle, viewInfo.Format, item.SubresourceSet.value_or(viewInfo.SubresourceSet), viewInfo.Dimension)
									.setArrayElement(i)
							);

							bindingSetDesc.addItem(
								nvrhi::BindingSetItem::Sampler(sampledImage.SeparateSampler.Slot + bindingOffsets->Sampler, viewInfo.TextureSampler)
									.setArrayElement(i)
							);
						}
						break;
					}
					case ShaderInputType::StorageImage:
					{
						for (uint32_t i = 0; i < input.Items.size(); i++)
						{
							const auto& item = input.Items[i];
							Ref<ViewableResource> viewable = input.Items[i].Item.As<ViewableResource>();
							const auto& viewInfo = viewable->GetViewInfo();

							SK_CORE_ASSERT(viewInfo.Handle->getDesc().isUAV, "Input provided is not writable");
							bindingSetDesc.addItem(
								nvrhi::BindingSetItem::Texture_UAV(slot + bindingOffsets->UnorderedAccess, viewInfo.Handle, viewInfo.Format, item.SubresourceSet.value_or(viewInfo.SubresourceSet), viewInfo.Dimension)
									.setArrayElement(i)
							);
						}
						break;
					}
				}
			}

			if (set == 0 && m_Specification.Shader->GetReflectionData().PushConstant)
			{
				const auto& pushConstant = *m_Specification.Shader->GetReflectionData().PushConstant;
				bindingSetDesc.addItem(nvrhi::BindingSetItem::PushConstants(pushConstant.Slot, pushConstant.StructSize));
			}

			if (m_BackedSets[inputSetIndex] && *m_BackedSets[inputSetIndex]->getDesc() == bindingSetDesc)
			{
				SK_CORE_TRACE_TAG("Renderer", "[ShaderInputManager '{}'] Creation of Binding set {} skipped", m_Specification.DebugName, set);
				m_PendingSets.reset(set);
				continue;
			}

			nvrhi::IBindingLayout* bindingLayout = m_Specification.Shader->GetBindingLayout(set);
			m_BackedSets[inputSetIndex] = device->createBindingSet(bindingSetDesc, bindingLayout);
			m_PendingSets.reset(set);
			SK_CORE_INFO_TAG("Renderer", "[ShaderInputManager '{}'] Binding set {} created", m_Specification.DebugName, set);
		}

		m_Backed = true;
	}

	void ShaderInputManager::Update()
	{
		// #TODO #Renderer better update method

		const auto& preBackedSets = m_Specification.Shader->GetRequestedBindingSets();
		for (uint32_t set = m_Specification.StartSet; set <= m_Specification.EndSet; set++)
		{
			if (preBackedSets.contains(set) || !m_Specification.Shader->HasLayout(set))
				continue;

			m_PendingSets.set(set);
		}

		Bake();
	}

	namespace utils {

		static bool InputCompadible(RenderInputType providedType, ShaderInputType requiredType)
		{
			switch (requiredType)
			{
				case ShaderInputType::None: return providedType == RenderInputType::None;
				case ShaderInputType::ConstantBuffer: return providedType == RenderInputType::ConstantBuffer;
				case ShaderInputType::StorageBuffer: return providedType == RenderInputType::StorageBuffer;
				case ShaderInputType::Sampler: return providedType == RenderInputType::Sampler;
				case ShaderInputType::Texture: return providedType == RenderInputType::Texture2D || providedType == RenderInputType::TextureCube || providedType == RenderInputType::Viewable;

				case ShaderInputType::Image:
				case ShaderInputType::StorageImage:
					return providedType == RenderInputType::Image2D || providedType == RenderInputType::ImageView ||
						   providedType == RenderInputType::Texture2D || providedType == RenderInputType::TextureCube ||
						   providedType == RenderInputType::Viewable;
			}
			SK_CORE_ASSERT(false, "Unknown ShaderInputType");
			return false;
		}

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

	}

	bool ShaderInputManager::Validate() const
	{
		const auto& reflection = m_Specification.Shader->GetReflectionData();

		std::map<uint32_t, fmt::memory_buffer> setErrors;

		for (const auto& [name, inputInfo] : m_InputInfos)
		{
			auto stream = fmt::appender(setErrors[inputInfo.Set]);

			const auto& inputSet = m_InputSetItems[inputInfo.Set - m_Specification.StartSet];
			const auto& input = inputSet.at(inputInfo.GetGraphicsBinding());

			for (uint32_t i = 0; i < inputInfo.Count; i++)
			{
				if (!input.Items[i].Item)
				{
					if (inputInfo.Type == ShaderInputType::Texture)
					{
						const auto& sampledImage = reflection.BindingLayouts[inputInfo.Set].SampledImages.at(inputInfo.Slot);
						fmt::format_to(stream, " - Input for (t{} s{}) index {} is null\n", sampledImage.SeparateImage.Slot, sampledImage.SeparateSampler.Slot, i);
						fmt::format_to(stream, "   Required is Texture '{}'\n", inputInfo.Name);
					}
					else
					{
						fmt::format_to(stream, " - Input for {}{} index {} is null\n", utils::GetBindingPrefix(inputInfo.GraphicsType), inputInfo.Slot, i);
						fmt::format_to(stream, "   Required is {} '{}'\n", inputInfo.Type, inputInfo.Name);
					}
					continue;
				}

				if (!utils::InputCompadible(input.Items[i].Type, input.Type))
				{
					if (inputInfo.Type == ShaderInputType::ConstantBuffer || inputInfo.Type == ShaderInputType::StorageBuffer)
					{
						fmt::format_to(stream, " - Incompatible type for '{}' slot {}{}\n", inputInfo.Name, utils::GetBindingPrefix(inputInfo.GraphicsType), inputInfo.Slot);
						fmt::format_to(stream, "   Required is {} but {} is provided\n", inputInfo.Type, input.Items[i].Type);
					}
					else if (inputInfo.Type == ShaderInputType::Texture)
					{
						const auto& sampledImage = reflection.BindingLayouts[inputInfo.Set].SampledImages.at(inputInfo.Slot);
						fmt::format_to(stream, " - Incompatible type for '{}' (t{}, s{}) index {}\n", sampledImage.Name, sampledImage.SeparateImage.Slot, sampledImage.SeparateSampler.Slot, i);
						fmt::format_to(stream, "   Required is Texture but {} is provided\n", input.Items[i].Type);
					}
					else
					{
						fmt::format_to(stream, " - Incompatible type for '{}' slot {}{} index {}\n", inputInfo.Name, utils::GetBindingPrefix(inputInfo.GraphicsType), inputInfo.Slot, i);
						fmt::format_to(stream, "   Required is {} but {} is provided\n", inputInfo.Type, input.Items[i].Type);
					}
				}
				else if (input.Items[i].Type == RenderInputType::Viewable && input.Type == ShaderInputType::Texture)
				{
					auto viewable = input.Items[i].Item.As<ViewableResource>();
					if (!viewable->HasSampler())
					{
						const auto& sampledImage = reflection.BindingLayouts[inputInfo.Set].SampledImages.at(inputInfo.Slot);
						fmt::format_to(stream, " - Incompatible tye for '{}' (t{}, s{}) index {}\n", sampledImage.Name, sampledImage.SeparateImage.Slot, sampledImage.SeparateSampler.Slot, i);
						fmt::format_to(stream, "   Required is Texture but Viewable without Sampler is provided\n");
					}
				}

				if (inputInfo.Type == ShaderInputType::StorageImage && !IsWritable(input.Items[i]))
				{
					fmt::format_to(stream, " - Input '{}' for u{} index {} is not writable\n", inputInfo.Name, inputInfo.Slot, i);
				}

			}

		}

		std::string errorMessage;
		for (auto& [set, message] : setErrors)
		{
			if (message.size())
			{
				errorMessage += fmt::format("Invalid inputs in set {}:\n{}", set, fmt::to_string(message));
			}
		}

		if (!errorMessage.empty())
		{
			std::string_view finalMsg = errorMessage;
			String::StripBack(finalMsg, "\n");
			SK_CORE_ERROR_TAG("Renderer", "[ShaderInputManager '{}'] Invalid inputs for shader '{}'\n{}", m_Specification.DebugName, m_Specification.Shader->GetName(), finalMsg);
		}

		return errorMessage.empty();
	}

	void ShaderInputManager::SetInput(const std::string& name, Ref<ConstantBuffer> constantBuffer, uint32_t arrayIndex)
	{
		const ShaderInputInfo* inputInfo = GetInputInfo(name);
		if (inputInfo)
		{
			BindingSetInput& input = m_InputSetItems[inputInfo->Set - m_Specification.StartSet].at(inputInfo->GetGraphicsBinding());
			input.Set(constantBuffer, arrayIndex);
			m_PendingSets.set(inputInfo->Set);
			//SK_CORE_TRACE_TAG("Renderer", "[ShaderInputManager '{}'] Input set '{}':{}", m_Specification.DebugName, name, arrayIndex);
			SK_LOG_INPUT(constantBuffer, name, arrayIndex);
		}
		else
		{
			SK_CORE_WARN_TAG("Renderer", "[ShaderInputManager '{}'] Input '{}' not found", m_Specification.DebugName, name);
		}
	}

	void ShaderInputManager::SetInput(const std::string& name, Ref<StorageBuffer> storageBuffer, uint32_t arrayIndex)
	{
		const ShaderInputInfo* inputInfo = GetInputInfo(name);
		if (inputInfo)
		{
			BindingSetInput& input = m_InputSetItems[inputInfo->Set - m_Specification.StartSet].at(inputInfo->GetGraphicsBinding());
			input.Set(storageBuffer, arrayIndex);
			m_PendingSets.set(inputInfo->Set);
			//SK_CORE_TRACE_TAG("Renderer", "[ShaderInputManager '{}'] Input set '{}':{}", m_Specification.DebugName, name, arrayIndex);
			SK_LOG_INPUT(storageBuffer, name, arrayIndex);
		}
		else
		{
			SK_CORE_WARN_TAG("Renderer", "[ShaderInputManager '{}'] Input '{}' not found", m_Specification.DebugName, name);
		}
	}

	void ShaderInputManager::SetInput(const std::string& name, Ref<ViewableResource> viewable, uint32_t arrayIndex)
	{
		const ShaderInputInfo* inputInfo = GetInputInfo(name);
		if (inputInfo)
		{
			BindingSetInput& input = m_InputSetItems[inputInfo->Set - m_Specification.StartSet].at(inputInfo->GetGraphicsBinding());
			input.Set(viewable, arrayIndex);
			m_PendingSets.set(inputInfo->Set);
			//SK_CORE_TRACE_TAG("Renderer", "[ShaderInputManager '{}'] Input set '{}':{}", m_Specification.DebugName, name, arrayIndex);
			SK_LOG_INPUT(viewable, name, arrayIndex);
		}
		else
		{
			SK_CORE_WARN_TAG("Renderer", "[ShaderInputManager '{}'] Input '{}' not found", m_Specification.DebugName, name);
		}
	}

	void ShaderInputManager::SetInput(const std::string& name, Ref<Image2D> image, uint32_t arrayIndex)
	{
		const ShaderInputInfo* inputInfo = GetInputInfo(name);
		if (inputInfo)
		{
			BindingSetInput& input = m_InputSetItems[inputInfo->Set - m_Specification.StartSet].at(inputInfo->GetGraphicsBinding());
			input.Set(image, arrayIndex);
			m_PendingSets.set(inputInfo->Set);
			SK_LOG_INPUT(image, name, arrayIndex);
		}
		else
		{
			SK_CORE_WARN_TAG("Renderer", "[ShaderInputManager '{}'] Input '{}' not found", m_Specification.DebugName, name);
		}
	}

	void ShaderInputManager::SetInput(const std::string& name, Ref<Image2D> image, const nvrhi::TextureSubresourceSet& subresource, uint32_t arrayIndex)
	{
		const ShaderInputInfo* inputInfo = GetInputInfo(name);
		if (inputInfo)
		{
			BindingSetInput& input = m_InputSetItems[inputInfo->Set - m_Specification.StartSet].at(inputInfo->GetGraphicsBinding());
			input.Set(image, arrayIndex);
			input.Set(subresource, arrayIndex);
			m_PendingSets.set(inputInfo->Set);
			SK_LOG_INPUT(image, name, arrayIndex);
		}
		else
		{
			SK_CORE_WARN_TAG("Renderer", "[ShaderInputManager '{}'] Input '{}' not found", m_Specification.DebugName, name);
		}
	}

	void ShaderInputManager::SetInput(const std::string& name, Ref<ImageView> imageView, uint32_t arrayIndex)
	{
		const ShaderInputInfo* inputInfo = GetInputInfo(name);
		if (inputInfo)
		{
			BindingSetInput& input = m_InputSetItems[inputInfo->Set - m_Specification.StartSet].at(inputInfo->GetGraphicsBinding());
			input.Set(imageView, arrayIndex);
			m_PendingSets.set(inputInfo->Set);
			//SK_CORE_TRACE_TAG("Renderer", "[ShaderInputManager '{}'] Input set '{}':{}", m_Specification.DebugName, name, arrayIndex);
			SK_LOG_INPUT(imageView, name, arrayIndex);
		}
		else
		{
			SK_CORE_WARN_TAG("Renderer", "[ShaderInputManager '{}'] Input '{}' not found", m_Specification.DebugName, name);
		}
	}

	void ShaderInputManager::SetInput(const std::string& name, Ref<Texture2D> texture, uint32_t arrayIndex)
	{
		const ShaderInputInfo* inputInfo = GetInputInfo(name);
		if (inputInfo)
		{
			BindingSetInput& input = m_InputSetItems[inputInfo->Set - m_Specification.StartSet].at(inputInfo->GetGraphicsBinding());
			input.Set(texture, arrayIndex);
			m_PendingSets.set(inputInfo->Set);
			//SK_CORE_TRACE_TAG("Renderer", "[ShaderInputManager '{}'] Input set '{}':{}", m_Specification.DebugName, name, arrayIndex);
			SK_LOG_INPUT(texture, name, arrayIndex);
		}
		else
		{
			SK_CORE_WARN_TAG("Renderer", "[ShaderInputManager '{}'] Input '{}' not found", m_Specification.DebugName, name);
		}
	}

	void ShaderInputManager::SetInput(const std::string& name, Ref<TextureCube> textureCube, uint32_t arrayIndex)
	{
		const ShaderInputInfo* inputInfo = GetInputInfo(name);
		if (inputInfo)
		{
			BindingSetInput& input = m_InputSetItems[inputInfo->Set - m_Specification.StartSet].at(inputInfo->GetGraphicsBinding());
			input.Set(textureCube, arrayIndex);
			m_PendingSets.set(inputInfo->Set);
			//SK_CORE_TRACE_TAG("Renderer", "[ShaderInputManager '{}'] Input set '{}':{}", m_Specification.DebugName, name, arrayIndex);
			SK_LOG_INPUT(textureCube, name, arrayIndex);
		}
		else
		{
			SK_CORE_WARN_TAG("Renderer", "[ShaderInputManager '{}'] Input '{}' not found", m_Specification.DebugName, name);
		}
	}

	void ShaderInputManager::SetInput(const std::string& name, Ref<Sampler> sampler, uint32_t arrayIndex)
	{
		const ShaderInputInfo* inputInfo = GetInputInfo(name);
		if (inputInfo)
		{
			BindingSetInput& input = m_InputSetItems[inputInfo->Set - m_Specification.StartSet].at(inputInfo->GetGraphicsBinding());
			input.Set(sampler, arrayIndex);
			m_PendingSets.set(inputInfo->Set);
			//SK_CORE_TRACE_TAG("Renderer", "[ShaderInputManager '{}'] Input set '{}':{}", m_Specification.DebugName, name, arrayIndex);
			SK_LOG_INPUT(sampler, name, arrayIndex);
		}
		else
		{
			SK_CORE_WARN_TAG("Renderer", "[ShaderInputManager '{}'] Input '{}' not found", m_Specification.DebugName, name);
		}
	}

	void ShaderInputManager::SetInputSubresourceSet(const std::string& name, const nvrhi::TextureSubresourceSet& subresourceSet, uint32_t arrayIndex)
	{
		const ShaderInputInfo* inputInfo = GetInputInfo(name);
		if (inputInfo)
		{
			BindingSetInput& input = m_InputSetItems[inputInfo->Set - m_Specification.StartSet].at(inputInfo->GetGraphicsBinding());
			input.Set(subresourceSet, arrayIndex);
			m_PendingSets.set(inputInfo->Set);
		}
	}

	const ShaderInputInfo* ShaderInputManager::GetInputInfo(const std::string& name) const
	{
		if (m_InputInfos.contains(name))
			return &m_InputInfos.at(name);

		return nullptr;
	}

	bool ShaderInputManager::IsWritable(const InputResource& input) const
	{
		switch (input.Type)
		{
			case RenderInputType::ConstantBuffer:
			case RenderInputType::StorageBuffer:
			case RenderInputType::Sampler:
				return false;

			case RenderInputType::Image2D:
			{
				auto item = input.Item.AsSafe<Image2D>();
				return item && item->GetSpecification().Usage == ImageUsage::Storage && ImageUtils::SupportsUAV(item->GetSpecification().Format);
			}
			case RenderInputType::ImageView:
			{
				auto item = input.Item.AsSafe<ImageView>();
				return item && item->SupportsStorage();
			}
			case RenderInputType::Texture2D:
			{
				auto item = input.Item.AsSafe<Texture2D>();
				return item && item->GetSpecification().Storage && ImageUtils::SupportsUAV(item->GetSpecification().Format);
			}
			case RenderInputType::TextureCube:
			{
				auto item = input.Item.AsSafe<TextureCube>();
				return item && item->GetSpecification().Storage && ImageUtils::SupportsUAV(item->GetSpecification().Format);
			}
		}
		SK_CORE_ASSERT(false, "Unknown RenderInputType");
		return false;
	}

}
