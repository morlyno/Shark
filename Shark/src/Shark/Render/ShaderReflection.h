#pragma once

#include "Shark/Core/Base.h"

#include <nvrhi/nvrhi.h>

namespace Shark {

	enum class GraphicsResourceType
	{
		None = 0,
		ConstantBuffer,
		ShaderResourceView,
		UnorderedAccessView,
		Sampler
	};

	struct GraphicsBinding
	{
		uint32_t Space = 0, Slot = 0;
		GraphicsResourceType Register = GraphicsResourceType::None;

		auto operator<=>(const GraphicsBinding&) const = default;
	};


	enum class ShaderInputType
	{
		None = 0,
		ConstantBuffer,
		StorageBuffer,
		Sampler,
		Image2D,
		ImageCube,
		StorageImage2D,
		StorageImageCube
	};

	struct ShaderInputInfo
	{
		std::string Name;
		uint32_t Set = 0;
		uint32_t Slot = 0;
		uint32_t Count = 0;
		GraphicsResourceType GraphicsType = GraphicsResourceType::None;
		ShaderInputType Type = ShaderInputType::None;

		GraphicsBinding GetGraphicsBinding() const { return { Set, Slot, GraphicsType }; }
	};

	enum class ResourceDimension
	{
		None = 0,
		Image2D,
		ImageCube
	};

	namespace ShaderResource {

		struct Buffer
		{
			std::string Name;
			uint32_t StructSize = 0;
			uint32_t Slot = 0;
			nvrhi::ShaderType Stage = nvrhi::ShaderType::None;
		};
		
		struct Image
		{
			std::string Name;
			uint32_t Slot = 0;
			uint32_t ArraySize = 1;
			uint32_t Dimension = 0;
			nvrhi::ShaderType Stage = nvrhi::ShaderType::None;
		};

		struct Sampler
		{
			std::string Name;
			uint32_t Slot = 0;
			uint32_t ArraySize = 1;
			nvrhi::ShaderType Stage = nvrhi::ShaderType::None;
		};

		struct PushConstant
		{
			static constexpr uint32_t Slot = 0;
			uint32_t StructSize = 0;
			nvrhi::ShaderType Stage = nvrhi::ShaderType::None;
		};

	}


	struct D3D11BindingSetOffsets
	{
		uint32_t ShaderResource = 0;
		uint32_t Sampler = 0;
		uint32_t ConstantBuffer = 0;
		uint32_t UnorderedAccess = 0;
	};

	struct ShaderBindingLayout
	{
		std::map<uint32_t, ShaderResource::Buffer> ConstantBuffers;
		std::map<uint32_t, ShaderResource::Buffer> StorageBuffers;
		std::map<uint32_t, ShaderResource::Image> Images;
		std::map<uint32_t, ShaderResource::Image> StorageImages;
		std::map<uint32_t, ShaderResource::Sampler> Samplers;

		nvrhi::ShaderType Stage = nvrhi::ShaderType::None;
		std::unordered_map<std::string, ShaderInputInfo> InputInfos;
		D3D11BindingSetOffsets BindingOffsets;
	};

	struct ShaderReflection
	{
		static constexpr uint32_t MaxLayouts = nvrhi::c_MaxBindingLayouts;

		nvrhi::static_vector<ShaderBindingLayout, MaxLayouts> BindingLayouts;
		std::optional<ShaderResource::PushConstant> PushConstant;

		//std::unordered_map<std::string, ShaderInputInfo> InputInfos;
	};

}

