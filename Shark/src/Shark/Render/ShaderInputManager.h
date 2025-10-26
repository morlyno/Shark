#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/Shader.h"
#include "Shark/Render/ConstantBuffer.h"
#include "Shark/Render/StorageBuffer.h"
#include "Shark/Render/Image.h"
#include "Shark/Render/Texture.h"

#include <nvrhi/nvrhi.h>

namespace Shark {

	struct BindingSetInput
	{
		ShaderInputType Type = ShaderInputType::None;
		std::vector<Ref<RendererResource>> Items;

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
		ShaderInputManager(const ShaderInputManagerSpecification& specification);
		~ShaderInputManager();

		void Bake();
		void Update();
		bool Validate() const;

		void SetInput(const std::string& name, Ref<ConstantBuffer> constantBuffer, uint32_t arrayIndex = 0);
		void SetInput(const std::string& name, Ref<StorageBuffer> storageBuffer, uint32_t arrayIndex = 0);
		void SetInput(const std::string& name, Ref<Image2D> image, uint32_t arrayIndex = 0);
		void SetInput(const std::string& name, Ref<Texture2D> texture, uint32_t arrayIndex = 0);
		void SetInput(const std::string& name, Ref<TextureCube> textureCube, uint32_t arrayIndex = 0);
		void SetInput(const std::string& name, Ref<Sampler> sampler, uint32_t arrayIndex = 0);

		uint32_t GetStartSet() const { return m_Specification.StartSet; }
		uint32_t GetEndSet() const { return m_Specification.EndSet; }
		nvrhi::BindingSetHandle GetHandle(uint32_t set) const { return m_BackedSets[set - m_Specification.StartSet]; }

	private:
		const ShaderInputInfo* GetInputInfo(const std::string& name) const;

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
