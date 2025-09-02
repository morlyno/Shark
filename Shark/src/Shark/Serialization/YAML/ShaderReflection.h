#pragma once

#include "Shark/Render/ShaderReflection.h"
#include "Shark/Serialization/YAML.h"

namespace YAML {

	template<>
	struct convert<Shark::ShaderReflection::MemberDeclaration>
	{
		static Node encode(const Shark::ShaderReflection::MemberDeclaration& memberDeclaration)
		{
			Node node(NodeType::Map);
			node.force_insert("Name", memberDeclaration.Name);
			node.force_insert("Type", memberDeclaration.Type);
			node.force_insert("Size", memberDeclaration.Size);
			node.force_insert("Offset", memberDeclaration.Offset);
			return node;
		}

		static bool decode(const Node& node, Shark::ShaderReflection::MemberDeclaration& outMemberDeclaration)
		{
			if (!node.IsMap() || node.size() != 4)
				return false;

			SK_DESERIALIZE_PROPERTY(node, "Name", outMemberDeclaration.Name, "");
			SK_DESERIALIZE_PROPERTY(node, "Type", outMemberDeclaration.Type, Shark::ShaderReflection::VariableType::None);
			SK_DESERIALIZE_PROPERTY(node, "Size", outMemberDeclaration.Size, 0);
			SK_DESERIALIZE_PROPERTY(node, "Offset", outMemberDeclaration.Offset, 0);
			return true;
		}
	};

	template<>
	struct convert<Shark::ShaderReflection::Resource>
	{
		static Node encode(const Shark::ShaderReflection::Resource& resource)
		{
			Node node(NodeType::Map);
			node.force_insert("Name", resource.Name);
			node.force_insert("Set", resource.Set);
			node.force_insert("Binding", resource.Binding);
			node.force_insert("Stage", resource.Stage);
			node.force_insert("Type", resource.Type);
			node.force_insert("ArraySize", resource.ArraySize);
			node.force_insert("StructSize", resource.StructSize);

			node.force_insert("DXBinding", resource.DXBinding);
			node.force_insert("DXSamplerBinding", resource.DXSamplerBinding);
			return node;
		}

		static bool decode(const Node& node, Shark::ShaderReflection::Resource& outResource)
		{
			if (!node.IsMap() || node.size() != 9)
				return false;

			SK_DESERIALIZE_PROPERTY(node, "Name", outResource.Name);
			SK_DESERIALIZE_PROPERTY(node, "Set", outResource.Set);
			SK_DESERIALIZE_PROPERTY(node, "Binding", outResource.Binding);
			SK_DESERIALIZE_PROPERTY(node, "Stage", outResource.Stage);
			SK_DESERIALIZE_PROPERTY(node, "Type", outResource.Type);
			SK_DESERIALIZE_PROPERTY(node, "ArraySize", outResource.ArraySize);
			SK_DESERIALIZE_PROPERTY(node, "StructSize", outResource.StructSize);
			SK_DESERIALIZE_PROPERTY(node, "DXBinding", outResource.DXBinding);
			SK_DESERIALIZE_PROPERTY(node, "DXSamplerBinding", outResource.DXSamplerBinding);
			return true;
		}
	};
}
