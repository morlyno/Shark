#pragma once

#include "Shark/Render/ShaderReflection.h"
#include "Shark/Serialization/YAML.h"
#include <nvrhi/nvrhi.h>

namespace YAML {

	template <typename T, std::size_t N>
	struct convert<nvrhi::static_vector<T, N>>
	{
		static Node encode(const nvrhi::static_vector<T, N>& rhs)
		{
			Node node(NodeType::Sequence);
			for (const auto& element : rhs)
				node.push_back(element);
			return node;
		}

		static bool decode(const Node& node, nvrhi::static_vector<T, N>& rhs)
		{
			if (!isNodeValid(node))
				return false;

			rhs.resize(node.size());
			for (auto i = 0u; i < node.size(); ++i)
				rhs[i] = node[i].as<T>();

			return true;
		}

	private:
		static bool isNodeValid(const Node& node)
		{
			return node.IsSequence() && node.size() <= N;
		}
	};

	template<>
	struct convert<Shark::ShaderResource::Buffer>
	{
		static Node encode(const Shark::ShaderResource::Buffer& resource)
		{
			Node node(NodeType::Map);
			node.force_insert("Name", resource.Name);
			node.force_insert("StructSize", resource.StructSize);
			node.force_insert("Slot", resource.Slot);
			node.force_insert("Stage", resource.Stage);
			return node;
		}

		static bool decode(const Node& node, Shark::ShaderResource::Buffer& outResource)
		{
			if (!node.IsMap() || node.size() != 4)
				return false;

			outResource.Name       = node["Name"].as<std::string>();
			outResource.StructSize = node["StructSize"].as<uint32_t>();
			outResource.Slot       = node["Slot"].as<uint32_t>();
			outResource.Stage      = node["Stage"].as<nvrhi::ShaderType>();
			return true;
		}
	};

	template<>
	struct convert<Shark::ShaderResource::Image>
	{
		static Node encode(const Shark::ShaderResource::Image& resource)
		{
			Node node(NodeType::Map);
			node.force_insert("Name", resource.Name);
			node.force_insert("Slot", resource.Slot);
			node.force_insert("ArraySize", resource.ArraySize);
			node.force_insert("Dimension", resource.Dimension);
			node.force_insert("Stage", resource.Stage);
			return node;
		}

		static bool decode(const Node& node, Shark::ShaderResource::Image& outResource)
		{
			if (!node.IsMap() || node.size() != 5)
				return false;
			
			YAML::Read(node, "Name", outResource.Name);
			YAML::Read(node, "Slot", outResource.Slot);
			YAML::Read(node, "ArraySize", outResource.ArraySize);
			YAML::Read(node, "Dimension", outResource.Dimension);
			YAML::Read(node, "Stage", outResource.Stage);
			return true;
		}
	};

	template<>
	struct convert<Shark::ShaderResource::Sampler>
	{
		static Node encode(const Shark::ShaderResource::Sampler& resource)
		{
			Node node(NodeType::Map);
			node.force_insert("Name", resource.Name);
			node.force_insert("Slot", resource.Slot);
			node.force_insert("ArraySize", resource.ArraySize);
			node.force_insert("Stage", resource.Stage);
			return node;
		}

		static bool decode(const Node& node, Shark::ShaderResource::Sampler& outResource)
		{
			if (!node.IsMap() || node.size() != 4)
				return false;
			
			outResource.Name       = node["Name"].as<std::string>();
			outResource.Slot       = node["Slot"].as<uint32_t>();
			outResource.ArraySize  = node["ArraySize"].as<uint32_t>();
			outResource.Stage      = node["Stage"].as<nvrhi::ShaderType>();
			return true;
		}
	};

	template<>
	struct convert<Shark::ShaderResource::PushConstant>
	{
		static Node encode(const Shark::ShaderResource::PushConstant& resource)
		{
			Node node(NodeType::Map);
			node.force_insert("Slot", resource.Slot);
			node.force_insert("StructSize", resource.StructSize);
			node.force_insert("Stage", resource.Stage);
			return node;
		}

		static bool decode(const Node& node, Shark::ShaderResource::PushConstant& outResource)
		{
			if (!node.IsMap() || node.size() != 3)
				return false;
			
			outResource.StructSize = node["StructSize"].as<uint32_t>();
			//outResource.Slot       = node["Slot"].as<uint32_t>();
			outResource.Stage      = node["Stage"].as<nvrhi::ShaderType>();
			return true;
		}
	};

	template<>
	struct convert<Shark::ShaderInputInfo>
	{
		static Node encode(const Shark::ShaderInputInfo& info)
		{
			Node node(NodeType::Map);
			node.force_insert("Name", info.Name);
			node.force_insert("Set", info.Set);
			node.force_insert("Slot", info.Slot);
			node.force_insert("Count", info.Count);
			node.force_insert("GraphicsType", info.GraphicsType);
			node.force_insert("Type", info.Type);
			return node;
		}

		static bool decode(const Node& node, Shark::ShaderInputInfo& outInfo)
		{
			if (!node.IsMap() || node.size() != 6)
				return false;

			YAML::Read(node, "Name", outInfo.Name);
			YAML::Read(node, "Set", outInfo.Set);
			YAML::Read(node, "Slot", outInfo.Slot);
			YAML::Read(node, "Count", outInfo.Count);
			YAML::Read(node, "GraphicsType", outInfo.GraphicsType);
			YAML::Read(node, "Type", outInfo.Type);
			return true;
		}
	};

	template<>
	struct convert<Shark::D3D11BindingSetOffsets>
	{
		static Node encode(const Shark::D3D11BindingSetOffsets& offsets)
		{
			Node node(NodeType::Map);
			node.force_insert("ShaderResource", offsets.ShaderResource);
			node.force_insert("Sampler", offsets.Sampler);
			node.force_insert("ConstantBuffer", offsets.ConstantBuffer);
			node.force_insert("UnorderedAccess", offsets.UnorderedAccess);
			return node;
		}

		static bool decode(const Node& node, Shark::D3D11BindingSetOffsets& outOffsets)
		{
			if (!node.IsMap() || node.size() != 4)
				return false;

			YAML::Read(node, "ShaderResource", outOffsets.ShaderResource);
			YAML::Read(node, "Sampler", outOffsets.Sampler);
			YAML::Read(node, "ConstantBuffer", outOffsets.ConstantBuffer);
			YAML::Read(node, "UnorderedAccess", outOffsets.UnorderedAccess);
			return true;
		}
	};

	template<>
	struct convert<Shark::ShaderBindingLayout>
	{
		static Node encode(const Shark::ShaderBindingLayout& layout)
		{
			Node node(NodeType::Map);
			node.force_insert("ConstantBuffers", layout.ConstantBuffers);
			node.force_insert("StorageBuffers", layout.StorageBuffers);
			node.force_insert("Images", layout.Images);
			node.force_insert("StorageImages", layout.StorageImages);
			node.force_insert("Samplers", layout.Samplers);
			
			node.force_insert("Stage", layout.Stage);
			node.force_insert("InputInfos", layout.InputInfos);
			node.force_insert("BindingOffsets", layout.BindingOffsets);
			return node;
		}

		static bool decode(const Node& node, Shark::ShaderBindingLayout& layout)
		{
			if (!node.IsMap() || node.size() != 8)
				return false;

			YAML::Read(node, "ConstantBuffers", layout.ConstantBuffers);
			YAML::Read(node, "StorageBuffers", layout.StorageBuffers);
			YAML::Read(node, "Images", layout.Images);
			YAML::Read(node, "StorageImages", layout.StorageImages);
			YAML::Read(node, "Samplers", layout.Samplers);
			YAML::Read(node, "Stage", layout.Stage);
			YAML::Read(node, "InputInfos", layout.InputInfos);
			YAML::Read(node, "BindingOffsets", layout.BindingOffsets);
			return true;
		}
	};

}

template <>
struct ::magic_enum::customize::enum_range<nvrhi::ShaderType> {
	static constexpr bool is_flags = true;
};
