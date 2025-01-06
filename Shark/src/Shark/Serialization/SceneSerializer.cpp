#include "skpch.h"
#include "SceneSerializer.h"

#include "Shark/Asset/AssetManager.h"
#include "Shark/Scene/Scene.h"
#include "Shark/Scene/Entity.h"
#include "Shark/Scripting/ScriptEngine.h"

#include "Shark/Serialization/YAML.h"
#include "Shark/Serialization/SerializationMacros.h"

#include "Shark/File/FileSystem.h"

#include "Shark/Debug/enttDebug.h"
#include "Shark/Debug/Profiler.h"

#include <magic_enum.hpp>

#if SK_DEBUG
#define SK_SERIALIZATION_ERROR(...) SK_CORE_ERROR_TAG("Serialization", __VA_ARGS__); SK_DEBUG_BREAK()
#else
#define SK_SERIALIZATION_ERROR(...) SK_CORE_ERROR_TAG("Serialization", __VA_ARGS__)
#endif

namespace YAML {

	template<>
	struct convert<Shark::Ref<Shark::MaterialTable>>
	{
		static Node encode(Shark::Ref<Shark::MaterialTable> materialTable)
		{
			return Node(materialTable->GetMaterials());
		}

		static bool decode(const Node& node, Shark::Ref<Shark::MaterialTable>& materialTable)
		{
			if (!node.IsMap())
				return false;

			materialTable = Shark::Ref<Shark::MaterialTable>::Create();
			auto materials = node.as<Shark::MaterialTable::MaterialMap>();
			for (const auto& [index, material] : materials)
			{
				if (material)
					materialTable->SetMaterial(index, material);
				else
					materialTable->ClearMaterial(index);
			}
			return true;
		}
	};

}

namespace Shark {

	//////////////////////////////////////////////////////////////////////////////////////////////////
	//// Scene Serializer ////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////

	SceneSerializer::SceneSerializer(Ref<Scene> scene)
		: m_Scene(scene)
	{
	}

	void SceneSerializer::Serialize(const std::filesystem::path& filepath)
	{
		SK_PROFILE_FUNCTION();

		YAML::Emitter out;
		SerializeToYAML(out);
		FileSystem::WriteString(filepath, out.c_str());
	}

	bool SceneSerializer::Deserialize(const std::filesystem::path& filepath)
	{
		SK_PROFILE_FUNCTION();

		if (!FileSystem::Exists(filepath))
		{
			m_ErrorMessage = "File not found", filepath;
			return false;
		}

		YAML::Node node = YAML::LoadFile(filepath);
		if (auto sceneNode = node["Scene"])
			return DeserializeFromYAML(sceneNode);

		m_ErrorMessage = "Scene node not found";
		return false;
	}

	void SceneSerializer::SerializeToYAML(YAML::Emitter& out)
	{
		SK_PROFILE_FUNCTION();

		out << YAML::BeginMap;
		out << YAML::Key << "Scene";

		out << YAML::BeginMap;
		out << YAML::Key << "Name" << YAML::Value << m_Scene->GetName();
		out << YAML::Key << "ActiveCamera" << YAML::Value << YAML::Hex << m_Scene->GetActiveCameraUUID() << YAML::Dec;
		out << YAML::Key << "Entities" << YAML::Value;
		out << YAML::BeginSeq;

		std::vector<Entity> entities = m_Scene->GetEntitiesSorted();
		for (Entity entity : entities)
		{
			SerializeEntity(out, entity);
		}

		out << YAML::EndSeq;
		out << YAML::EndMap;
		out << YAML::EndMap;
	}

	bool SceneSerializer::DeserializeFromYAML(YAML::Node& sceneNode)
	{
		SK_PROFILE_FUNCTION();

		SK_DESERIALIZE_PROPERTY(sceneNode, "Name", m_Scene->m_Name);
		SK_DESERIALIZE_PROPERTY(sceneNode, "ActiveCamera", m_Scene->m_ActiveCameraUUID);

		YAML::Node entitiesNode = sceneNode["Entities"];
		for (auto entityNode : entitiesNode)
		{
			DeserializeEntity(entityNode);
		}

		m_Scene->SortEntitites();
		return true;
	}

	void SceneSerializer::SerializeEntity(YAML::Emitter& out, Entity entity)
	{
		SK_PROFILE_FUNCTION();

		out << YAML::BeginMap;
		out << YAML::Key << "Entity" << YAML::Value << entity.GetUUID();

		if (auto component = entity.TryGetComponent<TagComponent>())
		{
			out << YAML::Key << "TagComponent";
			out << YAML::BeginMap;
			out << YAML::Key << "Name" << YAML::Value << component->Tag;
			out << YAML::EndMap;
		}

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
			out << YAML::Key << "Filled" << YAML::Value << component->Filled;
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
			out << YAML::Key << "LineSpacing" << YAML::Value << component->LineSpacing;
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
			out << YAML::EndMap;
		}

		if (auto component = entity.TryGetComponent<SubmeshComponent>())
		{
			AssetHandle meshHandle = AssetHandle::Invalid;
			if (!AssetManager::IsMemoryAsset(component->Mesh))
				meshHandle = component->Mesh;

			out << YAML::Key << "SubmeshComponent";
			out << YAML::BeginMap;
			out << YAML::Key << "Mesh" << YAML::Value << meshHandle;
			out << YAML::Key << "SubmeshIndex" << YAML::Value << component->SubmeshIndex;
			out << YAML::Key << "Material" << YAML::Value << component->Material;
			out << YAML::EndMap;
		}

		if (auto component = entity.TryGetComponent<MeshFilterComponent>())
		{
			out << YAML::Key << "MeshFilterComponent";
			out << YAML::BeginMap;
			out << YAML::Key << "RootEntityID" << YAML::Value << component->RootEntityID;
			out << YAML::EndMap;
		}

		if (auto component = entity.TryGetComponent<StaticMeshComponent>())
		{
			AssetHandle meshHandle = AssetHandle::Invalid;
			if (!AssetManager::IsMemoryAsset(component->StaticMesh))
				meshHandle = component->StaticMesh;

			out << YAML::Key << "StaticMeshComponent";
			out << YAML::BeginMap;
			out << YAML::Key << "StaticMesh" << YAML::Value << meshHandle;
			out << YAML::Key << "MaterialTable" << YAML::Value << component->MaterialTable;
			out << YAML::Key << "Visible" << YAML::Value << component->Visible;
			out << YAML::EndMap;
		}

		if (auto component = entity.TryGetComponent<PrefabComponent>())
		{
			AssetHandle prefabHandle;
			if (!AssetManager::IsMemoryAsset(component->Prefab))
				prefabHandle = component->Prefab;

			out << YAML::Key << "PrefabComponent";
			out << YAML::BeginMap;
			out << YAML::Key << "Prefab" << YAML::Value << prefabHandle;
			out << YAML::Key << "EntityID" << YAML::Value << component->EntityID;
			out << YAML::Key << "IsRoot" << YAML::Value << component->IsRoot;
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
			out << YAML::Key << "ScriptID" << YAML::Value << component->ScriptID;

			out << YAML::Key << "Fields";
			out << YAML::BeginSeq;

			const auto& scriptStorage = m_Scene->GetScriptStorage();
			if (scriptStorage.EntityInstances.contains(entity.GetUUID()))
			{
				const auto& entityStorage = scriptStorage.EntityInstances.at(entity.GetUUID());
				for (const auto& [fieldID, storage] : entityStorage.Fields)
				{
					out << YAML::BeginMap;
					out << YAML::Key << "ID" << YAML::Value << fieldID;
					out << YAML::Key << "Name" << YAML::Value << storage.GetName();
					out << YAML::Key << "Type" << YAML::Value << storage.GetDataType();

					switch (storage.m_DataType)
					{
						case ManagedFieldType::Bool: out << YAML::Key << "Value" << YAML::Value << storage.GetValue<bool>(); break;
						case ManagedFieldType::Byte: out << YAML::Key << "Value" << YAML::Value << storage.GetValue<uint8_t>(); break;
						case ManagedFieldType::SByte: out << YAML::Key << "Value" << YAML::Value << storage.GetValue<int8_t>(); break;
						case ManagedFieldType::Short: out << YAML::Key << "Value" << YAML::Value << storage.GetValue<int16_t>(); break;
						case ManagedFieldType::UShort: out << YAML::Key << "Value" << YAML::Value << storage.GetValue<uint16_t>(); break;
						case ManagedFieldType::Int: out << YAML::Key << "Value" << YAML::Value << storage.GetValue<int32_t>(); break;
						case ManagedFieldType::UInt: out << YAML::Key << "Value" << YAML::Value << storage.GetValue<uint32_t>(); break;
						case ManagedFieldType::Long: out << YAML::Key << "Value" << YAML::Value << storage.GetValue<int64_t>(); break;
						case ManagedFieldType::ULong: out << YAML::Key << "Value" << YAML::Value << storage.GetValue<uint64_t>(); break;
						case ManagedFieldType::Float: out << YAML::Key << "Value" << YAML::Value << storage.GetValue<float>(); break;
						case ManagedFieldType::Double: out << YAML::Key << "Value" << YAML::Value << storage.GetValue<double>(); break;
						case ManagedFieldType::Vector2: out << YAML::Key << "Value" << YAML::Value << storage.GetValue<glm::vec2>(); break;
						case ManagedFieldType::Vector3: out << YAML::Key << "Value" << YAML::Value << storage.GetValue<glm::vec3>(); break;
						case ManagedFieldType::Vector4: out << YAML::Key << "Value" << YAML::Value << storage.GetValue<glm::vec4>(); break;
						case ManagedFieldType::String: out << YAML::Key << "Value" << YAML::Value << storage.GetValue<std::string>(); break;
						case ManagedFieldType::Entity: out << YAML::Key << "Value" << YAML::Value << storage.GetValue<UUID>(); break;
						case ManagedFieldType::Prefab: out << YAML::Key << "Value" << YAML::Value << storage.GetValue<AssetHandle>(); break;
					}
					out << YAML::EndMap;
				}
			}

			out << YAML::EndSeq;
			out << YAML::EndMap;
		}

		out << YAML::EndMap;
	}

	void SceneSerializer::DeserializeEntity(YAML::Node& entityNode)
	{
		SK_PROFILE_FUNCTION();

		UUID entityID;
		std::string entityName;
		SK_DESERIALIZE_PROPERTY(entityNode, "Entity", entityID);
		SK_DESERIALIZE_PROPERTY(entityNode["TagComponent"], "Name", entityName);

		Entity entity = m_Scene->CreateEntityWithUUID(entityID, entityName, false);

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
			SK_DESERIALIZE_PROPERTY(componentNode, "TilingFactor", component.TilingFactor, glm::vec2(1.0f));
		}

		if (auto componentNode = entityNode["CircleRendererComponent"])
		{
			auto& component = entity.AddOrReplaceComponent<CircleRendererComponent>();
			SK_DESERIALIZE_PROPERTY(componentNode, "Color", component.Color, glm::vec4(1.0f));
			SK_DESERIALIZE_PROPERTY(componentNode, "Thickness", component.Thickness, 1.0f);
			SK_DESERIALIZE_PROPERTY(componentNode, "Fade", component.Fade, 0.002f);
			SK_DESERIALIZE_PROPERTY(componentNode, "Filled", component.Filled, true);
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
		}

		if (auto componentNode = entityNode["SubmeshComponent"])
		{
			auto& component = entity.AddOrReplaceComponent<SubmeshComponent>();
			SK_DESERIALIZE_PROPERTY(componentNode, "Mesh", component.Mesh, AssetHandle::Invalid);
			SK_DESERIALIZE_PROPERTY(componentNode, "SubmeshIndex", component.SubmeshIndex, 0);
			SK_DESERIALIZE_PROPERTY(componentNode, "Material", component.Material, AssetHandle::Invalid);
		}

		if (auto componentNode = entityNode["MeshFilterComponent"])
		{
			auto& component = entity.AddOrReplaceComponent<MeshFilterComponent>();
			SK_DESERIALIZE_PROPERTY(componentNode, "RootEntityID", component.RootEntityID, UUID::Invalid);
		}

		if (auto componentNode = entityNode["StaticMeshComponent"])
		{
			auto& component = entity.AddOrReplaceComponent<StaticMeshComponent>();
			SK_DESERIALIZE_PROPERTY(componentNode, "StaticMesh", component.StaticMesh);
			SK_DESERIALIZE_PROPERTY(componentNode, "MaterialTable", component.MaterialTable);
			SK_DESERIALIZE_PROPERTY(componentNode, "Visible", component.Visible);
		}

		if (auto componentNode = entityNode["PrefabComponent"])
		{
			auto& component = entity.AddOrReplaceComponent<PrefabComponent>();
			SK_DESERIALIZE_PROPERTY(componentNode, "Prefab", component.Prefab);
			SK_DESERIALIZE_PROPERTY(componentNode, "EntityID", component.EntityID);
			SK_DESERIALIZE_PROPERTY(componentNode, "IsRoot", component.IsRoot);
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
			SK_DESERIALIZE_PROPERTY(componentNode, "Type", component.Type, RigidbodyType::Static);
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
			auto& scriptEngine = ScriptEngine::Get();
			auto& component = entity.AddOrReplaceComponent<ScriptComponent>();

			const bool oldLayout = !componentNode["ScriptID"] && componentNode["ScriptName"];

			uint64_t scriptID = !oldLayout ? componentNode["ScriptID"].as<uint64_t>() : 0;
			if (oldLayout)
			{
				std::string scriptName = componentNode["ScriptName"].as<std::string>();
				scriptID = scriptEngine.FindScriptMetadata(scriptName);
			}

			component.ScriptID = scriptID;

			auto& scriptStorage = m_Scene->GetScriptStorage();
			if (scriptEngine.IsValidScriptID(scriptID))
			{
				scriptStorage.SetupEntityStorage(scriptID, entity.GetUUID());
				auto& entityStorage = scriptStorage.EntityInstances.at(entity.GetUUID());

				if (!oldLayout)
				{
					for (auto fieldNode : componentNode["Fields"])
					{
						uint64_t fieldID = fieldNode["ID"].as<uint64_t>();
						std::string fieldName = fieldNode["Name"].as<std::string>();

						if (!entityStorage.Fields.contains(fieldID))
						{
							SK_CORE_ERROR_TAG("Scripting", "Cannot deserialize field storage for field {}! The script no longer contains this field.", fieldName);
							continue;
						}

						auto& storage = entityStorage.Fields.at(fieldID);
						storage.m_Name = fieldName;
						storage.m_DataType = fieldNode["Type"].as<ManagedFieldType>();

						switch (storage.m_DataType)
						{
							case ManagedFieldType::Bool: storage.SetValue(fieldNode["Value"].as<bool>()); break;
							case ManagedFieldType::Byte: storage.SetValue(fieldNode["Value"].as<uint8_t>()); break;
							case ManagedFieldType::SByte: storage.SetValue(fieldNode["Value"].as<int8_t>()); break;
							case ManagedFieldType::Short: storage.SetValue(fieldNode["Value"].as<int16_t>()); break;
							case ManagedFieldType::UShort: storage.SetValue(fieldNode["Value"].as<uint16_t>()); break;
							case ManagedFieldType::Int: storage.SetValue(fieldNode["Value"].as<int32_t>()); break;
							case ManagedFieldType::UInt: storage.SetValue(fieldNode["Value"].as<uint32_t>()); break;
							case ManagedFieldType::Long: storage.SetValue(fieldNode["Value"].as<int64_t>()); break;
							case ManagedFieldType::ULong: storage.SetValue(fieldNode["Value"].as<uint64_t>()); break;
							case ManagedFieldType::Float: storage.SetValue(fieldNode["Value"].as<float>()); break;
							case ManagedFieldType::Double: storage.SetValue(fieldNode["Value"].as<double>()); break;
							case ManagedFieldType::Vector2: storage.SetValue(fieldNode["Value"].as<glm::vec2>()); break;
							case ManagedFieldType::Vector3: storage.SetValue(fieldNode["Value"].as<glm::vec3>()); break;
							case ManagedFieldType::Vector4: storage.SetValue(fieldNode["Value"].as<glm::vec4>()); break;
							case ManagedFieldType::String: storage.SetValue(fieldNode["Value"].as<std::string>()); break;
							case ManagedFieldType::Entity: storage.SetValue(fieldNode["Value"].as<UUID>()); break;
							case ManagedFieldType::Prefab: storage.SetValue(fieldNode["Value"].as<AssetHandle>()); break;
						}
					}
				}
				else
				{
					for (auto fieldNode : componentNode["Fields"])
					{
						std::string fieldName = fieldNode["Name"].as<std::string>();
						uint64_t fieldID = Hash::GenerateFNV(fieldName);

						if (!entityStorage.Fields.contains(fieldID))
						{
							SK_CORE_ERROR_TAG("Scripting", "Cannot deserialize field storage for field {}! The script no longer contains this field.", fieldName);
							continue;
						}

						auto& storage = entityStorage.Fields.at(fieldID);
						storage.m_Name = fieldName;
						storage.m_DataType = fieldNode["Type"].as<ManagedFieldType>(ManagedFieldType::None);

						switch (storage.m_DataType)
						{
							case Shark::ManagedFieldType::Bool: storage.SetValue(fieldNode["Value"].as<bool>()); break;
							case Shark::ManagedFieldType::Byte: storage.SetValue(fieldNode["Value"].as<uint8_t>()); break;
							case Shark::ManagedFieldType::SByte: storage.SetValue(fieldNode["Value"].as<int8_t>()); break;
							case Shark::ManagedFieldType::Short: storage.SetValue(fieldNode["Value"].as<int16_t>()); break;
							case Shark::ManagedFieldType::UShort: storage.SetValue(fieldNode["Value"].as<uint16_t>()); break;
							case Shark::ManagedFieldType::Int: storage.SetValue(fieldNode["Value"].as<int32_t>()); break;
							case Shark::ManagedFieldType::UInt: storage.SetValue(fieldNode["Value"].as<uint32_t>()); break;
							case Shark::ManagedFieldType::Long: storage.SetValue(fieldNode["Value"].as<int64_t>()); break;
							case Shark::ManagedFieldType::ULong: storage.SetValue(fieldNode["Value"].as<uint64_t>()); break;
							case Shark::ManagedFieldType::Float: storage.SetValue(fieldNode["Value"].as<float>()); break;
							case Shark::ManagedFieldType::Double: storage.SetValue(fieldNode["Value"].as<double>()); break;
							case Shark::ManagedFieldType::String: storage.SetValue(fieldNode["Value"].as<std::string>()); break;
							case Shark::ManagedFieldType::Entity: storage.SetValue(fieldNode["Value"].as<Shark::UUID>()); break;
							case Shark::ManagedFieldType::Vector2: storage.SetValue(fieldNode["Value"].as<glm::vec2>()); break;
							case Shark::ManagedFieldType::Vector3: storage.SetValue(fieldNode["Value"].as<glm::vec3>()); break;
							case Shark::ManagedFieldType::Vector4: storage.SetValue(fieldNode["Value"].as<glm::vec4>()); break;
						}
					}
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////
	//// Scene Asset Serializer //////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////

	bool SceneAssetSerializer::Serialize(Ref<Asset> asset, const AssetMetaData& metadata)
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_INFO_TAG("Serialization", "Serializing Scene to {}", metadata.FilePath);
		ScopedTimer timer("Serializing Scene");

		SceneSerializer serializer(asset.As<Scene>());
		serializer.Serialize(Project::GetActiveEditorAssetManager()->GetFilesystemPath(metadata));
		return true;
	}

	bool SceneAssetSerializer::TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata)
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_INFO_TAG("Serialization", "Deserializing Scene from {}", metadata.FilePath);
		ScopedTimer timer("Loading Scene");

		Ref<Scene> scene = Ref<Scene>::Create();
		SceneSerializer serializer(scene);
		if (!serializer.Deserialize(Project::GetActiveEditorAssetManager()->GetFilesystemPath(metadata)))
		{
			SK_CORE_ERROR_TAG("Serialization", "Failed to load scene!\n\t - {}\n\t - {}", serializer.GetErrorMessage(), metadata.FilePath);
			return false;
		}

		asset = scene;
		asset->Handle = metadata.Handle;
		return true;
	}

}
