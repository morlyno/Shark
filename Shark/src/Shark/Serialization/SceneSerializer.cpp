#include "skpch.h"
#include "SceneSerializer.h"

#include "Shark/Asset/AssetManager.h"
#include "Shark/Scene/Scene.h"
#include "Shark/Scene/Entity.h"
#include "Shark/Scripting/ScriptEngine.h"

#include "Shark/Serialization/SerializationMacros.h"

#include "Shark/File/FileSystem.h"
#include "Shark/Utils/YAMLUtils.h"

#include "Shark/Debug/enttDebug.h"
#include "Shark/Debug/Profiler.h"

#include <yaml-cpp/yaml.h>
#include <magic_enum.hpp>

#if SK_DEBUG
#define SK_SERIALIZATION_ERROR(...) SK_CORE_ERROR_TAG("Serialization", __VA_ARGS__); SK_DEBUG_BREAK()
#else
#define SK_SERIALIZATION_ERROR(...) SK_CORE_ERROR_TAG("Serialization", __VA_ARGS__)
#endif

namespace YAML {

	template<>
	struct convert<Shark::Ref<Shark::FieldStorage>>
	{
		static Node encode(const Shark::Ref<Shark::FieldStorage>& storage)
		{
			Node node(NodeType::Map);
			node.force_insert("Name", storage->Name);
			node.force_insert("Type", Shark::ToString(storage->Type));
			switch (storage->Type)
			{
				case Shark::ManagedFieldType::None: break;
				case Shark::ManagedFieldType::Bool: node.force_insert("Value", storage->GetValue<bool>()); break;
				case Shark::ManagedFieldType::Char: node.force_insert("Value", storage->GetValue<char>()); break;
				case Shark::ManagedFieldType::Byte: node.force_insert("Value", storage->GetValue<uint8_t>()); break;
				case Shark::ManagedFieldType::SByte: node.force_insert("Value", storage->GetValue<int8_t>()); break;
				case Shark::ManagedFieldType::Short: node.force_insert("Value", storage->GetValue<int16_t>()); break;
				case Shark::ManagedFieldType::UShort: node.force_insert("Value", storage->GetValue<uint16_t>()); break;
				case Shark::ManagedFieldType::Int: node.force_insert("Value", storage->GetValue<int32_t>()); break;
				case Shark::ManagedFieldType::UInt: node.force_insert("Value", storage->GetValue<uint32_t>()); break;
				case Shark::ManagedFieldType::Long: node.force_insert("Value", storage->GetValue<int64_t>()); break;
				case Shark::ManagedFieldType::ULong: node.force_insert("Value", storage->GetValue<uint64_t>()); break;
				case Shark::ManagedFieldType::Float: node.force_insert("Value", storage->GetValue<float>()); break;
				case Shark::ManagedFieldType::Double: node.force_insert("Value", storage->GetValue<double>()); break;
				case Shark::ManagedFieldType::String: node.force_insert("Value", storage->GetValue<std::string>()); break;
				case Shark::ManagedFieldType::Entity: node.force_insert("Value", storage->GetValue<Shark::UUID>()); break;
				case Shark::ManagedFieldType::Component: node.force_insert("Value", storage->GetValue<Shark::UUID>()); break;
				case Shark::ManagedFieldType::Vector2: node.force_insert("Value", storage->GetValue<glm::vec2>()); break;
				case Shark::ManagedFieldType::Vector3: node.force_insert("Value", storage->GetValue<glm::vec3>()); break;
				case Shark::ManagedFieldType::Vector4: node.force_insert("Value", storage->GetValue<glm::vec4>()); break;
				case Shark::ManagedFieldType::AssetHandle: node.force_insert("Value", storage->GetValue<Shark::AssetHandle>()); break;
				default: SK_CORE_ASSERT(false, "Unkown ManagedFieldType"); break;
			}
			return node;
		}

		static bool decode(const Node& node, Shark::Ref<Shark::FieldStorage>& storage)
		{
			if (!node.IsMap() || node.size() != 3)
				return false;

			storage = Shark::Ref<Shark::FieldStorage>::Create();
			storage->Name = node["Name"].as<std::string>();
			storage->Type = Shark::StringToManagedFieldType(node["Type"].as<std::string>());
			switch (storage->Type)
			{
				case Shark::ManagedFieldType::None: break;
				case Shark::ManagedFieldType::Bool: storage->SetValue(node["Value"].as<bool>()); break;
				case Shark::ManagedFieldType::Char: storage->SetValue(node["Value"].as<char>()); break;
				case Shark::ManagedFieldType::Byte: storage->SetValue(node["Value"].as<uint8_t>()); break;
				case Shark::ManagedFieldType::SByte: storage->SetValue(node["Value"].as<int8_t>()); break;
				case Shark::ManagedFieldType::Short: storage->SetValue(node["Value"].as<int16_t>()); break;
				case Shark::ManagedFieldType::UShort: storage->SetValue(node["Value"].as<uint16_t>()); break;
				case Shark::ManagedFieldType::Int: storage->SetValue(node["Value"].as<int32_t>()); break;
				case Shark::ManagedFieldType::UInt: storage->SetValue(node["Value"].as<uint32_t>()); break;
				case Shark::ManagedFieldType::Long: storage->SetValue(node["Value"].as<int64_t>()); break;
				case Shark::ManagedFieldType::ULong: storage->SetValue(node["Value"].as<uint64_t>()); break;
				case Shark::ManagedFieldType::Float: storage->SetValue(node["Value"].as<float>()); break;
				case Shark::ManagedFieldType::Double: storage->SetValue(node["Value"].as<double>()); break;
				case Shark::ManagedFieldType::String: storage->SetValue(node["Value"].as<std::string>()); break;
				case Shark::ManagedFieldType::Entity: storage->SetValue(node["Value"].as<Shark::UUID>()); break;
				case Shark::ManagedFieldType::Component: storage->SetValue(node["Value"].as<Shark::UUID>()); break;
				case Shark::ManagedFieldType::Vector2: storage->SetValue(node["Value"].as<glm::vec2>()); break;
				case Shark::ManagedFieldType::Vector3: storage->SetValue(node["Value"].as<glm::vec3>()); break;
				case Shark::ManagedFieldType::Vector4: storage->SetValue(node["Value"].as<glm::vec4>()); break;
				case Shark::ManagedFieldType::AssetHandle: storage->SetValue(node["Value"].as<Shark::AssetHandle>()); break;
				default: SK_CORE_ASSERT(false, "Unkown ManagedFieldType"); return false;
			}

			return true;
		}
	};


	template<typename TEnum>
		requires std::is_enum_v<TEnum>
	struct convert<TEnum>
	{
		static Node encode(const TEnum& value)
		{
			return Node(magic_enum::enum_name(value));
		}

		static bool decode(const Node& node, TEnum& value)
		{
			if (!node.IsScalar())
				return false;

			std::optional<TEnum> optValue = magic_enum::enum_cast<TEnum>(node.Scalar());
			if (!optValue.has_value())
				return false;

			value = optValue.value();
			return true;
		}
	};

}

namespace Shark {

	bool SceneSerializer::Serialize(Ref<Asset> asset, const AssetMetaData& metadata)
	{
		SK_PROFILE_FUNCTION();

		SK_CORE_VERIFY(asset);
		SK_CORE_INFO_TAG("Serialization", "Serializing Scene to {}", metadata.FilePath);
		Timer timer;

#if 0
		if (!ResourceManager::HasExistingFilePath(metadata))
		{
			SK_SERIALIZATION_ERROR("Path not found! {0}", metadata.FilePath);
			return false;
		}
#endif

		std::string result = SerializeToYAML(asset.As<Scene>());
		if (result.empty())
		{
			SK_SERIALIZATION_ERROR("YAML result was empty!");
			return false;
		}

		std::ofstream fout(Project::GetActiveEditorAssetManager()->GetFilesystemPath(metadata));
		SK_CORE_ASSERT(fout);

		fout << result;
		fout.close();

		SK_CORE_INFO_TAG("Serialization", "Serializing Scene took {}", timer.Elapsed());
		return true;
	}

	bool SceneSerializer::TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata)
	{
		SK_PROFILE_FUNCTION();

		SK_CORE_INFO_TAG("Serialization", "Deserializing Scene from {}", metadata.FilePath);
		Timer timer;

		if (!Project::GetActiveEditorAssetManager()->HasExistingFilePath(metadata))
		{
			SK_SERIALIZATION_ERROR("Path not found! {0}", metadata.FilePath);
			return false;
		}

		std::string filedata = FileSystem::ReadString(Project::GetActiveEditorAssetManager()->GetFilesystemPath(metadata));
		if (filedata.empty())
		{
			SK_SERIALIZATION_ERROR("File was empty!");
			return false;
		}

		Ref<Scene> scene = Ref<Scene>::Create();
		if (!DeserializeFromYAML(scene, filedata))
		{
			SK_SERIALIZATION_ERROR("Failed to deserialize Scene! {}", m_ErrorMsg);
			return false;
		}

		scene->m_Registry.sort<IDComponent>([](const entt::entity lhs, const entt::entity rhs) {
			return entt::registry::entity_type(lhs) < entt::registry::entity_type(rhs);
		});

		asset = scene;
		asset->Handle = metadata.Handle;

		SK_CORE_INFO_TAG("Serialization", "Deserializing Scene took {}", timer.Elapsed());
		return true;
	}

	std::string SceneSerializer::SerializeToYAML(Ref<Scene> scene)
	{
		SK_PROFILE_FUNCTION();

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Scene" /*<< YAML::Value << scene->GetName()*/;

		out << YAML::BeginMap;
		out << YAML::Key << "Name" << YAML::Value << scene->GetName();
		SK_CORE_TRACE_TAG("Serialization", " - Scene Name: {}", scene->GetName());

		out << YAML::Key << "ActiveCamera" << YAML::Value << YAML::Hex << scene->GetActiveCameraUUID() << YAML::Dec;
		SK_CORE_TRACE_TAG("Serialization", " - Active Camera: {}", scene->GetActiveCameraUUID());

		out << YAML::Key << "Entities" << YAML::Value;
		out << YAML::BeginSeq;

		std::vector<Entity> entities = scene->GetEntitiesSorted();
		for (Entity entity : entities)
		{
			out << YAML::BeginMap;
			out << YAML::Key << "Entity" << YAML::Value << entity.GetUUID();

			if (auto component = entity.TryGetComponent<TagComponent>())
			{
				out << YAML::Key << "TagComponent";
				out << YAML::BeginMap;
				out << YAML::Key << "Name" << YAML::Value << component->Tag;
				out << YAML::EndMap;
			}

			SK_CORE_TRACE_TAG("Serialization", " - Entity {} - {}", entity.GetName(), entity.GetUUID());

			if (auto component = entity.TryGetComponent<TransformComponent>())
			{
				out << YAML::Key << "TransformComponent";
				out << YAML::BeginMap;
				out << YAML::Key << "Translation" << YAML::Value << component->Translation;
				out << YAML::Key << "Rotation" << YAML::Value << component->Rotation;
				out << YAML::Key << "Scale" << YAML::Value << component->Scale;
				out << YAML::EndMap;
			}

			if (auto component = entity.TryGetComponent<RelationshipComponent>())
			{
				out << YAML::Key << "RelationshipComponent";
				out << YAML::BeginMap;
				out << YAML::Key << "Parent" << YAML::Value << component->Parent;
				out << YAML::Key << "Children" << YAML::Value << component->Children;
				out << YAML::EndMap;
			}

			if (auto component = entity.TryGetComponent<SpriteRendererComponent>())
			{
				out << YAML::Key << "SpriteRendererComponent";
				out << YAML::BeginMap;
				out << YAML::Key << "Color" << YAML::Value << component->Color;
				out << YAML::Key << "TextureHandle" << YAML::Value << component->TextureHandle;
				out << YAML::Key << "TilingFactor" << YAML::Value << component->TilingFactor;
				out << YAML::EndMap;
			}

			if (auto component = entity.TryGetComponent<CircleRendererComponent>())
			{
				out << YAML::Key << "CircleRendererComponent";
				out << YAML::BeginMap;
				out << YAML::Key << "Color" << YAML::Value << component->Color;
				out << YAML::Key << "Thickness" << YAML::Value << component->Thickness;
				out << YAML::Key << "Fade" << YAML::Value << component->Fade;
				out << YAML::EndMap;
			}

			if (auto component = entity.TryGetComponent<TextRendererComponent>())
			{
				out << YAML::Key << "TextRendererComponent";
				out << YAML::BeginMap;
				out << YAML::Key << "FontHandle" << YAML::Value << component->FontHandle;
				out << YAML::Key << "Text" << YAML::Value << component->Text;
				out << YAML::Key << "Color" << YAML::Value << component->Color;
				out << YAML::Key << "Kerning" << YAML::Value << component->Kerning;
				out << YAML::EndMap;
			}

			if (auto component = entity.TryGetComponent<MeshComponent>())
			{
				AssetHandle meshHandle = AssetHandle::Invalid;
				if (!AssetManager::IsMemoryAsset(component->Mesh))
					meshHandle = component->Mesh;

				out << YAML::Key << "MeshComponent";
				out << YAML::BeginMap;
				out << YAML::Key << "Mesh" << YAML::Value << meshHandle;
				out << YAML::Key << "SubmeshIndex" << YAML::Value << component->SubmeshIndex;
				out << YAML::Key << "Material" << YAML::Value << component->Material;
				out << YAML::EndMap;
			}

			if (auto component = entity.TryGetComponent<PointLightComponent>())
			{
				out << YAML::Key << "PointLightComponent";
				out << YAML::BeginMap;
				out << YAML::Key << "Radiance" << YAML::Value << component->Radiance;
				out << YAML::Key << "Intensity" << YAML::Value << component->Intensity;
				out << YAML::Key << "Radius" << YAML::Value << component->Radius;
				out << YAML::Key << "Falloff" << YAML::Value << component->Falloff;
				out << YAML::EndMap;
			}
			
			if (auto component = entity.TryGetComponent<DirectionalLightComponent>())
			{
				out << YAML::Key << "DirectionalLightComponent";
				out << YAML::BeginMap;
				out << YAML::Key << "Radiance" << YAML::Value << component->Radiance;
				out << YAML::Key << "Intensity" << YAML::Value << component->Intensity;
				out << YAML::EndMap;
			}

			if (auto component = entity.TryGetComponent<SkyComponent>())
			{
				out << YAML::Key << "SkyComponent";
				out << YAML::BeginMap;
				out << YAML::Key << "SceneEnvironment" << YAML::Value << component->SceneEnvironment;
				out << YAML::Key << "Intensity" << YAML::Value << component->Intensity;
				out << YAML::Key << "Lod" << YAML::Value << component->Lod;
				out << YAML::EndMap;
			}

			if (auto component = entity.TryGetComponent<CameraComponent>())
			{
				out << YAML::Key << "CameraComponent";
				out << YAML::BeginMap;
				out << YAML::Key << "IsPerspective" << YAML::Value << component->IsPerspective;
				out << YAML::Key << "PerspectiveFOV" << YAML::Value << component->PerspectiveFOV * Math::Rad2Deg;
				out << YAML::Key << "OrthographicSize" << YAML::Value << component->OrthographicSize;
				out << YAML::Key << "Near" << YAML::Value << component->Near;
				out << YAML::Key << "Far" << YAML::Value << component->Far;
				out << YAML::EndMap;
			}

			if (auto component = entity.TryGetComponent<RigidBody2DComponent>())
			{
				out << YAML::Key << "RigidBody2DComponent";
				out << YAML::BeginMap;
				out << YAML::Key << "Type" << YAML::Value << YAML::Node(component->Type);
				out << YAML::Key << "FixedRotation" << YAML::Value << component->FixedRotation;
				out << YAML::Key << "IsBullet" << YAML::Value << component->IsBullet;
				out << YAML::Key << "Awake" << YAML::Value << component->Awake;
				out << YAML::Key << "Enabled" << YAML::Value << component->Enabled;
				out << YAML::Key << "AllowSleep" << YAML::Value << component->AllowSleep;
				out << YAML::Key << "GravityScale" << YAML::Value << component->GravityScale;
				out << YAML::EndMap;
			}

			if (auto component = entity.TryGetComponent<BoxCollider2DComponent>())
			{
				out << YAML::Key << "BoxCollider2DComponent";
				out << YAML::BeginMap;
				out << YAML::Key << "Size" << YAML::Value << component->Size;
				out << YAML::Key << "Offset" << YAML::Value << component->Offset;
				out << YAML::Key << "Rotation" << YAML::Value << component->Rotation;
				out << YAML::Key << "Density" << YAML::Value << component->Density;
				out << YAML::Key << "Friction" << YAML::Value << component->Friction;
				out << YAML::Key << "Restitution" << YAML::Value << component->Restitution;
				out << YAML::Key << "RestitutionThreshold" << YAML::Value << component->RestitutionThreshold;
				out << YAML::Key << "IsSensor" << YAML::Value << component->IsSensor;
				out << YAML::EndMap;
			}

			if (auto component = entity.TryGetComponent<CircleCollider2DComponent>())
			{
				out << YAML::Key << "CircleCollider2DComponent";
				out << YAML::BeginMap;
				out << YAML::Key << "Radius" << YAML::Value << component->Radius;
				out << YAML::Key << "Offset" << YAML::Value << component->Offset;
				out << YAML::Key << "Rotation" << YAML::Value << component->Rotation;
				out << YAML::Key << "Density" << YAML::Value << component->Density;
				out << YAML::Key << "Friction" << YAML::Value << component->Friction;
				out << YAML::Key << "Restitution" << YAML::Value << component->Restitution;
				out << YAML::Key << "RestitutionThreshold" << YAML::Value << component->RestitutionThreshold;
				out << YAML::Key << "IsSensor" << YAML::Value << component->IsSensor;
				out << YAML::EndMap;
			}

			if (auto component = entity.TryGetComponent<DistanceJointComponent>())
			{
				out << YAML::Key << "DistanceJointComponent";
				out << YAML::BeginMap;
				out << YAML::Key << "ConnectedEntity" << YAML::Value << component->ConnectedEntity;
				out << YAML::Key << "CollideConnected" << YAML::Value << component->CollideConnected;
				out << YAML::Key << "AnchorOffsetA" << YAML::Value << component->AnchorOffsetA;
				out << YAML::Key << "AnchorOffsetB" << YAML::Value << component->AnchorOffsetB;
				out << YAML::Key << "MinLength" << YAML::Value << component->MinLength;
				out << YAML::Key << "MaxLength" << YAML::Value << component->MaxLength;
				out << YAML::Key << "Stiffness" << YAML::Value << component->Stiffness;
				out << YAML::Key << "Damping" << YAML::Value << component->Damping;
				out << YAML::EndMap;
			}

			if (auto component = entity.TryGetComponent<HingeJointComponent>())
			{
				out << YAML::Key << "HingeJointComponent";
				out << YAML::BeginMap;
				out << YAML::Key << "ConnectedEntity" << YAML::Value << component->ConnectedEntity;
				out << YAML::Key << "CollideConnected" << YAML::Value << component->CollideConnected;
				out << YAML::Key << "Anchor" << YAML::Value << component->Anchor;
				out << YAML::Key << "LowerAngle" << YAML::Value << component->LowerAngle;
				out << YAML::Key << "UpperAngle" << YAML::Value << component->UpperAngle;
				out << YAML::Key << "EnableMotor" << YAML::Value << component->EnableMotor;
				out << YAML::Key << "MotorSpeed" << YAML::Value << component->MotorSpeed;
				out << YAML::Key << "MaxMotorTorque" << YAML::Value << component->MaxMotorTorque;
				out << YAML::EndMap;
			}

			if (auto component = entity.TryGetComponent<PrismaticJointComponent>())
			{
				out << YAML::Key << "PrismaticJointComponent";
				out << YAML::BeginMap;
				out << YAML::Key << "ConnectedEntity" << YAML::Value << component->ConnectedEntity;
				out << YAML::Key << "CollideConnected" << YAML::Value << component->CollideConnected;
				out << YAML::Key << "Anchor" << YAML::Value << component->Anchor;
				out << YAML::Key << "Axis" << YAML::Value << component->Axis;
				out << YAML::Key << "EnableLimit" << YAML::Value << component->EnableLimit;
				out << YAML::Key << "LowerTranslation" << YAML::Value << component->LowerTranslation;
				out << YAML::Key << "UpperTranslation" << YAML::Value << component->UpperTranslation;
				out << YAML::Key << "EnableMotor" << YAML::Value << component->EnableMotor;
				out << YAML::Key << "MotorSpeed" << YAML::Value << component->MotorSpeed;
				out << YAML::Key << "MaxMotorForce" << YAML::Value << component->MaxMotorForce;
				out << YAML::EndMap;
			}

			if (auto component = entity.TryGetComponent<PulleyJointComponent>())
			{
				out << YAML::Key << "PulleyJointComponent";
				out << YAML::BeginMap;
				out << YAML::Key << "ConnectedEntity" << YAML::Value << component->ConnectedEntity;
				out << YAML::Key << "CollideConnected" << YAML::Value << component->CollideConnected;
				out << YAML::Key << "AnchorA" << YAML::Value << component->AnchorA;
				out << YAML::Key << "AnchorB" << YAML::Value << component->AnchorB;
				out << YAML::Key << "GroundAnchorA" << YAML::Value << component->GroundAnchorA;
				out << YAML::Key << "GroundAnchorB" << YAML::Value << component->GroundAnchorB;
				out << YAML::Key << "Ratio" << YAML::Value << component->Ratio;
				out << YAML::EndMap;
			}

			if (auto component = entity.TryGetComponent<ScriptComponent>())
			{
				out << YAML::Key << "ScriptComponent";
				out << YAML::BeginMap;
				out << YAML::Key << "ScriptName" << YAML::Value << component->ScriptName;

				out << YAML::Key << "Fields";
				out << YAML::BeginSeq;

				const auto& fieldStorages = ScriptEngine::GetFieldStorageMap(entity);
				for (const auto& [name, storage] : fieldStorages)
					out << YAML::Node(storage);

				out << YAML::EndSeq;
				out << YAML::EndMap;
			}

			out << YAML::EndMap;
		}

		out << YAML::EndSeq;
		out << YAML::EndMap;
		out << YAML::EndMap;

		return out.c_str();
	}

	bool SceneSerializer::DeserializeFromYAML(Ref<Scene> scene, const std::string& filedata)
	{
		SK_PROFILE_FUNCTION();

		YAML::Node node = YAML::Load(filedata);

		YAML::Node sceneNode = node["Scene"];
		if (!sceneNode)
			return false;

		const std::string sceneName = sceneNode["Name"].as<std::string>();
		scene->SetName(sceneName);
		SK_CORE_TRACE_TAG("Serialization", " - Scene Name: {}", sceneName);

		const UUID activeCameraID = sceneNode["ActiveCamera"].as<UUID>(UUID::Invalid);
		scene->SetActiveCamera(activeCameraID);
		SK_CORE_TRACE_TAG("Serialization", " - Active Camera: {}", activeCameraID);

		YAML::Node entitiesNode = sceneNode["Entities"];
		if (entitiesNode)
		{
			for (auto entityNode : entitiesNode)
			{
				const UUID id = entityNode["Entity"].as<UUID>(UUID::Invalid);
				const std::string name = entityNode["TagComponent"]["Name"].as<std::string>();

				Entity entity = scene->CreateEntityWithUUID(id, name);
				SK_CORE_TRACE_TAG("Serialization", " - Entity {} - {}", entity.GetName(), entity.GetUUID());

				if (auto componentNode = entityNode["TransformComponent"])
				{
					auto& component = entity.AddOrReplaceComponent<TransformComponent>();
					SK_DESERIALIZE_PROPERTY(componentNode, "Translation", component.Translation, glm::vec3(0.0f));
					SK_DESERIALIZE_PROPERTY(componentNode, "Rotation", component.Rotation, glm::vec3(0.0f));
					SK_DESERIALIZE_PROPERTY(componentNode, "Scale", component.Scale, glm::vec3(1.0f));
				}

				if (auto componentNode = entityNode["RelationshipComponent"])
				{
					auto& component = entity.AddOrReplaceComponent<RelationshipComponent>();
					SK_DESERIALIZE_PROPERTY(componentNode, "Parent", component.Parent, UUID::Invalid);
					SK_DESERIALIZE_PROPERTY(componentNode, "Children", component.Children, {});
				}

				if (auto componentNode = entityNode["SpriteRendererComponent"])
				{
					auto& component = entity.AddOrReplaceComponent<SpriteRendererComponent>();
					SK_DESERIALIZE_PROPERTY(componentNode, "Color", component.Color, glm::vec4(1.0f));
					SK_DESERIALIZE_PROPERTY(componentNode, "TextureHandle", component.TextureHandle, AssetHandle::Invalid);
					SK_DESERIALIZE_PROPERTY(componentNode, "TilingFactor", component.TilingFactor, 1.0f);
					SK_DESERIALIZE_PROPERTY(componentNode, "Transparent", component.Transparent, false);
				}

				if (auto componentNode = entityNode["CircleRendererComponent"])
				{
					auto& component = entity.AddOrReplaceComponent<CircleRendererComponent>();
					SK_DESERIALIZE_PROPERTY(componentNode, "Color", component.Color, glm::vec4(1.0f));
					SK_DESERIALIZE_PROPERTY(componentNode, "Thickness", component.Thickness, 1.0f);
					SK_DESERIALIZE_PROPERTY(componentNode, "Fade", component.Fade, 0.002f);
					SK_DESERIALIZE_PROPERTY(componentNode, "Filled", component.Filled, true);
					SK_DESERIALIZE_PROPERTY(componentNode, "Transparent", component.Transparent, false);
				}

				if (auto componentNode = entityNode["TextRendererComponent"])
				{
					auto& component = entity.AddOrReplaceComponent<TextRendererComponent>();
					SK_DESERIALIZE_PROPERTY(componentNode, "FontHandle", component.FontHandle, AssetHandle::Invalid);
					SK_DESERIALIZE_PROPERTY(componentNode, "Text", component.Text, {});
					SK_DESERIALIZE_PROPERTY(componentNode, "Color", component.Color, glm::vec4(1.0f));
					SK_DESERIALIZE_PROPERTY(componentNode, "Kerning", component.Kerning, 0.0f);
					SK_DESERIALIZE_PROPERTY(componentNode, "LineSpacing", component.LineSpacing, 0.0f);
				}

				if (auto componentNode = entityNode["MeshComponent"])
				{
					auto& component = entity.AddOrReplaceComponent<MeshComponent>();
					SK_DESERIALIZE_PROPERTY(componentNode, "Mesh", component.Mesh, AssetHandle::Invalid);
					SK_DESERIALIZE_PROPERTY(componentNode, "SubmeshIndex", component.SubmeshIndex, 0);
					SK_DESERIALIZE_PROPERTY(componentNode, "Material", component.Material, AssetHandle::Invalid);
				}

				if (auto componentNode = entityNode["PointLightComponent"])
				{
					auto& component = entity.AddOrReplaceComponent<PointLightComponent>();
					SK_DESERIALIZE_PROPERTY(componentNode, "Radiance", component.Radiance, glm::vec4(1.0f));
					SK_DESERIALIZE_PROPERTY(componentNode, "Intensity", component.Intensity, 1.0f);
					SK_DESERIALIZE_PROPERTY(componentNode, "Radius", component.Radius, 10.0f);
					SK_DESERIALIZE_PROPERTY(componentNode, "Falloff", component.Falloff, 1.0f);
				}
				
				if (auto componentNode = entityNode["DirectionalLightComponent"])
				{
					auto& component = entity.AddOrReplaceComponent<DirectionalLightComponent>();
					SK_DESERIALIZE_PROPERTY(componentNode, "Radiance", component.Radiance, glm::vec4(1.0f));
					SK_DESERIALIZE_PROPERTY(componentNode, "Intensity", component.Intensity, 1.0f);
				}

				if (auto componentNode = entityNode["SkyComponent"])
				{
					auto& component = entity.AddOrReplaceComponent<SkyComponent>();
					SK_DESERIALIZE_PROPERTY(componentNode, "SceneEnvironment", component.SceneEnvironment, AssetHandle::Invalid);
					SK_DESERIALIZE_PROPERTY(componentNode, "Intensity", component.Intensity, 1.0f);
					SK_DESERIALIZE_PROPERTY(componentNode, "Lod", component.Lod, 0.0f);
				}

				if (auto componentNode = entityNode["CameraComponent"])
				{
					auto& component = entity.AddOrReplaceComponent<CameraComponent>();
					SK_DESERIALIZE_PROPERTY(componentNode, "IsPerspective", component.IsPerspective, true);
					SK_DESERIALIZE_PROPERTY(componentNode, "PerspectiveFOV", component.PerspectiveFOV, 45.0f);
					SK_DESERIALIZE_PROPERTY(componentNode, "OrthographicSize", component.OrthographicSize, 10.0f);
					SK_DESERIALIZE_PROPERTY(componentNode, "Near", component.Near, 0.3f);
					SK_DESERIALIZE_PROPERTY(componentNode, "Far", component.Far, 1000.0f);

					component.PerspectiveFOV *= Math::Deg2Rad;
					component.Recalculate();
				}

				if (auto componentNode = entityNode["RigidBody2DComponent"])
				{
					auto& component = entity.AddOrReplaceComponent<RigidBody2DComponent>();
					SK_DESERIALIZE_PROPERTY(componentNode, "Type", component.Type, RigidBody2DComponent::BodyType::None);
					SK_DESERIALIZE_PROPERTY(componentNode, "FixedRotation", component.FixedRotation, false);
					SK_DESERIALIZE_PROPERTY(componentNode, "IsBullet", component.IsBullet, false);
					SK_DESERIALIZE_PROPERTY(componentNode, "Awake", component.Awake, true);
					SK_DESERIALIZE_PROPERTY(componentNode, "Enabled", component.Enabled, true);
					SK_DESERIALIZE_PROPERTY(componentNode, "AllowSleep", component.AllowSleep, true);
					SK_DESERIALIZE_PROPERTY(componentNode, "GravityScale", component.GravityScale, 1.0f);
				}

				if (auto componentNode = entityNode["BoxCollider2DComponent"])
				{
					auto& component = entity.AddOrReplaceComponent<BoxCollider2DComponent>();
					SK_DESERIALIZE_PROPERTY(componentNode, "Size", component.Size, glm::vec2(0.5f, 0.5f));
					SK_DESERIALIZE_PROPERTY(componentNode, "Offset", component.Offset, glm::vec2(0.0f, 0.0f));
					SK_DESERIALIZE_PROPERTY(componentNode, "Rotation", component.Rotation, 0.0f);
					SK_DESERIALIZE_PROPERTY(componentNode, "Density", component.Density, 1.0f);
					SK_DESERIALIZE_PROPERTY(componentNode, "Friction", component.Friction, 0.2f);
					SK_DESERIALIZE_PROPERTY(componentNode, "Restitution", component.Restitution, 0.0f);
					SK_DESERIALIZE_PROPERTY(componentNode, "RestitutionThreshold", component.RestitutionThreshold, 1.0f);
					SK_DESERIALIZE_PROPERTY(componentNode, "IsSensor", component.IsSensor, false);
				}

				if (auto componentNode = entityNode["CircleCollider2DComponent"])
				{
					auto& component = entity.AddOrReplaceComponent<CircleCollider2DComponent>();
					SK_DESERIALIZE_PROPERTY(componentNode, "Radius", component.Radius, 0.5f);
					SK_DESERIALIZE_PROPERTY(componentNode, "Offset", component.Offset, glm::vec2(0.0f, 0.0f));
					SK_DESERIALIZE_PROPERTY(componentNode, "Rotation", component.Rotation, 0.0f);
					SK_DESERIALIZE_PROPERTY(componentNode, "Density", component.Density, 1.0f);
					SK_DESERIALIZE_PROPERTY(componentNode, "Friction", component.Friction, 0.2f);
					SK_DESERIALIZE_PROPERTY(componentNode, "Restitution", component.Restitution, 0.0f);
					SK_DESERIALIZE_PROPERTY(componentNode, "RestitutionThreshold", component.RestitutionThreshold, 1.0f);
					SK_DESERIALIZE_PROPERTY(componentNode, "IsSensor", component.IsSensor, false);
				}

				if (auto componentNode = entityNode["DistanceJointComponent"])
				{
					auto& component = entity.AddOrReplaceComponent<DistanceJointComponent>();
					SK_DESERIALIZE_PROPERTY(componentNode, "ConnectedEntity", component.ConnectedEntity, UUID::Invalid);
					SK_DESERIALIZE_PROPERTY(componentNode, "CollideConnected", component.CollideConnected, true);
					SK_DESERIALIZE_PROPERTY(componentNode, "AnchorOffsetA", component.AnchorOffsetA, glm::vec2(0.0f, 0.0f));
					SK_DESERIALIZE_PROPERTY(componentNode, "AnchorOffsetB", component.AnchorOffsetB, glm::vec2(0.0f, 0.0f));
					SK_DESERIALIZE_PROPERTY(componentNode, "MinLength", component.MinLength, -1.0f);
					SK_DESERIALIZE_PROPERTY(componentNode, "MaxLength", component.MaxLength, -1.0f);
					SK_DESERIALIZE_PROPERTY(componentNode, "Stiffness", component.Stiffness, 0.0f);
					SK_DESERIALIZE_PROPERTY(componentNode, "Damping", component.Damping, 0.0f);
				}

				if (auto componentNode = entityNode["HingeJointComponent"])
				{
					auto& component = entity.AddOrReplaceComponent<HingeJointComponent>();
					SK_DESERIALIZE_PROPERTY(componentNode, "ConnectedEntity", component.ConnectedEntity, UUID::Invalid);
					SK_DESERIALIZE_PROPERTY(componentNode, "CollideConnected", component.CollideConnected, true);
					SK_DESERIALIZE_PROPERTY(componentNode, "Anchor", component.Anchor, glm::vec2(0.0f));
					SK_DESERIALIZE_PROPERTY(componentNode, "LowerAngle", component.LowerAngle, 0.0f);
					SK_DESERIALIZE_PROPERTY(componentNode, "UpperAngle", component.UpperAngle, 0.0f);
					SK_DESERIALIZE_PROPERTY(componentNode, "EnableMotor", component.EnableMotor, false);
					SK_DESERIALIZE_PROPERTY(componentNode, "MotorSpeed", component.MotorSpeed, 0.0f);
					SK_DESERIALIZE_PROPERTY(componentNode, "MaxMotorTorque", component.MaxMotorTorque, 0.0f);
				}

				if (auto componentNode = entityNode["PrismaticJointComponent"])
				{
					auto& component = entity.AddOrReplaceComponent<PrismaticJointComponent>();
					SK_DESERIALIZE_PROPERTY(componentNode, "ConnectedEntity", component.ConnectedEntity, UUID::Invalid);
					SK_DESERIALIZE_PROPERTY(componentNode, "CollideConnected", component.CollideConnected, true);
					SK_DESERIALIZE_PROPERTY(componentNode, "Anchor", component.Anchor, glm::vec2(0.0f));
					SK_DESERIALIZE_PROPERTY(componentNode, "Axis", component.Axis, glm::vec2(1.0f, 0.0f));
					SK_DESERIALIZE_PROPERTY(componentNode, "EnableLimit", component.EnableLimit, false);
					SK_DESERIALIZE_PROPERTY(componentNode, "LowerTranslation", component.LowerTranslation, 0.0f);
					SK_DESERIALIZE_PROPERTY(componentNode, "UpperTranslation", component.UpperTranslation, 0.0f);
					SK_DESERIALIZE_PROPERTY(componentNode, "EnableMotor", component.EnableMotor, false);
					SK_DESERIALIZE_PROPERTY(componentNode, "MotorSpeed", component.MotorSpeed, 0.0f);
					SK_DESERIALIZE_PROPERTY(componentNode, "MaxMotorForce", component.MaxMotorForce, 0.0f);
				}

				if (auto componentNode = entityNode["PulleyJointComponent"])
				{
					auto& component = entity.AddOrReplaceComponent<PulleyJointComponent>();
					SK_DESERIALIZE_PROPERTY(componentNode, "ConnectedEntity", component.ConnectedEntity, UUID::Invalid);
					SK_DESERIALIZE_PROPERTY(componentNode, "CollideConnected", component.CollideConnected, true);
					SK_DESERIALIZE_PROPERTY(componentNode, "AnchorA", component.AnchorA, glm::vec2(0.0f));
					SK_DESERIALIZE_PROPERTY(componentNode, "AnchorB", component.AnchorB, glm::vec2(0.0f));
					SK_DESERIALIZE_PROPERTY(componentNode, "GroundAnchorA", component.GroundAnchorA, glm::vec2(0.0f));
					SK_DESERIALIZE_PROPERTY(componentNode, "GroundAnchorB", component.GroundAnchorB, glm::vec2(0.0f));
					SK_DESERIALIZE_PROPERTY(componentNode, "Ratio", component.Ratio, 1.0f);
				}

				if (auto componentNode = entityNode["ScriptComponent"])
				{
					auto& component = entity.AddOrReplaceComponent<ScriptComponent>();
					SK_DESERIALIZE_PROPERTY(componentNode, "ScriptName", component.ScriptName, std::string());

					Ref<ScriptClass> klass = ScriptEngine::GetScriptClassFromName(component.ScriptName);
					component.ClassID = klass ? klass->GetID() : 0;

					auto& fieldStorages = ScriptEngine::GetFieldStorageMap(entity);

					auto fieldsNode = componentNode["Fields"];
					for (auto fieldNode : fieldsNode)
					{
						Ref<FieldStorage> storage = fieldNode.as<Ref<FieldStorage>>(nullptr);
						if (storage)
							fieldStorages[storage->Name] = storage;
					}
				}

			}
		}
		return true;
	}

}
