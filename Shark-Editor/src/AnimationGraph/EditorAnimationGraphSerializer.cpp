#include "skpch.h"
#include "EditorAnimationGraphSerializer.h"

#include "Shark/Asset/AssetThread/AssetLoadContext.h"
#include "Shark/Serialization/MeshSerializers.h"
#include "Shark/Serialization/SerializerUtilities.h"
#include "Shark/Serialization/YAML.h"

#include "Shark/File/Serialization/StreamReader.h"
#include "Shark/File/Serialization/StreamWriter.h"
#include "Shark/File/Serialization/FileStream.h"
#include "Shark/File/Serialization/StringStream.h"
#include "Shark/Debug/Profiler.h"

#include "Shark/NodeGraph/Prototype.h"

#include "NodeGraph/EditorNodes.h"
#include "AnimationGraph/EditorAnimationGraphAsset.h"
#include "AnimationGraph/AnimationGraphContext.h"

#include <choc/containers/choc_Value.h>
#include <choc/text/choc_JSON.h>

namespace NG = ::Shark::NodeGraph;
namespace NGE = ::Shark::NodeGraph::Editor;


template<>
struct YAML::convert<NGE::Node>
{
	static Node encode(const NGE::Node& editorNode)
	{
		Node node(NodeType::Map);
		node.force_insert("ID",                   Shark::UUID::Make(editorNode.ID.Get()));
		node.force_insert("Name",                 editorNode.Name);
		node.force_insert("Category",             editorNode.Category);
		node.force_insert("Settings.IsSimple",    editorNode.Settings.IsSimple);
		node.force_insert("Settings.CanEditPins", editorNode.Settings.CanEditPins);
		node.force_insert("State",                editorNode.State);
		node.force_insert("Inputs",               editorNode.Inputs);
		node.force_insert("Outputs",              editorNode.Outputs);
		return node;
	}

	static bool decode(const Node& node, NGE::Node& editorNode)
	{
		if (!node.IsMap() || node.size() != 8)
			return false;

		YAML::Read(node, "ID",                   [&editorNode](Shark::UUID id) { editorNode.ID = id.Value(); });
		YAML::Read(node, "Name",                 editorNode.Name);
		YAML::Read(node, "Category",             editorNode.Category);
		YAML::Read(node, "Settings.IsSimple",    editorNode.Settings.IsSimple);
		YAML::Read(node, "Settings.CanEditPins", editorNode.Settings.CanEditPins);
		YAML::Read(node, "State",                editorNode.State);
		YAML::Read(node, "Inputs",               editorNode.Inputs);
		YAML::Read(node, "Outputs",              editorNode.Outputs);
		editorNode.Initialize();
		return true;
	}
};

template<>
struct YAML::convert<NGE::Link>
{
	static Node encode(const NGE::Link& editorLink)
	{
		Node node(NodeType::Map);
		node.force_insert("ID",         Shark::UUID::Make(editorLink.ID.Get()));
		node.force_insert("StartPinID", Shark::UUID::Make(editorLink.StartPinID.Get()));
		node.force_insert("EndPinID",   Shark::UUID::Make(editorLink.EndPinID.Get()));
		node.force_insert("Color",      glm::vec4{ editorLink.Color.Value.x, editorLink.Color.Value.y, editorLink.Color.Value.z, editorLink.Color.Value.w });
		return node;
	}

	static bool decode(const Node& node, NGE::Link& editorLink)
	{
		if (!node.IsMap() || node.size() != 4)
			return false;

		YAML::Read(node, "ID",         [&editorLink](Shark::UUID id)  { editorLink.ID = id.Value(); });
		YAML::Read(node, "StartPinID", [&editorLink](Shark::UUID id)  { editorLink.StartPinID = id.Value(); });
		YAML::Read(node, "EndPinID",   [&editorLink](Shark::UUID id)  { editorLink.EndPinID = id.Value(); });
		YAML::Read(node, "Color",      [&editorLink](glm::vec4 color) { editorLink.Color = { color.x, color.y, color.z, color.w }; });
		return true;
	}
};

template<>
struct YAML::convert<NGE::Pin>
{
	static Node encode(const NGE::Pin& editorPin)
	{
		Node node(NodeType::Map);
		node.force_insert("ID",         Shark::UUID::Make(editorPin.ID.Get()));
		node.force_insert("Name",       editorPin.Name);
		node.force_insert("Identifier", editorPin.Identifier.ID);
		node.force_insert("Value",      editorPin.Value);
		node.force_insert("PinType",    editorPin.PinType);
		node.force_insert("Color",      glm::vec4{ editorPin.Color.Value.x, editorPin.Color.Value.y, editorPin.Color.Value.z, editorPin.Color.Value.w });
		return node;
	}

	static bool decode(const Node& node, NGE::Pin& editorPin)
	{
		if (!node.IsMap() || node.size() != 6)
			return false;

		YAML::Read(node, "ID",         [&editorPin](Shark::UUID id) { editorPin.ID = id.Value(); });
		YAML::Read(node, "Name",       editorPin.Name);
		YAML::Read(node, "Identifier", editorPin.Identifier.ID);
		YAML::Read(node, "Value",      editorPin.Value);
		YAML::Read(node, "PinType",    editorPin.PinType);
		YAML::Read(node, "Color",      [&editorPin](glm::vec4 color) { editorPin.Color = { color.x, color.y, color.z, color.w }; });
		return true;
	}
};

template<>
struct YAML::convert<choc::value::Value>
{
	static Node encode(const choc::value::Value& value)
	{
		auto output = value.serialise();
		return Node(Binary(output.data.data(), output.data.size()));
	}

	static bool decode(const Node& node, choc::value::Value& value)
	{
		if (!node.IsScalar())
			return false;

		auto binary = node.as<Binary>(Binary{});
		if (!binary.size())
			return true;

		choc::value::InputData data = {
			.start = reinterpret_cast<const uint8_t*>(binary.data()),
			.end   = reinterpret_cast<const uint8_t*>(binary.data() + binary.size())
		};

		TRY_ELSE(
			value = choc::value::Value::deserialise(data),
			return true
		);

		return true;
	}
};

template<>
struct YAML::convert<choc::value::ValueView>
{
	static Node encode(const choc::value::ValueView& value)
	{
		if (!value.isObject())
			return Node(choc::json::toString(value));

		auto object = choc::value::createObject("Object");
		object.addMember(value.getObjectClassName(), value);

		return Node(choc::json::toString(object));
	}
};

namespace Shark {

	bool EditorAnimationGraphSerializer::Serialize(Ref<Asset> asset, const AssetMetaData& metadata)
	{
		SK_PROFILE_FUNCTION();

		StringStreamWriter stream;

		if (!SerializeToYAML(asset.As<EditorAnimationGraphAsset>(), &stream))
		{
			SK_CORE_ERROR_TAG("Serialization", "Failed to serialize YAML!");
			return false;
		}

		stream.WriteToDisc(Utilities::GetAssetFilesystemPath(metadata));
		return true;
	}

	bool EditorAnimationGraphSerializer::TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata, AssetLoadContext* context)
	{
		SK_PROFILE_FUNCTION();

		auto filesystemPath = context->GetFilesystemPath(metadata);
		if (!Utilities::ValidateYamlAssetFile(filesystemPath, metadata, context))
		{
			context->SetErrorFallback(Ref<EditorAnimationGraphAsset>::Create());
			return false;
		}

		Ref<EditorAnimationGraphAsset> animationGraph;
		FileStreamReader stream(filesystemPath);

		if (!DeserializeFromYAML(animationGraph, &stream, context))
			return false;

		if (!animationGraph->Prototype)
		{
			NodeGraph::Editor::AnimationGraphContext graphContext(animationGraph, false);
			animationGraph->Prototype = graphContext.CreatePrototype();
		}

		asset = animationGraph;
		asset->Handle = metadata.Handle;
		return true;
	}

	bool EditorAnimationGraphSerializer::SerializeToYAML(Ref<EditorAnimationGraphAsset> animationGraph, StreamWriter* stream)
	{
		YAML::Emitter out(stream->GetStream());

		out << YAML::BeginMap;
		out << YAML::Key << "AnimationGraph";
		out << YAML::BeginMap;
		AnimationGraphSerializer::SerializeGraphToYAML(animationGraph, out);

		out << YAML::Key << "Graph";
		out << YAML::BeginMap;
		out << YAML::Key << "Nodes" << YAML::Value << animationGraph->m_Nodes;
		out << YAML::Key << "Links" << YAML::Value << animationGraph->m_Links;
		out << YAML::Key << "Inputs" << YAML::Value;
		out << YAML::BeginMap;
		for (auto& name : animationGraph->m_InputProperties.GetNames())
		{
			auto value = animationGraph->m_InputProperties.GetValue(name);
			out << YAML::Key << name << YAML::Value << value;
		}
		out << YAML::EndMap;
		out << YAML::Key << "State" << YAML::Value << animationGraph->m_GraphState;
		out << YAML::EndMap;

		out << YAML::EndMap;
		return true;
	}

	bool EditorAnimationGraphSerializer::DeserializeFromYAML(Ref<EditorAnimationGraphAsset>& animationGraph, StreamReader* stream, AssetLoadContext* context)
	{
		YAML::Node rootNode = YAML::Load(stream->GetStream());
		if (!rootNode["AnimationGraph"])
		{
			context->AddError(AssetLoadError::InvalidYAML, "Root node 'AnimationGraph' is missing");
			context->SetErrorFallback(Ref<EditorAnimationGraphAsset>::Create());
			return false;
		}

		animationGraph = Ref<EditorAnimationGraphAsset>::Create();
		AnimationGraphSerializer::DeserializeGraphFromYAML(animationGraph, rootNode["AnimationGraph"], context);

		YAML::Node graphNode;
		if (!(graphNode = rootNode["AnimationGraph"]["Graph"]))
		{
			// #TODO #assets add warning
			// #NOTE: return true is correct at the moment
			return true;
		}

		YAML::DeserializeProperty(graphNode, "Nodes", animationGraph->m_Nodes);
		YAML::DeserializeProperty(graphNode, "Links", animationGraph->m_Links);

		for (auto input : graphNode["Inputs"])
		{
			auto& name = input.first.Scalar();
			auto& data = input.second.Scalar();

			animationGraph->m_InputProperties.Set(name, choc::json::parseValue(data));
		}

		YAML::DeserializeProperty(graphNode, "State", animationGraph->m_GraphState);

		return true;
	}

}
