#include "skpch.h"
#include "SceneSerializer.h"
#include "Shark/Asset/ResourceManager.h"
#include "Shark/Scene/Scene.h"
#include "Shark/Scene/Entity.h"
#include "Shark/Utils/YAMLUtils.h"

#include <yaml-cpp/yaml.h>
#include "Shark/Scripting/ScriptEngine.h"
#include "Shark/Debug/enttDebug.h"

#define SK_SERIALIZATION_ERROR(...) SK_CORE_ERROR_TAG("Serialization", __VA_ARGS__); SK_DEBUG_BREAK();

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
				default: SK_CORE_ASSERT(false, "Unkown ManagedFieldType");
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
				default: SK_CORE_ASSERT(false, "Unkown ManagedFieldType");
			}

			return true;
		}
	};

}

namespace Shark {

	bool SceneSerializer::Serialize(Ref<Asset> asset, const AssetMetaData& metadata)
	{
		SK_CORE_VERIFY(asset);
		SK_CORE_INFO_TAG("Serialization", "Serializing Scene to {}", metadata.FilePath);
		Timer timer;

		if (!ResourceManager::HasExistingFilePath(metadata))
		{
			SK_SERIALIZATION_ERROR("Path not found! {0}", metadata.FilePath);
			return false;
		}

		std::string result = SerializeToYAML(asset.As<Scene>());
		if (result.empty())
		{
			SK_SERIALIZATION_ERROR("YAML result was empty!");
			return false;
		}

		std::ofstream fout(ResourceManager::GetFileSystemPath(metadata));
		SK_CORE_ASSERT(fout);

		fout << result;
		fout.close();

		SK_CORE_INFO_TAG("Serialization", "Serializing Scene took {}ms", timer.ElapsedMilliSeconds());
		return true;
	}

	bool SceneSerializer::Deserialize(Ref<Asset>& asset, const AssetMetaData& metadata)
	{
		SK_CORE_INFO_TAG("Serialization", "Deserializing Scene from {}", metadata.FilePath);
		Timer timer;

		if (!ResourceManager::HasExistingFilePath(metadata))
		{
			SK_SERIALIZATION_ERROR("Path not found! {0}", metadata.FilePath);
			return false;
		}

		std::string filedata = FileSystem::ReadString(ResourceManager::GetFileSystemPath(metadata));
		if (filedata.empty())
		{
			SK_SERIALIZATION_ERROR("File was empty!");
			return false;
		}

		Ref<Scene> scene = Ref<Scene>::Create();
		if (!DeserializeFromYAML(scene, filedata))
		{
			SK_SERIALIZATION_ERROR("Failed to load data from YAML!");
			return false;
		}

		asset = scene;
		asset->Handle = metadata.Handle;

		SK_CORE_INFO_TAG("Serialization", "Deserializing Scene took {}ms", timer.ElapsedMilliSeconds());
		return true;
	}

	std::string SceneSerializer::SerializeToYAML(Ref<Scene> scene)
	{
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
			
			if (auto component = entity.TryGetComponent<CameraComponent>())
			{
				out << YAML::Key << "CameraComponent";
				out << YAML::BeginMap;

				auto& camera = component->Camera;
				out << YAML::Key << "Type" << YAML::Value << ToStringView(camera.GetProjectionType());
				out << YAML::Key << "Aspectratio" << YAML::Value << camera.GetAspectratio();
				out << YAML::Key << "PerspectiveFOV" << YAML::Value << camera.GetPerspectiveFOV();
				out << YAML::Key << "PerspectiveNear" << YAML::Value << camera.GetPerspectiveNear();
				out << YAML::Key << "PerspectiveFar" << YAML::Value << camera.GetPerspectiveFar();
				out << YAML::Key << "OrthographicZoom" << YAML::Value << camera.GetOrthographicZoom();
				out << YAML::Key << "OrthographicNear" << YAML::Value << camera.GetOrthographicNear();
				out << YAML::Key << "OrthographicFar" << YAML::Value << camera.GetOrthographicFar();
				out << YAML::EndMap;
			}
			
			if (auto component = entity.TryGetComponent<RigidBody2DComponent>())
			{
				out << YAML::Key << "RigidBody2DComponent";
				out << YAML::BeginMap;
				out << YAML::Key << "Type" << YAML::Value << ToStringView(component->Type);
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
		YAML::Node node = YAML::Load(filedata);

		YAML::Node sceneNode = node["Scene"];
		if (!sceneNode)
			return false;
		
		const std::string sceneName = node["Scene"].as<std::string>("Untitled");
		scene->SetName(sceneName);
		SK_CORE_TRACE_TAG("Serialization", " - Scene Name: {}", sceneName);

		const UUID activeCameraID = sceneNode["ActiveCamera"].as<UUID>(UUID::Null);
		scene->SetActiveCamera(activeCameraID);
		SK_CORE_TRACE_TAG("Serialization", " - Active Camera: {}", activeCameraID);

		YAML::Node entitiesNode = sceneNode["Entities"];
		if (entitiesNode)
		{
			for (auto entityNode : entitiesNode)
			{
				const UUID id = entityNode["Entity"].as<UUID>(UUID::Null);
				const std::string name = entityNode["TagComponent"]["Name"].as<std::string>();

				Entity entity = scene->CreateEntityWithUUID(id, name);
				SK_CORE_TRACE_TAG("Serialization", " - Entity {} - {}", entity.GetName(), entity.GetUUID());

				if (auto componentNode = entityNode["TransformComponent"])
				{
					auto& component = entity.AddOrReplaceComponent<TransformComponent>();
					component.Translation = componentNode["Translation"].as<glm::vec3>();
					component.Rotation    = componentNode["Rotation"].as<glm::vec3>();
					component.Scale       = componentNode["Scale"].as<glm::vec3>();
				}
				
				if (auto componentNode = entityNode["RelationshipComponent"])
				{
					auto& component = entity.AddOrReplaceComponent<RelationshipComponent>();
					component.Parent = componentNode["Parent"].as<UUID>();
					component.Children = componentNode["Children"].as<std::vector<UUID>>();
				}
				
				if (auto componentNode = entityNode["SpriteRendererComponent"])
				{
					auto& component = entity.AddOrReplaceComponent<SpriteRendererComponent>();
					component.Color = componentNode["Color"].as<glm::vec4>();
					component.TextureHandle = componentNode["TextureHandle"].as<AssetHandle>();
					component.TilingFactor = componentNode["TilingFactor"].as<float>();
				}
				
				if (auto componentNode = entityNode["CircleRendererComponent"])
				{
					auto& component = entity.AddOrReplaceComponent<CircleRendererComponent>();
					component.Color = componentNode["Color"].as<glm::vec4>();
					component.Thickness = componentNode["Thickness"].as<float>();
					component.Fade = componentNode["Fade"].as<float>();
				}
				
				if (auto componentNode = entityNode["CameraComponent"])
				{
					auto& component = entity.AddOrReplaceComponent<CameraComponent>();

					auto projection = StringToSceneCameraProjection(componentNode["Type"].as<std::string>());
					float aspecRatio = componentNode["Aspectratio"].as<float>();

					SceneCamera::PerspectiveSpecs ps;
					ps.FOV = componentNode["PerspectiveFOV"].as<float>();
					ps.Near = componentNode["PerspectiveNear"].as<float>();
					ps.Far = componentNode["PerspectiveFar"].as<float>();

					SceneCamera::OrthographicSpecs os;
					os.Zoom = componentNode["OrthographicZoom"].as<float>();
					os.Near = componentNode["OrthographicNear"].as<float>();
					os.Far = componentNode["OrthographicFar"].as<float>();

					component.Camera = SceneCamera(projection, aspecRatio, ps, os);
				}

				if (auto componentNode = entityNode["RigidBody2DComponent"])
				{
					auto& component = entity.AddOrReplaceComponent<RigidBody2DComponent>();
					component.Type = StringToRigidBody2DType(componentNode["Type"].as<std::string>());
					component.FixedRotation = componentNode["FixedRotation"].as<bool>();
					component.IsBullet = componentNode["IsBullet"].as<bool>();
					component.Awake = componentNode["Awake"].as<bool>();
					component.Enabled = componentNode["Enabled"].as<bool>();
					component.AllowSleep = componentNode["AllowSleep"].as<bool>();
					component.GravityScale = componentNode["GravityScale"].as<float>();
				}
				
				if (auto componentNode = entityNode["BoxCollider2DComponent"])
				{
					auto& component = entity.AddOrReplaceComponent<BoxCollider2DComponent>();
					component.Size = componentNode["Size"].as<glm::vec2>();
					component.Offset = componentNode["Offset"].as<glm::vec2>();
					component.Rotation = componentNode["Rotation"].as<float>();
					component.Density = componentNode["Density"].as<float>();
					component.Friction = componentNode["Friction"].as<float>();
					component.Restitution = componentNode["Restitution"].as<float>();
					component.RestitutionThreshold = componentNode["RestitutionThreshold"].as<float>();
					component.IsSensor = componentNode["IsSensor"].as<bool>();
				}
				
				if (auto componentNode = entityNode["CircleCollider2DComponent"])
				{
					auto& component = entity.AddOrReplaceComponent<CircleCollider2DComponent>();
					component.Radius = componentNode["Radius"].as<float>();
					component.Offset = componentNode["Offset"].as<glm::vec2>();
					component.Rotation = componentNode["Rotation"].as<float>();
					component.Density = componentNode["Density"].as<float>();
					component.Friction = componentNode["Friction"].as<float>();
					component.Restitution = componentNode["Restitution"].as<float>();
					component.RestitutionThreshold = componentNode["RestitutionThreshold"].as<float>();
					component.IsSensor = componentNode["IsSensor"].as<bool>();
				}
				
				if (auto componentNode = entityNode["ScriptComponent"])
				{
					auto& component = entity.AddOrReplaceComponent<ScriptComponent>();
					component.ScriptName = componentNode["ScriptName"].as<std::string>();

					Ref<ScriptClass> klass = ScriptEngine::GetScriptClassFromName(component.ScriptName);
					component.ClassID = klass ? klass->GetID() : 0;

					auto fields = componentNode["Fields"];
					auto& fieldStorages = ScriptEngine::GetFieldStorageMap(entity);

					for (auto field : fields)
					{
						Ref<FieldStorage> storage = field.as<Ref<FieldStorage>>();
						fieldStorages[storage->Name] = storage;
					}
				}
				
			}
		}

		return true;
	}

}