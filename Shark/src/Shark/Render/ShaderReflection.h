#pragma once

#include "Shark/Core/Base.h"

namespace Shark {

	namespace ShaderReflection {

		enum class ShaderStage
		{
			None = 0,
			Vertex,
			Pixel,
			Compute
		};

		enum class VariableType
		{
			None = 0,
			Int, Int2, Int3, Int4,
			UInt, UInt2, UInt3, UInt4,
			Float, Float2, Float3, Float4,
			Mat3, Mat4,
			Bool
		};

		struct MemberDeclaration
		{
			std::string Name;
			VariableType Type = VariableType::None;
			uint32_t Size = 0;
			uint32_t Offset = 0;
		};

		enum class UpdateFrequencyType
		{
			None = 0,
			PerScene,
			PerMaterial,
			PerDrawCall,
		};

		struct ConstantBuffer
		{
			std::string Name;
			ShaderStage Stage = ShaderStage::None;
			UpdateFrequencyType UpdateFrequency = UpdateFrequencyType::None;
			uint32_t Binding;
			uint32_t Size;
			uint32_t MemberCount;
			std::vector<MemberDeclaration> Members;
		};

		enum class ResourceType
		{
			None = 0,

			// Sampler Only
			Sampler,

			// Texture Only
			Image2D,
			Image3D,
			ImageCube,

			// Combined Image Sampler
			Texture2D,
			Texture3D,
			TextureCube,

			StorageImage2D,
			StorageImage3D,
			StorageImageCube
		};

		constexpr bool IsImageType(ResourceType type)
		{
			switch (type)
			{
				case ResourceType::Image2D:
				case ResourceType::Image3D:
				case ResourceType::ImageCube:
					return true;
			}
			return false;
		}

		struct Resource
		{
			std::string Name;
			ShaderStage Stage = ShaderStage::None;
			ResourceType Type = ResourceType::None;
			UpdateFrequencyType UpdateFrequency = UpdateFrequencyType::None;
			uint32_t Binding;
			uint32_t ArraySize;

			// Only used for ResourceType::Texture*
			// for ResourceType::Sampler it's the same as Binding
			uint32_t SamplerBinding;
		};

	}

	struct ShaderReflectionData
	{
		std::unordered_map<std::string, ShaderReflection::ConstantBuffer> ConstantBuffers;
		std::unordered_map<std::string, ShaderReflection::Resource> Resources;
	};

	inline std::string ToString(ShaderReflection::ResourceType resourceType)
	{
		switch (resourceType)
		{
			case ShaderReflection::ResourceType::None: return "None";
			case ShaderReflection::ResourceType::Image2D: return "Texture2D";
			case ShaderReflection::ResourceType::Image3D: return "Texture3D";
			case ShaderReflection::ResourceType::ImageCube: return "TextureCube";
			case ShaderReflection::ResourceType::Sampler: return "Sampler";
			case ShaderReflection::ResourceType::Texture2D: return "Sampler2D";
			case ShaderReflection::ResourceType::Texture3D: return "Sampler3D";
			case ShaderReflection::ResourceType::TextureCube: return "SamplerCube";
			case ShaderReflection::ResourceType::StorageImage2D: return "StorageImage2D";
			case ShaderReflection::ResourceType::StorageImage3D: return "StorageImage3D";
			case ShaderReflection::ResourceType::StorageImageCube: return "StorageImageCube";
		}

		SK_CORE_ASSERT(false, "Unkown ShaderRelfection::ResourceType");
		return "Unkown";
	}

	inline ShaderReflection::ResourceType StringToShaderReflectionResourceType(const std::string& resourceType)
	{
		if (resourceType == "None") return ShaderReflection::ResourceType::None;
		if (resourceType == "Texture2D") return ShaderReflection::ResourceType::Image2D;
		if (resourceType == "Texture3D") return ShaderReflection::ResourceType::Image3D;
		if (resourceType == "TextureCube") return ShaderReflection::ResourceType::ImageCube;
		if (resourceType == "Sampler") return ShaderReflection::ResourceType::Sampler;
		if (resourceType == "Sampler2D") return ShaderReflection::ResourceType::Texture2D;
		if (resourceType == "Sampler3D") return ShaderReflection::ResourceType::Texture3D;
		if (resourceType == "SamplerCube") return ShaderReflection::ResourceType::TextureCube;
		if (resourceType == "StorageImage2D") return ShaderReflection::ResourceType::StorageImage2D;
		if (resourceType == "StorageImage3D") return ShaderReflection::ResourceType::StorageImage3D;
		if (resourceType == "StorageImageCube") return ShaderReflection::ResourceType::StorageImageCube;

		SK_CORE_ASSERT(false, "Unkown ShaderReflection::ResourceType");
		return ShaderReflection::ResourceType::None;
	}

	inline std::string ToString(ShaderReflection::ShaderStage stage)
	{
		switch (stage)
		{
			case ShaderReflection::ShaderStage::None: return "None";
			case ShaderReflection::ShaderStage::Vertex: return "Vertex";
			case ShaderReflection::ShaderStage::Pixel: return "Pixel";
			case ShaderReflection::ShaderStage::Compute: return "Compute";
		}

		SK_CORE_ASSERT(false, "Unkown ShaderReflection::ShaderStage");
		return "Unkown";
	}

	inline ShaderReflection::ShaderStage StringToShaderReflectionShaderStage(const std::string& shaderStage)
	{
		if (shaderStage == "None")
			return ShaderReflection::ShaderStage::None;
		if (shaderStage == "Vertex")
			return ShaderReflection::ShaderStage::Vertex;
		if (shaderStage == "Pixel")
			return ShaderReflection::ShaderStage::Pixel;
		if (shaderStage == "Compute")
			return ShaderReflection::ShaderStage::Compute;

		SK_CORE_ASSERT(false, "Unkown ShaderRelfection::ShaderStage");
		return ShaderReflection::ShaderStage::None;
	}

	inline std::string ToString(ShaderReflection::VariableType variableType)
	{
		switch (variableType)
		{
			case ShaderReflection::VariableType::None: return "None";
			case ShaderReflection::VariableType::Int: return "Int";
			case ShaderReflection::VariableType::Int2: return "Int2";
			case ShaderReflection::VariableType::Int3: return "Int3";
			case ShaderReflection::VariableType::Int4: return "Int4";
			case ShaderReflection::VariableType::UInt: return "UInt";
			case ShaderReflection::VariableType::UInt2: return "UInt2";
			case ShaderReflection::VariableType::UInt3: return "UInt3";
			case ShaderReflection::VariableType::UInt4: return "UInt4";
			case ShaderReflection::VariableType::Float: return "Float";
			case ShaderReflection::VariableType::Float2: return "Float2";
			case ShaderReflection::VariableType::Float3: return "Float3";
			case ShaderReflection::VariableType::Float4: return "Float4";
			case ShaderReflection::VariableType::Mat3: return "Mat3";
			case ShaderReflection::VariableType::Mat4: return "Mat4";
			case ShaderReflection::VariableType::Bool: return "Bool";
		}

		SK_CORE_ASSERT(false, "Unkown ShaderReflection::VariabelType");
		return "Unkown";
	}

	inline ShaderReflection::VariableType StringToShaderReflectionVariabelType(const std::string& variableType)
	{
		if (variableType == "None") return ShaderReflection::VariableType::None;
		if (variableType == "Int") return ShaderReflection::VariableType::Int;
		if (variableType == "Int2") return ShaderReflection::VariableType::Int2;
		if (variableType == "Int3") return ShaderReflection::VariableType::Int3;
		if (variableType == "Int4") return ShaderReflection::VariableType::Int4;
		if (variableType == "UInt") return ShaderReflection::VariableType::UInt;
		if (variableType == "UInt2") return ShaderReflection::VariableType::UInt2;
		if (variableType == "UInt3") return ShaderReflection::VariableType::UInt3;
		if (variableType == "UInt4") return ShaderReflection::VariableType::UInt4;
		if (variableType == "Float") return ShaderReflection::VariableType::Float;
		if (variableType == "Float2") return ShaderReflection::VariableType::Float2;
		if (variableType == "Float3") return ShaderReflection::VariableType::Float3;
		if (variableType == "Float4") return ShaderReflection::VariableType::Float4;
		if (variableType == "Mat3") return ShaderReflection::VariableType::Mat3;
		if (variableType == "Mat4") return ShaderReflection::VariableType::Mat4;
		if (variableType == "Bool") return ShaderReflection::VariableType::Bool;

		SK_CORE_ASSERT(false, "Unkown ShaderReflection::VariabelType");
		return ShaderReflection::VariableType::None;
	}

	inline std::string ToString(ShaderReflection::UpdateFrequencyType updateFrequency)
	{
		switch (updateFrequency)
		{
			case ShaderReflection::UpdateFrequencyType::None: return "None";
			case ShaderReflection::UpdateFrequencyType::PerScene: return "PerScene";
			case ShaderReflection::UpdateFrequencyType::PerMaterial: return "PerMaterial";
			case ShaderReflection::UpdateFrequencyType::PerDrawCall: return "PerDrawCall";
		}

		SK_CORE_ASSERT(false, "Unkown ShaderReflection::UpdateFrequencyType");
		return "Unkown";
	}

	inline ShaderReflection::UpdateFrequencyType StringToShaderReflectionUpdateFrequency(const std::string& updateFrequency)
	{
		if (updateFrequency == "None") return ShaderReflection::UpdateFrequencyType::None;
		if (updateFrequency == "PerScene") return ShaderReflection::UpdateFrequencyType::PerScene;
		if (updateFrequency == "PerMaterial") return ShaderReflection::UpdateFrequencyType::PerMaterial;
		if (updateFrequency == "PerDrawCall") return ShaderReflection::UpdateFrequencyType::PerDrawCall;

		SK_CORE_ASSERT(false, "Unkown ShaderReflection::UpdateFrequencyType");
		return ShaderReflection::UpdateFrequencyType::None;
	}

}
