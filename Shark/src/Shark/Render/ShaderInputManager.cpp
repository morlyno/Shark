#include "skpch.h"
#include "ShaderInputManager.h"

#include "Shark/Render/Renderer.h"
#include "Shark/Utils/String.h"

namespace Shark {

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
				case RenderInputType::Viewable:
				case RenderInputType::ImageView:
					return true;
			}
			SK_CORE_ASSERT(false, "Unknown RenderInputType");
			return false;
		}

		static bool IsImageInput(ShaderInputType type)
		{
			switch (type)
			{
				case ShaderInputType::ConstantBuffer:
				case ShaderInputType::StorageBuffer:
				case ShaderInputType::Sampler:
					return false;

				case ShaderInputType::Image:
				case ShaderInputType::Texture:
				case ShaderInputType::StorageImage:
					return true;
			}
			SK_CORE_ASSERT(false, "Unknown ShaderInputType");
			return false;
		}

		static std::string_view GetInputName(Ref<ConstantBuffer> input)
		{
			if (!input)
				return "<null>";

			return input->GetDebugName();
		}

		static std::string_view GetInputName(Ref<StorageBuffer> input)
		{
			if (!input)
				return "<null>";

			return input->GetDebugName();
		}

		static std::string_view GetInputName(Ref<Sampler> input)
		{
			if (!input)
				return "<null>";

			const auto& specification = input->GetSpecification();
			switch (specification.Address)
			{
				case AddressMode::Repeat: return specification.Filter == FilterMode::Linear ? "Linear-Repeat" : "Point-Repeat";
				case AddressMode::ClampToEdge: return specification.Filter == FilterMode::Linear ? "Linear-ClampToEdge" : "Point-ClampToEdge";
				case AddressMode::MirrorRepeat: return specification.Filter == FilterMode::Linear ? "Linear-MirrorRepeat" : "Point-MirrorRepeat";
			}

			return "<Sampler>";
		}

		static std::string_view GetInputName(Ref<Image2D> input)
		{
			if (!input)
				return "<null>";

			return input->GetSpecification().DebugName;
		}

		static std::string_view GetInputName(Ref<ImageView> input)
		{
			if (!input)
				return "<null>";

			return input->GetImage()->GetSpecification().DebugName;
		}

		static std::string_view GetInputName(Ref<Texture2D> input)
		{
			if (!input)
				return "<null>";

			return input->GetSpecification().DebugName;
		}

		static std::string_view GetInputName(Ref<TextureCube> input)
		{
			if (!input)
				return "<null>";

			return input->GetSpecification().DebugName;
		}

		static std::string_view GetInputName(Ref<ViewableResource> input)
		{
			if (!input)
				return "<null>";

			if (auto i = input.AsSafe<Image2D>())
				return GetInputName(i);

			if (auto i = input.AsSafe<Texture2D>())
				return GetInputName(i);

			if (auto i = input.AsSafe<ImageView>())
				return GetInputName(i);

			return "<Viewable>";
		}

	}
	
#if 1
	#define SK_LOG_INPUT(_item, _name, _arrayIndex) SK_CORE_TRACE_TAG("Renderer", "[ShaderInputManager '{}'] Input set '{}':{} => {}:'{}'", m_Specification.DebugName, _name, _arrayIndex, fmt::ptr(_item.Raw()), utils::GetInputName(_item));
#else
	#define SK_LOG_INPUT(_item, _name, _arrayIndex) (void)0
#endif

	ShaderInputManager::ShaderInputManager()
	{
		m_Specification.StartSet = 1;
		m_Specification.EndSet = 0;
	}

	ShaderInputManager::ShaderInputManager(const ShaderInputManagerSpecification& specification)
	{
		Initialize(specification);
	}

	ShaderInputManager::~ShaderInputManager()
	{
	}

	void ShaderInputManager::Initialize(const ShaderInputManagerSpecification& specification)
	{
		m_Specification = specification;

		if (m_Specification.DebugName.empty())
			m_Specification.DebugName = "<UNNAMED>";

		auto deviceManager = Renderer::GetDeviceManager();
		m_EnableValidation = deviceManager->GetSpecification().EnableNvrhiValidationLayer;

		const auto& reflection = m_Specification.Shader->GetReflectionData();
		if (reflection.BindingLayouts.empty())
		{
			m_Specification.StartSet = 1;
			m_Specification.EndSet = 0;
			return;
		}

		m_Specification.EndSet = std::min(m_Specification.EndSet, (uint32_t)reflection.BindingLayouts.size() - 1);

		const auto& preBackedSets = m_Specification.Shader->GetRequestedBindingSets();

		for (uint32_t set = m_Specification.StartSet; set <= m_Specification.EndSet; set++)
		{
			if (!m_Specification.Shader->HasLayout(set))
				continue;

			if (preBackedSets.contains(set))
			{
				m_Handles[set] = preBackedSets.at(set);
				continue;
			}

			m_SetCount += 1;
			m_Managers[set] = Scope<DescriptorSetManager>::Create(m_Specification.Shader, set, m_Specification.DebugName);

			const auto& layout = reflection.BindingLayouts[set];
			for (const auto& [name, inputInfo] : layout.InputInfos)
			{
				m_InputInfos[name] = &inputInfo;
				m_InputCount += inputInfo.Count;

				auto& input = m_Inputs[inputInfo.GetGraphicsBinding()];
				input.Type = inputInfo.Type;
				input.Items.resize(inputInfo.Count);
			}
		}

		m_Updates.reserve(m_InputCount);
	}

	bool ShaderInputManager::Package(std::vector<InputUpdate>& outUpdates)
	{
		if (m_Updates.empty())
			return false;

		outUpdates = m_Updates;
		m_Updates.clear();
		return true;
	}

	void ShaderInputManager::PrepareAll()
	{
		m_Updates.clear();

		for (const auto& [binding, input] : m_Inputs)
		{
			for (uint32_t i = 0; i < input.Items.size(); i++)
			{
				m_Updates.push_back(
					InputUpdate{
						binding, i,
						input.Items[i].Item,
						input.Items[i].Type,
						input.Items[i].ViewArgs
					}
				);
			}
		}
	}

	void ShaderInputManager::Update(std::span<const InputUpdate> updates, bool force)
	{
		for (const auto& input : updates)
		{
			auto* manager = m_Managers[input.Binding.Space].Raw();
			auto* info = manager->GetInputInfo(input.Binding.Slot, input.Binding.Register);

			InputKey key = {
				input.Binding.Slot,
				input.Binding.Register,
				input.ArrayIndex
			};

			manager->SetInput(key, input.Input->GetResourceHandle());

			if (utils::IsImageInput(info->Type))
			{
				auto viewable = input.Input.As<ViewableResource>();
				const auto& view = viewable->GetViewInfo();

				manager->SetDescriptor(
					key,
					input.ViewArgs.SubresourceSet.value_or(view.SubresourceSet),
					input.ViewArgs.Format.value_or(view.Format),
					input.ViewArgs.Dimension.value_or(view.Dimension)
				);

				if (info->Type == ShaderInputType::Texture)
				{
					manager->SetInput(key, view.TextureSampler, true);
				}
			}
		}

		uint8_t setsUpdated = 0;
		for (uint32_t set = m_Specification.StartSet; set <= m_Specification.EndSet; set++)
		{
			if (!m_Managers[set])
				continue;

			auto& manager = m_Managers[set];
			if (m_EnableValidation && !manager->Validate())
			{
				continue;
			}

			if (manager->Update(force))
			{
				m_Handles[set] = manager->GetHandle();
				setsUpdated |= BIT(set);
			}
		}

		SK_CORE_INFO_TAG("Renderer", "[ShaderInputManager '{}'] {} out of {} sets {} ({:08b})", m_Specification.DebugName, std::popcount(setsUpdated), m_SetCount, force ? "created" : "updated", setsUpdated);
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
			auto stream = fmt::appender(setErrors[inputInfo->Set]);

			const auto& input = m_Inputs.at(inputInfo->GetGraphicsBinding());

			for (uint32_t i = 0; i < inputInfo->Count; i++)
			{
				if (!input.Items[i].Item)
				{
					if (inputInfo->Type == ShaderInputType::Texture)
					{
						const auto& sampledImage = reflection.BindingLayouts[inputInfo->Set].SampledImages.at(inputInfo->Slot);
						fmt::format_to(stream, " - Input for (t{} s{}) index {} is null\n", sampledImage.SeparateImage.Slot, sampledImage.SeparateSampler.Slot, i);
						fmt::format_to(stream, "   Required is Texture '{}'\n", inputInfo->Name);
					}
					else
					{
						fmt::format_to(stream, " - Input for {}{} index {} is null\n", utils::GetBindingPrefix(inputInfo->GraphicsType), inputInfo->Slot, i);
						fmt::format_to(stream, "   Required is {} '{}'\n", inputInfo->Type, inputInfo->Name);
					}
					continue;
				}

				if (!utils::InputCompadible(input.Items[i].Type, input.Type))
				{
					if (inputInfo->Type == ShaderInputType::ConstantBuffer || inputInfo->Type == ShaderInputType::StorageBuffer)
					{
						fmt::format_to(stream, " - Incompatible type for '{}' slot {}{}\n", inputInfo->Name, utils::GetBindingPrefix(inputInfo->GraphicsType), inputInfo->Slot);
						fmt::format_to(stream, "   Required is {} but {} is provided\n", inputInfo->Type, input.Items[i].Type);
					}
					else if (inputInfo->Type == ShaderInputType::Texture)
					{
						const auto& sampledImage = reflection.BindingLayouts[inputInfo->Set].SampledImages.at(inputInfo->Slot);
						fmt::format_to(stream, " - Incompatible type for '{}' (t{}, s{}) index {}\n", sampledImage.Name, sampledImage.SeparateImage.Slot, sampledImage.SeparateSampler.Slot, i);
						fmt::format_to(stream, "   Required is Texture but {} is provided\n", input.Items[i].Type);
					}
					else
					{
						fmt::format_to(stream, " - Incompatible type for '{}' slot {}{} index {}\n", inputInfo->Name, utils::GetBindingPrefix(inputInfo->GraphicsType), inputInfo->Slot, i);
						fmt::format_to(stream, "   Required is {} but {} is provided\n", inputInfo->Type, input.Items[i].Type);
					}
				}
				else if (input.Items[i].Type == RenderInputType::Viewable && input.Type == ShaderInputType::Texture)
				{
					auto viewable = input.Items[i].Item.As<ViewableResource>();
					if (!viewable->HasSampler())
					{
						const auto& sampledImage = reflection.BindingLayouts[inputInfo->Set].SampledImages.at(inputInfo->Slot);
						fmt::format_to(stream, " - Incompatible tye for '{}' (t{}, s{}) index {}\n", sampledImage.Name, sampledImage.SeparateImage.Slot, sampledImage.SeparateSampler.Slot, i);
						fmt::format_to(stream, "   Required is Texture but Viewable without Sampler is provided\n");
					}
				}

				if (inputInfo->Type == ShaderInputType::StorageImage && !IsWritable(input.Items[i]))
				{
					fmt::format_to(stream, " - Input '{}' for u{} index {} is not writable\n", inputInfo->Name, inputInfo->Slot, i);
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
		if (!inputInfo)
		{
			SK_CORE_WARN_TAG("Renderer", "[ShaderInputManager '{}'] Input '{}' not found", m_Specification.DebugName, name);
			return;
		}

		BindingSetInput& input = m_Inputs.at(inputInfo->GetGraphicsBinding());
		if (input.IsSame(arrayIndex, constantBuffer))
			return;

		input.Set(arrayIndex, constantBuffer);
		SK_LOG_INPUT(constantBuffer, name, arrayIndex);

		m_Updates.push_back(
			InputUpdate{
				.Binding = inputInfo->GetGraphicsBinding(),
				.ArrayIndex = arrayIndex,
				.Input = constantBuffer,
				.Type = RenderInputType::ConstantBuffer
			}
		);
	}

	void ShaderInputManager::SetInput(const std::string& name, Ref<StorageBuffer> storageBuffer, uint32_t arrayIndex)
	{
		const ShaderInputInfo* inputInfo = GetInputInfo(name);
		if (!inputInfo)
		{
			SK_CORE_WARN_TAG("Renderer", "[ShaderInputManager '{}'] Input '{}' not found", m_Specification.DebugName, name);
			return;
		}

		BindingSetInput& input = m_Inputs.at(inputInfo->GetGraphicsBinding());
		if (input.IsSame(arrayIndex, storageBuffer))
			return;

		input.Set(arrayIndex, storageBuffer);
		SK_LOG_INPUT(storageBuffer, name, arrayIndex);

		m_Updates.push_back(
			InputUpdate{
				.Binding = inputInfo->GetGraphicsBinding(),
				.ArrayIndex = arrayIndex,
				.Input = storageBuffer,
				.Type = RenderInputType::StorageBuffer
			}
		);
	}

	void ShaderInputManager::SetInput(const std::string& name, Ref<ViewableResource> viewable, uint32_t arrayIndex)
	{
		const ShaderInputInfo* inputInfo = GetInputInfo(name);
		if (!inputInfo)
		{
			SK_CORE_WARN_TAG("Renderer", "[ShaderInputManager '{}'] Input '{}' not found", m_Specification.DebugName, name);
			return;
		}

		BindingSetInput& input = m_Inputs.at(inputInfo->GetGraphicsBinding());
		if (input.IsSame(arrayIndex, viewable))
			return;

		input.Set(arrayIndex, viewable);
		SK_LOG_INPUT(viewable, name, arrayIndex);

		m_Updates.push_back(
			InputUpdate{
				.Binding = inputInfo->GetGraphicsBinding(),
				.ArrayIndex = arrayIndex,
				.Input = viewable,
				.Type = RenderInputType::Viewable
			}
		);
	}

	void ShaderInputManager::SetInput(const std::string& name, Ref<Image2D> image, uint32_t arrayIndex)
	{
		const ShaderInputInfo* inputInfo = GetInputInfo(name);
		if (!inputInfo)
		{
			SK_CORE_WARN_TAG("Renderer", "[ShaderInputManager '{}'] Input '{}' not found", m_Specification.DebugName, name);
			return;
		}

		BindingSetInput& input = m_Inputs.at(inputInfo->GetGraphicsBinding());
		if (input.IsSame(arrayIndex, image))
			return;

		input.Set(arrayIndex, image);
		SK_LOG_INPUT(image, name, arrayIndex);

		m_Updates.push_back(
			InputUpdate{
				.Binding = inputInfo->GetGraphicsBinding(),
				.ArrayIndex = arrayIndex,
				.Input = image,
				.Type = RenderInputType::Image2D
			}
		);
	}

	void ShaderInputManager::SetInput(const std::string& name, Ref<ImageView> imageView, uint32_t arrayIndex)
	{
		const ShaderInputInfo* inputInfo = GetInputInfo(name);
		if (!inputInfo)
		{
			SK_CORE_WARN_TAG("Renderer", "[ShaderInputManager '{}'] Input '{}' not found", m_Specification.DebugName, name);
			return;
		}

		BindingSetInput& input = m_Inputs.at(inputInfo->GetGraphicsBinding());
		if (input.IsSame(arrayIndex, imageView))
			return;

		input.Set(arrayIndex, imageView);
		SK_LOG_INPUT(imageView, name, arrayIndex);

		m_Updates.push_back(
			InputUpdate{
				.Binding = inputInfo->GetGraphicsBinding(),
				.ArrayIndex = arrayIndex,
				.Input = imageView,
				.Type = RenderInputType::ImageView
			}
		);
	}

	void ShaderInputManager::SetInput(const std::string& name, Ref<Texture2D> texture, uint32_t arrayIndex)
	{
		const ShaderInputInfo* inputInfo = GetInputInfo(name);
		if (!inputInfo)
		{
			SK_CORE_WARN_TAG("Renderer", "[ShaderInputManager '{}'] Input '{}' not found", m_Specification.DebugName, name);
			return;
		}

		BindingSetInput& input = m_Inputs.at(inputInfo->GetGraphicsBinding());
		if (input.IsSame(arrayIndex, texture))
			return;

		input.Set(arrayIndex, texture);
		SK_LOG_INPUT(texture, name, arrayIndex);

		m_Updates.push_back(
			InputUpdate{
				.Binding = inputInfo->GetGraphicsBinding(),
				.ArrayIndex = arrayIndex,
				.Input = texture,
				.Type = RenderInputType::Texture2D
			}
		);
	}

	void ShaderInputManager::SetInput(const std::string& name, Ref<TextureCube> textureCube, uint32_t arrayIndex)
	{
		const ShaderInputInfo* inputInfo = GetInputInfo(name);
		if (!inputInfo)
		{
			SK_CORE_WARN_TAG("Renderer", "[ShaderInputManager '{}'] Input '{}' not found", m_Specification.DebugName, name);
			return;
		}

		BindingSetInput& input = m_Inputs.at(inputInfo->GetGraphicsBinding());
		if (input.IsSame(arrayIndex, textureCube))
			return;

		input.Set(arrayIndex, textureCube);
		SK_LOG_INPUT(textureCube, name, arrayIndex);

		m_Updates.push_back(
			InputUpdate{
				.Binding = inputInfo->GetGraphicsBinding(),
				.ArrayIndex = arrayIndex,
				.Input = textureCube,
				.Type = RenderInputType::TextureCube
			}
		);
	}

	void ShaderInputManager::SetInput(const std::string& name, Ref<Sampler> sampler, uint32_t arrayIndex)
	{
		const ShaderInputInfo* inputInfo = GetInputInfo(name);
		if (!inputInfo)
		{
			SK_CORE_WARN_TAG("Renderer", "[ShaderInputManager '{}'] Input '{}' not found", m_Specification.DebugName, name);
			return;
		}

		BindingSetInput& input = m_Inputs.at(inputInfo->GetGraphicsBinding());
		if (input.IsSame(arrayIndex, sampler))
			return;

		input.Set(arrayIndex, sampler);
		SK_LOG_INPUT(sampler, name, arrayIndex);

		m_Updates.push_back(
			InputUpdate{
				.Binding = inputInfo->GetGraphicsBinding(),
				.ArrayIndex = arrayIndex,
				.Input = sampler,
				.Type = RenderInputType::Sampler
			}
		);
	}

	void ShaderInputManager::SetInput(const std::string& name, Ref<ViewableResource> viewable, const InputViewArgs& viewArgs, uint32_t arrayIndex)
	{
		const auto* info = GetInputInfo(name);
		if (!info)
		{
			SK_CORE_WARN_TAG("Renderer", "[ShaderInputManager '{}'] Input '{}' not found", m_Specification.DebugName, name);
			return;
		}

		BindingSetInput& input = m_Inputs.at(info->GetGraphicsBinding());
		if (input.IsSame(arrayIndex, viewable, viewArgs))
			return;

		input.Set(arrayIndex, viewable, viewArgs);
		SK_LOG_INPUT(viewable, name, arrayIndex);

		m_Updates.push_back(
			InputUpdate{
				.Binding = info->GetGraphicsBinding(),
				.ArrayIndex = arrayIndex,
				.Input = viewable,
				.Type = RenderInputType::Viewable,
				.ViewArgs = viewArgs
			}
		);
	}

	void ShaderInputManager::SetInput(const std::string& name, Ref<Image2D> image, const InputViewArgs& viewArgs, uint32_t arrayIndex)
	{
		const auto* info = GetInputInfo(name);
		if (!info)
		{
			SK_CORE_WARN_TAG("Renderer", "[ShaderInputManager '{}'] Input '{}' not found", m_Specification.DebugName, name);
			return;
		}

		BindingSetInput& input = m_Inputs.at(info->GetGraphicsBinding());
		if (input.IsSame(arrayIndex, image, viewArgs))
			return;

		input.Set(arrayIndex, image, viewArgs);
		SK_LOG_INPUT(image, name, arrayIndex);

		m_Updates.push_back(
			InputUpdate{
				.Binding = info->GetGraphicsBinding(),
				.ArrayIndex = arrayIndex,
				.Input = image,
				.Type = RenderInputType::Image2D,
				.ViewArgs = viewArgs
			}
		);
	}

	void ShaderInputManager::SetInput(const std::string& name, Ref<ImageView> imageView, const InputViewArgs& viewArgs, uint32_t arrayIndex)
	{
		const auto* info = GetInputInfo(name);
		if (!info)
		{
			SK_CORE_WARN_TAG("Renderer", "[ShaderInputManager '{}'] Input '{}' not found", m_Specification.DebugName, name);
			return;
		}

		BindingSetInput& input = m_Inputs.at(info->GetGraphicsBinding());
		if (input.IsSame(arrayIndex, imageView, viewArgs))
			return;

		input.Set(arrayIndex, imageView, viewArgs);
		SK_LOG_INPUT(imageView, name, arrayIndex);

		m_Updates.push_back(
			InputUpdate{
				.Binding = info->GetGraphicsBinding(),
				.ArrayIndex = arrayIndex,
				.Input = imageView,
				.Type = RenderInputType::ImageView,
				.ViewArgs = viewArgs
			}
		);
	}

	void ShaderInputManager::SetInput(const std::string& name, Ref<Texture2D> texture, const InputViewArgs& viewArgs, uint32_t arrayIndex)
	{
		const auto* info = GetInputInfo(name);
		if (!info)
		{
			SK_CORE_WARN_TAG("Renderer", "[ShaderInputManager '{}'] Input '{}' not found", m_Specification.DebugName, name);
			return;
		}

		BindingSetInput& input = m_Inputs.at(info->GetGraphicsBinding());
		if (input.IsSame(arrayIndex, texture, viewArgs))
			return;

		input.Set(arrayIndex, texture, viewArgs);
		SK_LOG_INPUT(texture, name, arrayIndex);

		m_Updates.push_back(
			InputUpdate{
				.Binding = info->GetGraphicsBinding(),
				.ArrayIndex = arrayIndex,
				.Input = texture,
				.Type = RenderInputType::Texture2D,
				.ViewArgs = viewArgs
			}
		);
	}

	void ShaderInputManager::SetInput(const std::string& name, Ref<TextureCube> textureCube, const InputViewArgs& viewArgs, uint32_t arrayIndex)
	{
		const auto* info = GetInputInfo(name);
		if (!info)
		{
			SK_CORE_WARN_TAG("Renderer", "[ShaderInputManager '{}'] Input '{}' not found", m_Specification.DebugName, name);
			return;
		}

		BindingSetInput& input = m_Inputs.at(info->GetGraphicsBinding());
		if (input.IsSame(arrayIndex, textureCube, viewArgs))
			return;

		input.Set(arrayIndex, textureCube, viewArgs);
		SK_LOG_INPUT(textureCube, name, arrayIndex);

		m_Updates.push_back(
			InputUpdate{
				.Binding = info->GetGraphicsBinding(),
				.ArrayIndex = arrayIndex,
				.Input = textureCube,
				.Type = RenderInputType::TextureCube,
				.ViewArgs = viewArgs
			}
		);
	}

	const ShaderInputInfo* ShaderInputManager::GetInputInfo(const std::string& name) const
	{
		if (m_InputInfos.contains(name))
			return m_InputInfos.at(name);

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
