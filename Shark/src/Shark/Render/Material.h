#pragma once

#include "Shark/Render/Shaders.h"
#include "Shark/Render/Texture.h"

namespace Shark {

	enum class DataType
	{
		None = 0,
		Void,
		Bool, Int, Float,
		String,
		Texture, Texture1D, Texture2D, Texture3D, TextureCube,
	};

	struct ResourceDescriptor
	{
		DataType Type = DataType::None;
		uint32_t Rows = 0;
		uint32_t Collums = 0;
		void* Data = nullptr;
	};

	struct MaterialDescriptor
	{
		std::unordered_map<std::string, ResourceDescriptor> Resources;
	};

	enum MaterialFlag
	{
		DepthTest = BIT(0),
		Blend = BIT(1),
		TwoSided = BIT(2),

		OutLine = BIT(3),
		Blur = BIT(4),
		Bloom = BIT(5)
	};

	class Material : public RefCount
	{
	public:
		virtual ~Material() = default;

		virtual Ref<Shaders> GetShaders() const = 0;

		virtual const std::string& GetName() const = 0;
		virtual const MaterialDescriptor& GetDescriptor() const = 0;

		virtual void SetFlag(MaterialFlag flag, bool enabled) = 0;
		virtual bool IsFalgSet(MaterialFlag flag) = 0;

		virtual void Set(const std::string& name, const void* data, uint32_t size) = 0;
		virtual void Set(const std::string& name, const Ref<Texture2D>& texture) = 0;

		virtual void Bind() = 0;

		static Ref<Material> Create(const Ref<Shaders>& shaders, const std::string& name);
	};

}
