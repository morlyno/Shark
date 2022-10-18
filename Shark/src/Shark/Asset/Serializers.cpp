#include "skpch.h"
#include "Serializers.h"

#include "Shark/Core/Timer.h"
#include "Shark/Asset/Assets.h"
#include "Shark/Asset/ResourceManager.h"

#include "Shark/Scene/Scene.h"
#include "Shark/Scene/Entity.h"
#include "Shark/Scene/Components.h"

#include "Shark/Scripting/ScriptEngine.h"
#include "Shark/Scripting/ScriptUtils.h"

#include "Shark/Render/Renderer.h"
#include "Shark/Render/Texture.h"

#include "Shark/Utils/YAMLUtils.h"
#include "Shark/Debug/Profiler.h"

#include <yaml-cpp/yaml.h>
#include <stb_image.h>

namespace Shark {

	namespace Convert {

		static std::string BodyTypeToString(RigidBody2DComponent::BodyType bodyType)
		{
			switch (bodyType)
			{
				case RigidBody2DComponent::BodyType::None:        return "None";
				case RigidBody2DComponent::BodyType::Static:      return "Static";
				case RigidBody2DComponent::BodyType::Dynamic:     return "Dynamic";
				case RigidBody2DComponent::BodyType::Kinematic:   return "Kinematic";
			}

			SK_CORE_ASSERT(false, "Unkonw Body Type");
			return "Unkown";
		}

		static RigidBody2DComponent::BodyType StringToBodyType(const std::string bodyType)
		{
			if (bodyType == "None") return RigidBody2DComponent::BodyType::None;
			if (bodyType == "Static") return RigidBody2DComponent::BodyType::Static;
			if (bodyType == "Dynamic") return RigidBody2DComponent::BodyType::Dynamic;
			if (bodyType == "Kinematic") return RigidBody2DComponent::BodyType::Kinematic;

			SK_CORE_ASSERT(false, "Unkonw Body Type");
			return RigidBody2DComponent::BodyType::None;
		}

		static std::string ProjectionToString(SceneCamera::Projection projection)
		{
			switch (projection)
			{
				case SceneCamera::Projection::None:           return "None";
				case SceneCamera::Projection::Orthographic:   return "Orthographic";
				case SceneCamera::Projection::Perspective:    return "Perspective";
			}

			SK_CORE_ASSERT(false, "Unkown Projection Type");
			return "Unkown";
		}

		static SceneCamera::Projection StringToProjection(const std::string& projection)
		{
			if (projection == "None")           return SceneCamera::Projection::None;
			if (projection == "Orthographic")   return SceneCamera::Projection::Orthographic;
			if (projection == "Perspective")    return SceneCamera::Projection::Perspective;

			SK_CORE_ASSERT(false, "Unkown Projection Type");
			return SceneCamera::Projection::None;
		}

		ImageFormat StringToImageFormat(const std::string& str)
		{
			if (str == "None")      return ImageFormat::None;
			if (str == "RGBA8")     return ImageFormat::RGBA8;
			if (str == "R32_SINT")  return ImageFormat::R32_SINT;
			if (str == "Depth32")   return ImageFormat::Depth32;

			return ImageFormat::RGBA8;
		}

		FilterMode StringToFilterMode(const std::string& str)
		{
			if (str == "Nearest") return FilterMode::Nearest;
			if (str == "Linear") return FilterMode::Linear;

			SK_CORE_ASSERT(false, "Unkown String");
			return FilterMode::Linear;
		}

		WrapMode StringToAddressMode(const std::string& str)
		{
			if (str == "Repeat") return WrapMode::Repeat;
			if (str == "Clamp")  return WrapMode::Clamp;
			if (str == "Mirror") return WrapMode::Mirror;
			if (str == "Border") return WrapMode::Border;

			SK_CORE_ASSERT(false, "Unkown String");
			return WrapMode::Repeat;
		}

	}

	#pragma region Scene

	static bool SerializeEntity(YAML::Emitter& out, Entity entity, const Ref<Scene>& scene)
	{
		SK_PROFILE_FUNCTION();

		if (!out.good())
		{
			SK_CORE_ERROR_TAG("Serialization", "Bad Yaml Emitter! Skipping Entity {0}", (uint32_t)entity);
			SK_CORE_ERROR_TAG("Serialization", fmt::format("Yaml Error: {}", out.GetLastError()));
			SK_CORE_ASSERT(false);
			return false;
		}

		if (!entity)
			return false;

		out << YAML::BeginMap;
		out << YAML::Key << "Entity" << YAML::Value << YAML::Hex << entity.GetUUID() << YAML::Dec;

		if (entity.AllOf<TagComponent>())
		{
			out << YAML::Key << "TagComponent" << YAML::Value;
			out << YAML::BeginMap;

			auto& comp = entity.GetComponent<TagComponent>();
			out << YAML::Key << "Tag" << YAML::Value << comp.Tag;

			out << YAML::EndMap;

			SK_CORE_TRACE_TAG("Serialization", "Searializing Entity. ID: 0x{0:x}, TAG: {1}", (uint32_t)entity, comp.Tag);
		}

		if (entity.AllOf<TransformComponent>())
		{
			out << YAML::Key << "TransformComponent" << YAML::Value;
			out << YAML::BeginMap;

			auto& comp = entity.GetComponent<TransformComponent>();
			out << YAML::Key << "Position" << YAML::Value << comp.Translation;
			out << YAML::Key << "Rotation" << YAML::Value << comp.Rotation;
			out << YAML::Key << "Scaling" << YAML::Value << comp.Scale;

			out << YAML::EndMap;
		}

		if (entity.AllOf<RelationshipComponent>())
		{
			out << YAML::Key << "RelationshipComponent" << YAML::Value;
			out << YAML::BeginMap;

			auto& comp = entity.GetComponent<RelationshipComponent>();
			out << YAML::Key << "Parent" << YAML::Hex << comp.Parent << YAML::Dec;

			out << YAML::Key << "Children" << YAML::Value;
			out << YAML::BeginSeq;
			for (UUID childID : comp.Children)
			{
				out << YAML::BeginMap;
				out << YAML::Key << "Child" << YAML::Value << YAML::Hex << childID << YAML::Dec;
				out << YAML::EndMap;
			}
			out << YAML::EndSeq;

			out << YAML::EndMap;
		}

		if (entity.AllOf<SpriteRendererComponent>())
		{
			out << YAML::Key << "SpriteRendererComponent" << YAML::Value;
			out << YAML::BeginMap;

			auto& comp = entity.GetComponent<SpriteRendererComponent>();
			out << YAML::Key << "Color" << YAML::Value << comp.Color;
			out << YAML::Key << "Texture" << YAML::Value << comp.TextureHandle;
			out << YAML::Key << "TilingFactor" << YAML::Value << comp.TilingFactor;

			out << YAML::EndMap;
		}

		if (entity.AllOf<CircleRendererComponent>())
		{
			out << YAML::Key << "CircleRendererComponent" << YAML::Value;
			out << YAML::BeginMap;

			const auto& comp = entity.GetComponent<CircleRendererComponent>();
			out << YAML::Key << "Color" << YAML::Value << comp.Color;
			out << YAML::Key << "Thickness" << YAML::Value << comp.Thickness;
			out << YAML::Key << "Fade" << YAML::Value << comp.Fade;

			out << YAML::EndMap;
		}

		if (entity.AllOf<CameraComponent>())
		{
			out << YAML::Key << "CameraComponent" << YAML::Value;
			out << YAML::BeginMap;

			auto& comp = entity.GetComponent<CameraComponent>();
			auto& cam = comp.Camera;

			out << YAML::Key << "Type" << YAML::Value << Convert::ProjectionToString(cam.GetProjectionType());
			out << YAML::Key << "Aspectratio" << YAML::Value << cam.GetAspectratio();

			out << YAML::Key << "PerspectiveFOV" << YAML::Value << cam.GetPerspectiveFOV();
			out << YAML::Key << "PerspectiveNear" << YAML::Value << cam.GetPerspectiveNear();
			out << YAML::Key << "PerspectiveFar" << YAML::Value << cam.GetPerspectiveFar();

			out << YAML::Key << "OrthographicZoom" << YAML::Value << cam.GetOrthographicZoom();
			out << YAML::Key << "OrthographicNear" << YAML::Value << cam.GetOrthographicNear();
			out << YAML::Key << "OrthographicFar" << YAML::Value << cam.GetOrthographicFar();

			out << YAML::EndMap;
		}

		if (entity.AllOf<RigidBody2DComponent>())
		{
			out << YAML::Key << "RigidBody2DComponent" << YAML::Value;
			out << YAML::BeginMap;

			const auto& comp = entity.GetComponent<RigidBody2DComponent>();

			out << YAML::Key << "Type" << YAML::Value << Convert::BodyTypeToString(comp.Type);
			out << YAML::Key << "FixedRotation" << YAML::Value << comp.FixedRotation;
			out << YAML::Key << "IsBullet" << YAML::Value << comp.IsBullet;
			out << YAML::Key << "Awake" << YAML::Value << comp.Awake;
			out << YAML::Key << "Enabled" << YAML::Value << comp.Enabled;
			out << YAML::Key << "GravityScale" << YAML::Value << comp.GravityScale;
			out << YAML::Key << "AllowSleep" << YAML::Value << comp.AllowSleep;

			out << YAML::EndMap;
		}

		if (entity.AllOf<BoxCollider2DComponent>())
		{
			out << YAML::Key << "BoxCollider2DComponent" << YAML::Value;
			out << YAML::BeginMap;

			const auto& comp = entity.GetComponent<BoxCollider2DComponent>();

			out << YAML::Key << "Size" << YAML::Value << comp.Size;
			out << YAML::Key << "Offset" << YAML::Value << comp.Offset;
			out << YAML::Key << "Rotation" << YAML::Value << comp.Rotation;
			out << YAML::Key << "Density" << YAML::Value << comp.Density;
			out << YAML::Key << "Friction" << YAML::Value << comp.Friction;
			out << YAML::Key << "Restitution" << YAML::Value << comp.Restitution;
			out << YAML::Key << "RestitutionThreshold" << YAML::Value << comp.RestitutionThreshold;
			out << YAML::Key << "IsSensor" << YAML::Value << comp.IsSensor;

			out << YAML::EndMap;
		}

		if (entity.AllOf<CircleCollider2DComponent>())
		{
			out << YAML::Key << "CircleCollider2DComponent" << YAML::Value;
			out << YAML::BeginMap;

			const auto& comp = entity.GetComponent<CircleCollider2DComponent>();

			out << YAML::Key << "Radius" << YAML::Value << comp.Radius;
			out << YAML::Key << "Offset" << YAML::Value << comp.Offset;
			out << YAML::Key << "Rotation" << YAML::Value << comp.Rotation;
			out << YAML::Key << "Density" << YAML::Value << comp.Density;
			out << YAML::Key << "Friction" << YAML::Value << comp.Friction;
			out << YAML::Key << "Restitution" << YAML::Value << comp.Restitution;
			out << YAML::Key << "RestitutionThreshold" << YAML::Value << comp.RestitutionThreshold;
			out << YAML::Key << "IsSensor" << YAML::Value << comp.IsSensor;

			out << YAML::EndMap;
		}

		if (entity.AllOf<ScriptComponent>())
		{
			out << YAML::Key << "ScriptComponent" << YAML::Value;
			out << YAML::BeginMap;

			const auto& comp = entity.GetComponent<ScriptComponent>();
			out << YAML::Key << "ScriptName" << YAML::Value << comp.ScriptName;

			out << YAML::Key << "Fields" << YAML::Value;
			out << YAML::BeginSeq;

			const auto& fieldStorages = ScriptEngine::GetFieldStorageMap(entity);
			for (const auto& [name, storage] : fieldStorages)
			{
				out << YAML::BeginMap;
				out << YAML::Key << "Name" << YAML::Value << name;
				out << YAML::Key << "Type" << YAML::Value << ToString(storage->Type);
				out << YAML::Key << "Value" << YAML::Value;
				switch (storage->Type)
				{
					case ManagedFieldType::Bool:      out << storage->GetValue<bool>(); break;
					case ManagedFieldType::Char:      out << storage->GetValue<char16_t>(); break;
					case ManagedFieldType::Byte:      out << storage->GetValue<uint8_t>(); break;
					case ManagedFieldType::SByte:     out << storage->GetValue<int8_t>(); break;
					case ManagedFieldType::Short:     out << storage->GetValue<int16_t>(); break;
					case ManagedFieldType::UShort:    out << storage->GetValue<uint16_t>(); break;
					case ManagedFieldType::Int:       out << storage->GetValue<int32_t>(); break;
					case ManagedFieldType::UInt:      out << storage->GetValue<uint32_t>(); break;
					case ManagedFieldType::Long:      out << storage->GetValue<int64_t>(); break;
					case ManagedFieldType::ULong:     out << storage->GetValue<uint64_t>(); break;
					case ManagedFieldType::Float:     out << storage->GetValue<float>(); break;
					case ManagedFieldType::Double:    out << storage->GetValue<double>(); break;
					case ManagedFieldType::String:    out << storage->GetValue<std::string>(); break;
					case ManagedFieldType::Entity:    out << storage->GetValue<UUID>(); break;
					case ManagedFieldType::Component: out << storage->GetValue<UUID>(); break;
					case ManagedFieldType::Vector2:   out << storage->GetValue<glm::vec2>(); break;
					case ManagedFieldType::Vector3:   out << storage->GetValue<glm::vec3>(); break;
					case ManagedFieldType::Vector4:   out << storage->GetValue<glm::vec4>(); break;
					default: SK_CORE_ASSERT("Unkown Field Type"); break;
				}
				out << YAML::EndMap;
			}

			out << YAML::EndSeq;

			out << YAML::EndMap;
		}

		out << YAML::EndMap;

		return true;
	}

	static bool LoadScene(const Ref<Scene>& scene, const AssetMetaData& metadata)
	{
		YAML::Node in = YAML::LoadFile(ResourceManager::GetFileSystemPath(metadata));
		auto sceneNode = in["Scene"];
		if (!sceneNode)
			return false;

		SK_CORE_INFO_TAG("Serialization", "Loading Scene [{0}]", metadata.FilePath);

		auto activeCamera = sceneNode["ActiveCamera"];
		SK_CORE_ASSERT(activeCamera, "Couldn't Load Active Camera");
		scene->SetActiveCamera(activeCamera.as<UUID>());

		auto entities = sceneNode["Entities"];
		if (entities)
		{
			for (auto entity : entities)
			{
				SK_PROFILE_SCOPED("SceneSerializer::Deserialize Entity");

				UUID uuid = entity["Entity"].as<uint64_t>();
				auto tagComp = entity["TagComponent"];
				auto tag = tagComp["Tag"].as<std::string>();

				Entity deserializedEntity = scene->CreateEntityWithUUID(uuid, tag);
				SK_CORE_TRACE_TAG("Serialization", "Deserializing Entity [{0}] {1:x}", tag, uuid);

				if (auto transformComponent = entity["TransformComponent"])
				{
					auto position = transformComponent["Position"];
					auto rotation = transformComponent["Rotation"];
					auto scaling = transformComponent["Scaling"];
					auto& comp = deserializedEntity.AddOrReplaceComponent<TransformComponent>();

					SK_CORE_ASSERT(position, "Couldn't deserialize TransformComponent::Position");
					comp.Translation = position.as<glm::vec3>();

					SK_CORE_ASSERT(rotation, "Couldn't deserialize TransformComponent::Rotation");
					comp.Rotation = rotation.as<glm::vec3>();

					SK_CORE_ASSERT(scaling, "Couldn't deserialize TransformComponent::Scaling");
					comp.Scale = scaling.as<glm::vec3>();
				}

				if (auto relationshipComponent = entity["RelationshipComponent"])
				{
					auto& comp = deserializedEntity.AddOrReplaceComponent<RelationshipComponent>();
					comp.Parent = relationshipComponent["Parent"].as<UUID>();

					auto children = relationshipComponent["Children"];
					for (auto child : children)
						comp.Children.emplace_back(child["Child"].as<UUID>());
				}

				if (auto spriteRendererComponent = entity["SpriteRendererComponent"])
				{
					auto color = spriteRendererComponent["Color"];
					auto textureHandle = spriteRendererComponent["Texture"];
					auto tilingfactor = spriteRendererComponent["TilingFactor"];
					auto thickness = spriteRendererComponent["Thickness"];
					auto geometry = spriteRendererComponent["Geometry"];

					auto& comp = deserializedEntity.AddOrReplaceComponent<SpriteRendererComponent>();

					SK_CORE_ASSERT(color, "Couldn't deserialize SpriteRendererComponent::Color");
					comp.Color = color.as<glm::vec4>();

					SK_CORE_ASSERT(textureHandle, "Couldn't deserialize SpriteRendererComponent::Texture");
					comp.TextureHandle = textureHandle.as<UUID>();

					SK_CORE_ASSERT(tilingfactor, "Couldn't deserialize SpriteRendererComponent::TilingFactor");
					comp.TilingFactor = tilingfactor.as<float>();
				}

				if (auto circleRendererComponent = entity["CircleRendererComponent"])
				{
					auto color = circleRendererComponent["Color"];
					auto thickness = circleRendererComponent["Thickness"];
					auto fade = circleRendererComponent["Fade"];

					auto& comp = deserializedEntity.AddOrReplaceComponent<CircleRendererComponent>();

					SK_CORE_ASSERT(color, "Couldn't deserialize CircleRendererCompnent::Color");
					comp.Color = color.as<glm::vec4>();

					SK_CORE_ASSERT(thickness, "Couldn't deserialize CirlceRendererComponent::Thickness");
					comp.Thickness = thickness.as<float>();

					SK_CORE_ASSERT(fade, "Couldn't deserialize CirlceRendererComponent::Fade");
					comp.Fade = fade.as<float>();
				}

				if (auto cameraComponent = entity["CameraComponent"])
				{
					SceneCamera::Projection projection = SceneCamera::Projection::Perspective;
					SceneCamera::PerspectiveSpecs ps;
					SceneCamera::OrthographicSpecs os;
					float aspecRatio = 1.77778f;
					bool mainCam = false;

					auto aspectratio = cameraComponent["Aspectratio"];
					auto perspectiveFOV = cameraComponent["PerspectiveFOV"];
					auto perspectiveNear = cameraComponent["PerspectiveNear"];
					auto perspectiveFar = cameraComponent["PerspectiveFar"];
					auto orthographicZoom = cameraComponent["OrthographicZoom"];
					auto orthographicNear = cameraComponent["OrthographicNear"];
					auto orthographicFar = cameraComponent["OrthographicFar"];
					auto isMainCamera = cameraComponent["IsMainCamera"];

					auto type = cameraComponent["Type"];
					SK_CORE_ASSERT(type, "Couldn't deserialize CameraComponent::Projection");
					projection = Convert::StringToProjection(type.as<std::string>());

					SK_CORE_ASSERT(aspectratio, "Couldn't deserialize CameraComponent::AspectRatio");
					aspecRatio = aspectratio.as<float>();

					SK_CORE_ASSERT(perspectiveFOV, "Couldn't deserialize CameraComponent::PerspectiveFOV");
					ps.FOV = perspectiveFOV.as<float>();

					SK_CORE_ASSERT(perspectiveNear, "Couldn't deserialize CameraComponent::PerspectiveNear");
					ps.Near = perspectiveNear.as<float>();

					SK_CORE_ASSERT(perspectiveFar, "Couldn't deserialize CameraComponent::PerspectiveFar");
					ps.Far = perspectiveFar.as<float>();

					SK_CORE_ASSERT(orthographicZoom, "Couldn't deserialize CameraComponent::OrthographicZoom");
					os.Zoom = orthographicZoom.as<float>();

					SK_CORE_ASSERT(orthographicNear, "Couldn't deserialize CameraComponent::OrthographicNear");
					os.Near = orthographicNear.as<float>();

					SK_CORE_ASSERT(orthographicNear, "Couldn't deserialize CameraComponent::OrthographicNear");
					os.Far = orthographicFar.as<float>();

					deserializedEntity.AddOrReplaceComponent<CameraComponent>(SceneCamera(projection, aspecRatio, ps, os));
				}

				if (auto rigidBody2DComponent = entity["RigidBody2DComponent"])
				{
					auto type = rigidBody2DComponent["Type"];
					auto fixedRotation = rigidBody2DComponent["FixedRotation"];
					auto isBullet = rigidBody2DComponent["IsBullet"];
					auto awake = rigidBody2DComponent["Awake"];
					auto enabled = rigidBody2DComponent["Enabled"];
					auto gravityScale = rigidBody2DComponent["GravityScale"];
					auto allowSleep = rigidBody2DComponent["AllowSleep"];

					auto& comp = deserializedEntity.AddOrReplaceComponent<RigidBody2DComponent>();

					SK_CORE_ASSERT(type, "Couldn't desirialize RigidBody2DComponent::Type");
					comp.Type = Convert::StringToBodyType(type.as<std::string>());

					SK_CORE_ASSERT(isBullet, "Couldn't desirialize RigidBody2DComponent::FixedRotation");
					comp.FixedRotation = fixedRotation.as<bool>();

					SK_CORE_ASSERT(isBullet, "Couldn't desirialize RigidBody2DComponent::IsBullet");
					comp.IsBullet = isBullet.as<bool>();

					SK_CORE_ASSERT(awake, "Couldn't desirialize RigidBody2DComponent::Awake");
					comp.Awake = awake.as<bool>();

					SK_CORE_ASSERT(enabled, "Couldn't desirialize RigidBody2DComponent::Enabled");
					comp.Enabled = enabled.as<bool>();

					SK_CORE_ASSERT(gravityScale, "Couldn't desirialize RigidBody2DComponent::GravityScale");
					comp.GravityScale = gravityScale.as<float>();

					SK_CORE_ASSERT(allowSleep, "Couldn't desirialize RigidBody2DComponent::AllowSleep");
					comp.AllowSleep = allowSleep.as<bool>();
				}

				if (auto boxCollider2DComponent = entity["BoxCollider2DComponent"])
				{
					auto size = boxCollider2DComponent["Size"];
					auto offset = boxCollider2DComponent["Offset"];
					auto rotation = boxCollider2DComponent["Rotation"];
					auto density = boxCollider2DComponent["Density"];
					auto friction = boxCollider2DComponent["Friction"];
					auto restitution = boxCollider2DComponent["Restitution"];
					auto restitutionThreshold = boxCollider2DComponent["RestitutionThreshold"];
					auto isSensor = boxCollider2DComponent["IsSensor"];

					auto& comp = deserializedEntity.AddOrReplaceComponent<BoxCollider2DComponent>();

					SK_CORE_ASSERT(size, "Couldn't desirialize BoxCollider2DComponent::Size");
					comp.Size = size.as<glm::vec2>();

					SK_CORE_ASSERT(offset, "Couldn't desirialize BoxCollider2DComponent::Offset");
					comp.Offset = offset.as<glm::vec2>();

					SK_CORE_ASSERT(rotation, "Couldn't desirialize BoxCollider2DComponent::Rotation");
					comp.Rotation = rotation.as<float>();

					SK_CORE_ASSERT(density, "Couldn't desirialize BoxCollider2DComponent::Density");
					comp.Density = density.as<float>();

					SK_CORE_ASSERT(friction, "Couldn't desirialize BoxCollider2DComponent::Friction");
					comp.Friction = friction.as<float>();

					SK_CORE_ASSERT(restitution, "Couldn't desirialize BoxCollider2DComponent::Restitution");
					comp.Restitution = restitution.as<float>();

					SK_CORE_ASSERT(restitutionThreshold, "Couldn't desirialize BoxCollider2DComponent::RestitutionThreshold");
					comp.RestitutionThreshold = restitutionThreshold.as<float>();

					SK_CORE_ASSERT(isSensor, "Couldn't desirialize BoxCollider2DComponent::IsSensor");
					comp.IsSensor = isSensor.as<bool>();
				}

				if (auto cirlceCollider2DComponent = entity["CircleCollider2DComponent"])
				{
					auto radius = cirlceCollider2DComponent["Radius"];
					auto offset = cirlceCollider2DComponent["Offset"];
					auto rotation = cirlceCollider2DComponent["Rotation"];
					auto density = cirlceCollider2DComponent["Density"];
					auto friction = cirlceCollider2DComponent["Friction"];
					auto restitution = cirlceCollider2DComponent["Restitution"];
					auto restitutionThreshold = cirlceCollider2DComponent["RestitutionThreshold"];
					auto isSensor = cirlceCollider2DComponent["IsSensor"];

					auto& comp = deserializedEntity.AddOrReplaceComponent<CircleCollider2DComponent>();

					SK_CORE_ASSERT(radius, "Couldn't desirialize CircleCollider2DComponent::Radius");
					comp.Radius = radius.as<float>();

					SK_CORE_ASSERT(offset, "Couldn't desirialize CircleCollider2DComponent::Offset");
					comp.Offset = offset.as<glm::vec2>();

					SK_CORE_ASSERT(rotation, "Couldn't desirialize CircleCollider2DComponent::Rotation");
					comp.Rotation = rotation.as<float>();

					SK_CORE_ASSERT(density, "Couldn't desirialize CircleCollider2DComponent::Density");
					comp.Density = density.as<float>();

					SK_CORE_ASSERT(friction, "Couldn't desirialize CircleCollider2DComponent::Friction");
					comp.Friction = friction.as<float>();

					SK_CORE_ASSERT(restitution, "Couldn't desirialize CircleCollider2DComponent::Restitution");
					comp.Restitution = restitution.as<float>();

					SK_CORE_ASSERT(restitutionThreshold, "Couldn't desirialize CircleCollider2DComponent::RestitutionThreshold");
					comp.RestitutionThreshold = restitutionThreshold.as<float>();

					SK_CORE_ASSERT(isSensor, "Couldn't desirialize CircleCollider2DComponent::IsSensor");
					comp.IsSensor = isSensor.as<bool>(false);
				}

				if (auto scriptComponent = entity["ScriptComponent"])
				{
					auto name = scriptComponent["ScriptName"];
					auto& comp = deserializedEntity.AddOrReplaceComponent<ScriptComponent>();

					SK_CORE_ASSERT(name, "Couldn't deserialize ScriptComponent::ScriptName");
					comp.ScriptName = name.as<std::string>();
					Ref<ScriptClass> klass = ScriptEngine::GetScriptClassFromName(comp.ScriptName);
					comp.ClassID = klass ? klass->GetID() : 0;

					auto fields = scriptComponent["Fields"];
					auto& fieldStorages = ScriptEngine::GetFieldStorageMap(deserializedEntity);
					for (auto field : fields)
					{
						Ref<FieldStorage> storage = Ref<FieldStorage>::Create();
						storage->Name = field["Name"].as<std::string>();
						storage->Type = ToManagedFieldType(field["Type"].as<std::string>());
						fieldStorages[storage->Name] = storage;
						switch (storage->Type)
						{
							case ManagedFieldType::Bool:      storage->SetValue(field["Value"].as<bool>()); break;
							case ManagedFieldType::Char:      storage->SetValue(field["Value"].as<char16_t>()); break;
							case ManagedFieldType::Byte:      storage->SetValue(field["Value"].as<uint8_t>()); break;
							case ManagedFieldType::SByte:     storage->SetValue(field["Value"].as<int8_t>()); break;
							case ManagedFieldType::Short:     storage->SetValue(field["Value"].as<int16_t>()); break;
							case ManagedFieldType::UShort:    storage->SetValue(field["Value"].as<uint16_t>()); break;
							case ManagedFieldType::Int:       storage->SetValue(field["Value"].as<int32_t>()); break;
							case ManagedFieldType::UInt:      storage->SetValue(field["Value"].as<uint32_t>()); break;
							case ManagedFieldType::Long:      storage->SetValue(field["Value"].as<int64_t>()); break;
							case ManagedFieldType::ULong:     storage->SetValue(field["Value"].as<uint64_t>()); break;
							case ManagedFieldType::Float:     storage->SetValue(field["Value"].as<float>()); break;
							case ManagedFieldType::Double:    storage->SetValue(field["Value"].as<double>()); break;
							case ManagedFieldType::String:    storage->SetValue(field["Value"].as<std::string>()); break;
							case ManagedFieldType::Entity:    storage->SetValue(field["Value"].as<UUID>()); break;
							case ManagedFieldType::Component: storage->SetValue(field["Value"].as<UUID>()); break;
							case ManagedFieldType::Vector2:   storage->SetValue(field["Value"].as<glm::vec2>()); break;
							case ManagedFieldType::Vector3:   storage->SetValue(field["Value"].as<glm::vec3>()); break;
							case ManagedFieldType::Vector4:   storage->SetValue(field["Value"].as<glm::vec4>()); break;
							default: SK_CORE_ASSERT(false, "Unkown ManagedFieldType"); break;
						}
					}
				}
			}
		}

		return true;
	}

	bool SceneSerializer::TryLoadData(Ref<Asset>& asset, const AssetMetaData& metadata)
	{
		SK_PROFILE_FUNCTION();

		Ref<Scene> scene = Ref<Scene>::Create();
		asset = scene;
		asset->Handle = metadata.Handle;

		if (!ResourceManager::HasExistingFilePath(metadata))
		{
			asset->SetFlag(AssetFlag::FileNotFound, true);
			SK_CORE_ERROR_TAG("Serialization", "Scene file not found! {0}", metadata.FilePath);
			return false;
		}

		ScopedTimer timer("Scene Deserialization");

		try
		{
			if (!LoadScene(scene, metadata))
			{
				scene->SetFlag(AssetFlag::InvalidFile, true);
				SK_CORE_ERROR_TAG("Serialization", "Invalid Scene File");
				return false;
			}
		}
		catch (YAML::Exception& e)
		{
			scene->SetFlag(AssetFlag::InvalidFile, true);
			scene->m_Registry.clear();
			scene->m_EntityUUIDMap.clear();
			SK_CORE_ERROR_TAG("Serialization", "Failed to read Scene File! {0}", e.what());
			return false;
		}

		return true;
	}

	bool SceneSerializer::Serialize(const Ref<Asset>& asset, const AssetMetaData& metadata)
	{
		SK_PROFILE_FUNCTION();

		SK_CORE_ASSERT(asset);
		if (!asset && asset->GetAssetType() != AssetType::Scene)
			return false;

		Ref<Scene> scene = asset.As<Scene>();

		ScopedTimer timer("Scene Serialization");
		YAML::Emitter out;

		SK_CORE_INFO_TAG("Serialization", "Searializing Scene [{0}]", metadata.FilePath.generic_string());

		out << YAML::BeginMap;
		out << YAML::Key << "Scene" << YAML::Value;

		out << YAML::BeginMap;
		out << YAML::Key << "ActiveCamera" << YAML::Value << YAML::Hex << scene->GetActiveCameraUUID() << YAML::Dec;
		out << YAML::Key << "Entities" << YAML::Value;

		out << YAML::BeginSeq;
		std::map<UUID, Entity> entityIDMap = std::map(scene->m_EntityUUIDMap.begin(), scene->m_EntityUUIDMap.end());
		for (const auto& [uuid, entity] : entityIDMap)
			SerializeEntity(out, entity, scene);
		out << YAML::EndSeq;

		out << YAML::EndMap;
		out << YAML::EndMap;

		std::ofstream fout(ResourceManager::GetFileSystemPath(metadata));
		fout << out.c_str();
		fout.close();

		return true;
	}

	#pragma endregion

	#pragma region Texture

	bool TextureSerializer::TryLoadData(Ref<Asset>& asset, const AssetMetaData& metadata)
	{
		SK_PROFILE_FUNCTION();

		Ref<Texture2D> textureAsset = Texture2D::Create();
		asset = textureAsset;
		asset->Handle = metadata.Handle;

		if (!ResourceManager::HasExistingFilePath(metadata))
		{
			asset->SetFlag(AssetFlag::FileNotFound, true);
			SK_CORE_ERROR_TAG("Serialization", "Failed To Load Texture! FileNotFound {0}", metadata.FilePath);
			return false;
		}

		SK_CORE_INFO_TAG("Serialization", "Loading Texture [{0}]", metadata.FilePath.generic_string());
		ScopedTimer timer("Loading Texture");

		YAML::Node in = YAML::LoadFile(ResourceManager::GetFileSystemPath(metadata));
		auto texture = in["Texture"];
		if (!texture)
		{
			asset->SetFlag(AssetFlag::InvalidFile, true);
			SK_CORE_ERROR_TAG("Serialization", "Failed To Load Texture! Invalid Texture Asset File");
			return false;
		}

		TextureSpecification specs;

		AssetHandle sourceHandle = texture["TextureSource"].as<AssetHandle>();
		const auto& tsMetadata = ResourceManager::GetMetaData(sourceHandle);
		std::filesystem::path sourcePath = ResourceManager::GetFileSystemPath(tsMetadata);

		specs.Format = Convert::StringToImageFormat(texture["Format"].as<std::string>());
		specs.MipLevels = texture["MipLeves"].as<uint32_t>();

		auto sampler = texture["Sampler"];
		specs.Sampler.Min = Convert::StringToFilterMode(sampler["MinFilter"].as<std::string>());
		specs.Sampler.Mag = Convert::StringToFilterMode(sampler["MagFilter"].as<std::string>());
		specs.Sampler.Mip = Convert::StringToFilterMode(sampler["MipFilter"].as<std::string>());

		specs.Sampler.Wrap.U = Convert::StringToAddressMode(sampler["AddressModeU"].as<std::string>());
		specs.Sampler.Wrap.V = Convert::StringToAddressMode(sampler["AddressModeV"].as<std::string>());
		specs.Sampler.Wrap.W = Convert::StringToAddressMode(sampler["AddressModeW"].as<std::string>());

		specs.Sampler.BorderColor = sampler["BorderColor"].as<glm::vec4>();
		specs.Sampler.Anisotropy = sampler["Anisotropy"].as<bool>();
		specs.Sampler.MaxAnisotropy = sampler["MaxAnisotropy"].as<uint32_t>();
		specs.Sampler.LODBias = sampler["LODBias"].as<float>();

		TextureMetadata textureMetadata;
		TextureSourceSerializer serializer;
		if (!serializer.LoadImageData(sourcePath, textureMetadata))
			return false;

		specs.Width = textureMetadata.Width;
		specs.Height = textureMetadata.Height;
		specs.Format = textureMetadata.Format;

		textureAsset->Set(specs, textureMetadata.ImageData.Data);
		textureAsset->SetFilePath(sourcePath);

		if (specs.MipLevels != 1)
		{
			SK_CORE_TRACE_TAG("Serialization", "Generating {0} Mips", specs.MipLevels);
			Renderer::GenerateMips(textureAsset->GetImage());
		}

		textureMetadata.ImageData.Release();
		return true;
	}

	bool TextureSerializer::Serialize(const Ref<Asset>& asset, const AssetMetaData& metadata)
	{
		SK_PROFILE_FUNCTION();

		SK_CORE_ASSERT(asset, "[Texture Serialier] Asset was null");
		if (!asset)
			return false;

		SK_CORE_INFO_TAG("Serialization", "Serializing Texture [{0}]", metadata.FilePath);

		ScopedTimer timer("Texture Serialization");
		YAML::Emitter out;

		Ref<Texture2D> texture = asset.As<Texture2D>();
		const auto& specs = texture->GetSpecification();
		AssetHandle textureSourceHandle = ResourceManager::GetAssetHandleFromFilePath(texture->GetFilePath());

		out << YAML::BeginMap;
		out << YAML::Key << "Texture" << YAML::Value;

		out << YAML::BeginMap;
		out << YAML::Key << "TextureSource" << YAML::Value << textureSourceHandle;
		out << YAML::Key << "Format" << YAML::Value << EnumToString(specs.Format);
		out << YAML::Key << "MipLeves" << YAML::Value << specs.MipLevels;
		out << YAML::Key << "Sampler";

		const auto& sampler = specs.Sampler;

		out << YAML::BeginMap;
		out << YAML::Key << "MinFilter" << YAML::Value << EnumToString(sampler.Min);
		out << YAML::Key << "MagFilter" << YAML::Value << EnumToString(sampler.Mag);
		out << YAML::Key << "MipFilter" << YAML::Value << EnumToString(sampler.Mip);
		out << YAML::Key << "AddressModeU" << YAML::Value << EnumToString(sampler.Wrap.U);
		out << YAML::Key << "AddressModeV" << YAML::Value << EnumToString(sampler.Wrap.V);
		out << YAML::Key << "AddressModeW" << YAML::Value << EnumToString(sampler.Wrap.W);
		out << YAML::Key << "BorderColor" << YAML::Value << sampler.BorderColor;
		out << YAML::Key << "Anisotropy" << YAML::Value << sampler.Anisotropy;
		out << YAML::Key << "MaxAnisotropy" << YAML::Value << sampler.MaxAnisotropy;
		out << YAML::Key << "LODBias" << YAML::Value << sampler.LODBias;
		out << YAML::EndMap;

		out << YAML::EndMap;

		out << YAML::EndMap;

		if (!out.good())
		{
			SK_CORE_ERROR_TAG("Serialization", "Failed to serialize Texture! {0}", out.GetLastError());
			SK_CORE_ASSERT(false);
			return false;
		}

		std::ofstream fout(ResourceManager::GetFileSystemPath(metadata));
		fout << out.c_str();
		fout.close();

		return true;
	}

	std::filesystem::path TextureSerializer::DeserializeSourcePath(const std::filesystem::path& filePath)
	{
		SK_PROFILE_FUNCTION();

		if (!FileSystem::Exists(filePath))
		{
			SK_CORE_ERROR_TAG("Serialization", "Texture File not found! {0}", filePath);
			return std::filesystem::path{};
		}

		YAML::Node in = YAML::LoadFile(FileSystem::GetAbsolute(filePath));
		auto texture = in["Texture"];
		if (!texture)
		{
			SK_CORE_ERROR_TAG("Serialization", "Texture Node not found");
			return std::filesystem::path{};
		}

		AssetHandle sourceHandle = texture["TextureSource"].as<AssetHandle>();
		const auto& tsMetadata = ResourceManager::GetMetaData(sourceHandle);
		return ResourceManager::GetFileSystemPath(tsMetadata);
	}

	#pragma endregion

	#pragma region Texture Source

	bool TextureSourceSerializer::TryLoadData(Ref<Asset>& asset, const AssetMetaData& metadata)
	{
		Ref<TextureSource> textureSource = Ref<TextureSource>::Create();
		asset = textureSource;
		return true;
	}

	bool TextureSourceSerializer::Serialize(const Ref<Asset>& asset, const AssetMetaData& metadata)
	{
		return true;
	}

	bool TextureSourceSerializer::LoadImageData(const std::filesystem::path& filePath, TextureMetadata& outTextureMetadata)
	{
		if (!FileSystem::Exists(filePath))
			return false;

		ScopedTimer timer("Loding Image Data");

		int x, y, comp;
		std::string narrowPath = filePath.string();
		stbi_uc* data = stbi_load(narrowPath.c_str(), &x, &y, &comp, STBI_rgb_alpha);
		if (!data)
		{
			SK_CORE_ERROR_TAG("Serialization", "Failed to load Image Data: {}", stbi_failure_reason());
			return false;
		}

		outTextureMetadata.Width = x;
		outTextureMetadata.Height = y;
		outTextureMetadata.Format = ImageFormat::RGBA8;
		outTextureMetadata.ImageData = Buffer::Copy(data, (uint64_t)x * y * 4);

		stbi_image_free(data);
		return true;
	}

#pragma endregion

	#pragma region Script File

	bool ScriptFileSerializer::TryLoadData(Ref<Asset>& asset, const AssetMetaData& metadata)
	{
		Ref<ScriptFile> scriptFile = Ref<ScriptFile>::Create();
		asset = scriptFile;
		return true;
	}

	bool ScriptFileSerializer::Serialize(const Ref<Asset>& asset, const AssetMetaData& metadata)
	{
		return true;
	}

	#pragma endregion

}
