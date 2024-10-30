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

		using MemberList = std::vector<MemberDeclaration>;

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
			StorageImageCube,

			ConstantBuffer,
			StorageBuffer,
			PushConstant
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

		constexpr bool IsTextureType(ResourceType type)
		{
			switch (type)
			{
				case ResourceType::Texture2D:
				case ResourceType::Texture3D:
				case ResourceType::TextureCube:
					return true;
			}
			return false;
		}

		constexpr bool IsStorageImage(ResourceType type)
		{
			switch (type)
			{
				case ResourceType::StorageImage2D:
				case ResourceType::StorageImage3D:
				case ResourceType::StorageImageCube:
					return true;
			}
			return false;
		}

		struct Resource
		{
			std::string Name;
			uint32_t Set = (uint32_t)-1, Binding = (uint32_t)-1;
			ShaderStage Stage = ShaderStage::None;
			ResourceType Type = ResourceType::None;
			uint32_t ArraySize = 0;

			// ConstantBuffer size
			uint32_t StructSize = 0;

			// Used only for DirectX 11
			uint32_t DXBinding = (uint32_t)-1;

			// For all combined image samples this value is valid and is currently the same as DXBinding
			uint32_t DXSamplerBinding = (uint32_t)-1;
		};

	}

	struct ShaderReflectionData
	{
		// set => binding => resource
		std::map<uint32_t, std::map<uint32_t, ShaderReflection::Resource>> Resources;
		std::map<uint32_t, std::map<uint32_t, ShaderReflection::MemberList>> Members;

		bool HasPushConstant = false;
		ShaderReflection::Resource PushConstant;
		ShaderReflection::MemberList PushConstantMembers;

		std::unordered_map<std::string, std::pair<uint32_t, uint32_t>> NameCache;
		std::unordered_map<std::string, std::tuple<uint32_t, uint32_t, uint32_t>> MemberNameCache;
	};

}
