#include "skpch.h"
#include "ShaderInputManager.h"

#include "Shark/Render/Renderer.h"
#include "Shark/Utils/String.h"

namespace Shark {

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

		for (uint32_t set = m_Specification.StartSet; set <= m_Specification.EndSet; set++)
		{
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

		for (uint32_t inputSetIndex = 0; inputSetIndex < m_InputSetItems.size(); inputSetIndex++)
		{
			const uint32_t set = inputSetIndex + m_Specification.StartSet;
			if (!m_PendingSets.test(set))
				continue;

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
						auto constantBuffer = input.Items[0].As<ConstantBuffer>();
						bindingSetDesc.addItem(nvrhi::BindingSetItem::ConstantBuffer(slot + bindingOffsets->ConstantBuffer, constantBuffer->GetHandle()));
						break;
					}
					case ShaderInputType::StorageBuffer:
					{
						auto storageBuffer = input.Items[0].As<StorageBuffer>();
						bindingSetDesc.addItem(nvrhi::BindingSetItem::StructuredBuffer_SRV(slot + bindingOffsets->ShaderResource, storageBuffer->GetHandle()));
						break;
					}
					case ShaderInputType::Sampler:
					{
						for (uint32_t i = 0; i < input.Items.size(); i++)
						{
							auto sampler = input.Items[i].As<Sampler>();
							bindingSetDesc.addItem(
								nvrhi::BindingSetItem::Sampler(slot + bindingOffsets->Sampler, sampler->GetHandle())
									.setArrayElement(i)
							);
						}
						break;
					}
					case ShaderInputType::Image2D:
					{
						for (uint32_t i = 0; i < input.Items.size(); i++)
						{
							auto image = input.Items[i].As<Image2D>();
							bindingSetDesc.addItem(
								nvrhi::BindingSetItem::Texture_SRV(slot + bindingOffsets->ShaderResource, image->GetHandle())
									.setArrayElement(i)
							);
						}
						break;
					}
					case ShaderInputType::ImageCube:
					{
						for (uint32_t i = 0; i < input.Items.size(); i++)
						{
							auto textureCube = input.Items[i].As<TextureCube>();
							bindingSetDesc.addItem(
								nvrhi::BindingSetItem::Texture_SRV(slot + bindingOffsets->ShaderResource, textureCube->GetImage()->GetHandle())
									.setArrayElement(i)
							);
						}
						break;
					}
					case ShaderInputType::StorageImage2D:
					{
						for (uint32_t i = 0; i < input.Items.size(); i++)
						{
							auto image = input.Items[i].As<Image2D>();
							bindingSetDesc.addItem(
								nvrhi::BindingSetItem::Texture_UAV(slot + bindingOffsets->UnorderedAccess, image->GetHandle())
									.setArrayElement(i)
							);
						}
						break;
					}
					case ShaderInputType::StorageImageCube:
					{
						for (uint32_t i = 0; i < input.Items.size(); i++)
						{
							auto cube = input.Items[i].As<TextureCube>();
							bindingSetDesc.addItem(
								nvrhi::BindingSetItem::Texture_UAV(slot + bindingOffsets->UnorderedAccess, cube->GetImage()->GetHandle())
									.setArrayElement(i)
							);
						}
						break;
					}
					default:
						break;

				}
			}

			if (set == 0 && m_Specification.Shader->GetReflectionData().PushConstant)
			{
				const auto& pushConstant = *m_Specification.Shader->GetReflectionData().PushConstant;
				bindingSetDesc.addItem(nvrhi::BindingSetItem::PushConstants(pushConstant.Slot, pushConstant.StructSize));
			}

			if (m_BackedSets[inputSetIndex] && *m_BackedSets[inputSetIndex]->getDesc() == bindingSetDesc)
			{
				SK_CORE_INFO_TAG("Renderer", "[ShaderInputManager '{}'] Creation of Binding set {} skipped", m_Specification.DebugName, set);
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
		SK_NOT_IMPLEMENTED();
	}

	bool ShaderInputManager::Validate() const
	{
		const auto& reflection = m_Specification.Shader->GetReflectionData();

		std::string errorMessage;
		fmt::memory_buffer setError;
		std::bitset<nvrhi::c_MaxBindingLayouts> activeSets;

		LayoutArray<nvrhi::BindingSetHandle> backedSets;

		for (uint32_t currentSet = m_Specification.StartSet; currentSet <= m_Specification.EndSet; currentSet++)
		{
			const auto& layout = reflection.BindingLayouts[currentSet];
			const auto& inputSet = m_InputSetItems[currentSet - m_Specification.StartSet];

			setError.clear();
			auto stream = fmt::appender(setError);

			for (const auto& [slot, constantBuffer] : layout.ConstantBuffers)
			{
				const GraphicsBinding binding = { .Space = currentSet, .Slot = slot, .Register = GraphicsResourceType::ConstantBuffer };
				if (!inputSet.contains(binding))
				{
					fmt::format_to(stream, "Input missing for slot b{}\n", slot);
					fmt::format_to(stream, "Required is ConstantBuffer '{}'\n", constantBuffer.Name);
					continue;
				}

				const auto& input = inputSet.at(binding);
				if (input.Type != ShaderInputType::ConstantBuffer)
				{
					fmt::format_to(stream, "Incompatible type for slot b{}\n", slot);
					fmt::format_to(stream, "Required is ConstantBuffer but {} is provided\n", input.Type);
				}

				if (input.Items.empty() || !input.Items[0])
				{
					fmt::format_to(stream, "Input for b{} index {} is null\n", slot, 0);
					fmt::format_to(stream, "Required is ConstantBuffer '{}'\n", constantBuffer.Name);
				}
			}

			for (const auto& [slot, storageBuffer] : layout.StorageBuffers)
			{
				const GraphicsBinding binding = { .Space = currentSet, .Slot = slot, .Register = GraphicsResourceType::ShaderResourceView };
				if (!inputSet.contains(binding))
				{
					fmt::format_to(stream, "Input missing for slot t{}\n", slot);
					fmt::format_to(stream, "Required is StorageBuffer '{}'\n", storageBuffer.Name);
					continue;
				}

				const auto& input = inputSet.at(binding);
				if (input.Type != ShaderInputType::StorageBuffer)
				{
					fmt::format_to(stream, "Incompatible type for slot t{}\n", slot);
					fmt::format_to(stream, "Required is StorageBuffer but {} is provided\n", input.Type);
				}

				if (input.Items.empty() || !input.Items[0])
				{
					fmt::format_to(stream, "Input for t{} index {} is null\n", slot, 0);
					fmt::format_to(stream, "Required is StorageBuffer '{}'\n", storageBuffer.Name);
				}
			}

			for (const auto& [slot, image] : layout.Images)
			{
				const GraphicsBinding binding = { .Space = currentSet, .Slot = slot, .Register = GraphicsResourceType::ShaderResourceView };
				if (!inputSet.contains(binding))
				{
					fmt::format_to(stream, "Input missing for slot t{}\n", slot);
					fmt::format_to(stream, "Required is Image '{}'\n", image.Name);
					continue;
				}

				const auto& input = inputSet.at(binding);
				if (input.Type != ShaderInputType::Image2D && input.Type != ShaderInputType::ImageCube)
				{
					fmt::format_to(stream, "Incompatible type for slot t{}\n", slot);
					fmt::format_to(stream, "Required is Image but {} is provided\n", input.Type);
				}

				for (uint32_t i = 0; i < image.ArraySize; i++)
				{
					if (input.Items.size() < i || !input.Items[i])
					{
						fmt::format_to(stream, "Input for t{} index {} is null\n", slot, i);
						fmt::format_to(stream, "Required is Image '{}'\n", image.Name);
						continue;
					}
				}
			}

			for (const auto& [slot, storageImage] : layout.StorageImages)
			{
				const GraphicsBinding binding = { .Space = currentSet, .Slot = slot, .Register = GraphicsResourceType::UnorderedAccessView };
				if (!inputSet.contains(binding))
				{
					fmt::format_to(stream, "Input missing for slot u{}\n", slot);
					fmt::format_to(stream, "Required is StorageImage '{}'\n", storageImage.Name);
					continue;
				}

				const auto& input = inputSet.at(binding);
				if (input.Type != ShaderInputType::StorageImage2D && input.Type != ShaderInputType::StorageImageCube)
				{
					fmt::format_to(stream, "Incompatible type for slot u{}\n", slot);
					fmt::format_to(stream, "Required is StorageImage but {} is provided\n", input.Type);
				}

				for (uint32_t i = 0; i < storageImage.ArraySize; i++)
				{
					if (input.Items.size() < i || !input.Items[i])
					{
						fmt::format_to(stream, "Input for u{} index {} is null\n", slot, i);
						fmt::format_to(stream, "Required is StorageImage '{}'\n", storageImage.Name);
						continue;
					}
				}
			}

			for (const auto& [slot, sampler] : layout.Samplers)
			{
				const GraphicsBinding binding = { .Space = currentSet, .Slot = slot, .Register = GraphicsResourceType::Sampler };
				if (!inputSet.contains(binding))
				{
					fmt::format_to(stream, "Input missing for slot s{}\n", slot);
					fmt::format_to(stream, "Required is Sampler '{}'\n", sampler.Name);
					continue;
				}

				const auto& input = inputSet.at(binding);
				if (input.Type != ShaderInputType::Sampler)
				{
					fmt::format_to(stream, "Incompatible type for slot s{}\n", slot);
					fmt::format_to(stream, "Required is Sampler but {} is provided\n", input.Type);
				}

				for (uint32_t i = 0; i < sampler.ArraySize; i++)
				{
					if (input.Items.size() < i || !input.Items[i])
					{
						fmt::format_to(stream, "Input for s{} index {} is null\n", slot, i);
						fmt::format_to(stream, "Required is Sampler '{}'\n", sampler.Name);
						continue;
					}
				}
			}

			if (setError.size())
			{
				errorMessage += fmt::format("Invalid inputs in set {}\n{}", currentSet, fmt::to_string(setError));
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
			input.Items[arrayIndex] = constantBuffer;
			input.Type = ShaderInputType::ConstantBuffer;
			m_PendingSets.set(inputInfo->Set);
			SK_CORE_TRACE_TAG("Renderer", "[ShaderInputManager '{}'] Input set '{}':{}", m_Specification.DebugName, name, arrayIndex);
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
			input.Items[arrayIndex] = storageBuffer;
			input.Type = ShaderInputType::StorageBuffer;
			m_PendingSets.set(inputInfo->Set);
			SK_CORE_TRACE_TAG("Renderer", "[ShaderInputManager '{}'] Input set '{}':{}", m_Specification.DebugName, name, arrayIndex);
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
			input.Items[arrayIndex] = image;
			input.Type = ShaderInputType::Image2D;
			m_PendingSets.set(inputInfo->Set);
			SK_CORE_TRACE_TAG("Renderer", "[ShaderInputManager '{}'] Input set '{}':{}", m_Specification.DebugName, name, arrayIndex);
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
			input.Items[arrayIndex] = texture ? texture->GetImage() : nullptr;
			input.Type = ShaderInputType::Image2D;
			m_PendingSets.set(inputInfo->Set);
			SK_CORE_TRACE_TAG("Renderer", "[ShaderInputManager '{}'] Input set '{}':{}", m_Specification.DebugName, name, arrayIndex);
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
			input.Items[arrayIndex] = textureCube;
			input.Type = ShaderInputType::ImageCube;
			m_PendingSets.set(inputInfo->Set);
			SK_CORE_TRACE_TAG("Renderer", "[ShaderInputManager '{}'] Input set '{}':{}", m_Specification.DebugName, name, arrayIndex);
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
			input.Items[arrayIndex] = sampler;
			m_PendingSets.set(inputInfo->Set);
			SK_CORE_TRACE_TAG("Renderer", "[ShaderInputManager '{}'] Input set '{}':{}", m_Specification.DebugName, name, arrayIndex);
		}
		else
		{
			SK_CORE_WARN_TAG("Renderer", "[ShaderInputManager '{}'] Input '{}' not found", m_Specification.DebugName, name);
		}
	}

	const ShaderInputInfo* ShaderInputManager::GetInputInfo(const std::string& name) const
	{
		if (m_InputInfos.contains(name))
			return &m_InputInfos.at(name);

		return nullptr;
	}

}
