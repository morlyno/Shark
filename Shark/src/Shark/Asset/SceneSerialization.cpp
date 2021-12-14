#include "skpch.h"
#include "SceneSerialization.h"

#include "Shark/Scene/Entity.h"
#include "Shark/Scene/Components.h"
#include "Shark/Utility/YAMLUtils.h"
#include "Shark/File/FileSystem.h"

#include "Shark/Core/Timer.h"
#include "Shark/Debug/Instrumentor.h"

#include <yaml-cpp/yaml.h>
#include <fmt/format.h>
#include <fstream>
#include <DirectXMath.h>

namespace Shark {

	namespace Convert {

		static std::string BodyTypeToString(RigidBody2DComponent::BodyType bodyType)
		{
			switch (bodyType)
			{
				case RigidBody2DComponent::BodyType::Static:      return "Static";
				case RigidBody2DComponent::BodyType::Dynamic:     return "Dynamic";
				case RigidBody2DComponent::BodyType::Kinematic:   return "Kinematic";
			}

			SK_CORE_ASSERT(false, "Unkonw Body Type");
			return std::string{};
		}

		static RigidBody2DComponent::BodyType StringToBodyType(const std::string bodyType)
		{
			if (bodyType == "Static") return RigidBody2DComponent::BodyType::Static;
			if (bodyType == "Dynamic") return RigidBody2DComponent::BodyType::Dynamic;
			if (bodyType == "Kinematic") return RigidBody2DComponent::BodyType::Kinematic;

			SK_CORE_ASSERT(false, "Unkonw Body Type");
			return RigidBody2DComponent::BodyType::Static;
		}

		static std::string ProjectionToString(SceneCamera::Projection projection)
		{
			switch (projection)
			{
				case SceneCamera::Projection::Orthographic:   return "Orthographic";
				case SceneCamera::Projection::Perspective:    return "Perspective";
			}

			SK_CORE_ASSERT(false, "Unkown Projection Type");
			return std::string{};
		}

		static SceneCamera::Projection StringToProjection(const std::string& projection)
		{
			if (projection == "Orthographic")   return SceneCamera::Projection::Orthographic;
			if (projection == "Perspective")    return SceneCamera::Projection::Perspective;

			SK_CORE_WARN("Use of Lagacy mode for DeSerialization of Projection");
			if (projection == "0") return SceneCamera::Projection::Perspective;
			if (projection == "1") return SceneCamera::Projection::Orthographic;

			SK_CORE_ASSERT(false, "Unkown Projection Type");
			return SceneCamera::Projection::Orthographic;
		}

	}

	static bool SerializeEntity(YAML::Emitter& out, Entity entity, const Ref<Scene>& scene)
	{
		SK_PROFILE_FUNCTION();
		
		if (!out.good())
		{
			SK_CORE_ERROR("Bad Yaml Emitter! Skipping Entity {0}", (uint32_t)entity);
			SK_CORE_ERROR(fmt::format("Yaml Error: {}", out.GetLastError()));
			SK_CORE_ASSERT(false);
			return false;
		}

		if (!entity)
			return false;

		out << YAML::BeginMap;
		out << YAML::Key << "Entity" << YAML::Value << YAML::Hex << entity.GetUUID() << YAML::Dec;

		if (entity.HasComponent<TagComponent>())
		{
			out << YAML::Key << "TagComponent" << YAML::Value;
			out << YAML::BeginMap;

			auto& comp = entity.GetComponent<TagComponent>();
			out << YAML::Key << "Tag" << YAML::Value << comp.Tag;

			out << YAML::EndMap;

			SK_CORE_TRACE("Searializing Entity. ID: {0}, TAG: {1}", (uint32_t)entity, comp.Tag);
		}

		if (entity.HasComponent<TransformComponent>())
		{
			out << YAML::Key << "TransformComponent" << YAML::Value;
			out << YAML::BeginMap;

			auto& comp = entity.GetComponent<TransformComponent>();
			out << YAML::Key << "Position" << YAML::Value << comp.Position;
			out << YAML::Key << "Rotation" << YAML::Value << comp.Rotation;
			out << YAML::Key << "Scaling" << YAML::Value << comp.Scaling;

			out << YAML::EndMap;
		}
		
		if (entity.HasComponent<SpriteRendererComponent>())
		{
			out << YAML::Key << "SpriteRendererComponent" << YAML::Value;
			out << YAML::BeginMap;

			auto& comp = entity.GetComponent<SpriteRendererComponent>();
			out << YAML::Key << "Color" << YAML::Value << comp.Color;
			out << YAML::Key << "Texture" << YAML::Value << comp.TextureHandle;
			out << YAML::Key << "TilingFactor" << YAML::Value << comp.TilingFactor;

			out << YAML::EndMap;
		}
		
		if (entity.HasComponent<CircleRendererComponent>())
		{
			out << YAML::Key << "CircleRendererComponent" << YAML::Value;
			out << YAML::BeginMap;

			const auto& comp = entity.GetComponent<CircleRendererComponent>();
			out << YAML::Key << "Color" << YAML::Value << comp.Color;
			out << YAML::Key << "Thickness" << YAML::Value << comp.Thickness;
			out << YAML::Key << "Fade" << YAML::Value << comp.Fade;

			out << YAML::EndMap;
		}

		if (entity.HasComponent<CameraComponent>())
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

		if (entity.HasComponent<RigidBody2DComponent>())
		{
			out << YAML::Key << "RigidBody2DComponent" << YAML::Value;
			out << YAML::BeginMap;

			const auto& comp = entity.GetComponent<RigidBody2DComponent>();

			out << YAML::Key << "Type" << YAML::Value << Convert::BodyTypeToString(comp.Type);
			out << YAML::Key << "FixedRotation" << YAML::Value << comp.FixedRotation;

			out << YAML::EndMap;
		}

		if (entity.HasComponent<BoxCollider2DComponent>())
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

			out << YAML::EndMap;

		}

		if (entity.HasComponent<CircleCollider2DComponent>())
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

			out << YAML::EndMap;

		}

		out << YAML::EndMap;

		return true;
	}

	bool SceneSerializer::TryLoadData(Ref<Asset>& asset, const std::filesystem::path& filePath)
	{
		asset = Ref<Scene>::Create();
		return Deserialize(asset.As<Scene>(), filePath);
	}

	bool SceneSerializer::Serialize(Ref<Asset> asset, const std::filesystem::path& filePath)
	{
		SK_CORE_ASSERT(asset);
		if (!asset && asset->GetAssetType() != AssetType::Scene)
			return false;

		return Serialize(asset.As<Scene>(), filePath);
	}

	bool SceneSerializer::Deserialize(Ref<Asset> asset, const std::filesystem::path& filePath)
	{
		SK_CORE_ASSERT(asset);
		if (!asset && asset->GetAssetType() != AssetType::Scene)
			return false;

		return Deserialize(asset.As<Scene>(), filePath);
	}

	bool SceneSerializer::Serialize(Ref<Scene> scene, const std::filesystem::path& filepath)
	{
		SK_PROFILE_FUNCTION();

		if (!scene)
		{
			SK_CORE_WARN("SceneSerializer Scene is null but Serialize was called");
			return false;
		}

		Timer timer;

		YAML::Emitter out;

		SK_CORE_INFO("Searializing Scene to {0}", filepath);

		out << YAML::BeginMap;

		// Scene
		out << YAML::Key << "Scene" << YAML::Value << scene->Handle;

		out << YAML::Key << "ActiveCamera" << YAML::Value << YAML::Hex << scene->GetActiveCameraUUID() << YAML::Dec;

		out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;
		
		scene->m_Registry.each([&](auto& entityID)
		{
			Entity entity{ entityID, scene };
			SerializeEntity(out, entity, scene);
		});


		out << YAML::EndSeq << YAML::EndMap;

		std::ofstream fout(filepath);
		if (!fout.good())
		{
			SK_CORE_ERROR("Output File Stream Failed!");
			return false;
		}
		fout << out.c_str();
		fout.close();
		SK_CORE_INFO("Scene Serialization tock: {:.4f}ms", timer.Stop().MilliSeconds());

		return true;
	}

	bool SceneSerializer::Deserialize(Ref<Scene> scene, const std::filesystem::path& filepath)
	{
		SK_PROFILE_FUNCTION();

		if (!scene)
		{
			SK_CORE_ERROR("SceneSerializer Scene is null but Deserialize was called");
			return false;
		}

		if (!FileSystem::ValidateSceneFilePath(filepath))
		{
			SK_CORE_ERROR("Tryed to Deseialize Scene but FilePath is invalid [File: {}]", filepath);
			return false;
		}

		Timer timer;

		YAML::Node in = YAML::LoadFile(filepath);
		if (!in["Scene"])
			return false;

		SK_CORE_INFO("Deserializing Scene from: {0}", filepath);

		struct SceneUUIDFallback
		{
			operator UUID() const
			{
				SK_CORE_WARN("Deserialized Scene without UUID");
				return UUID::Generate();
			}
		};

		scene->Handle = in["Scene"].as<UUID>(SceneUUIDFallback{});

		scene->SetActiveCamera(in["ActiveCamera"].as<UUID>());

		auto entities = in["Entities"];
		if (entities)
		{
			for (auto entity : entities)
			{
				SK_PROFILE_SCOPED("SceneSerializer::Deserialize Entity");

				UUID uuid = entity["Entity"].as<uint64_t>();
				auto tagComp = entity["TagComponent"];
				auto tag = tagComp["Tag"].as<std::string>();

				Entity deserializedEntity = scene->CreateEntityWithUUID(uuid, tag);
				SK_CORE_TRACE("Deserializing Entity [{}] {:x}", tag, uuid);

				auto transformComponent = entity["TransformComponent"];
				if (transformComponent)
				{
					auto position = transformComponent["Position"];
					auto rotation = transformComponent["Rotation"];
					auto scaling = transformComponent["Scaling"];
					auto& comp = deserializedEntity.AddOrReplaceComponent<TransformComponent>();

					SK_CORE_ASSERT(position, "Couldn't deserialize TransformComponent::Position");
					comp.Position = position.as<DirectX::XMFLOAT3>();

					SK_CORE_ASSERT(rotation, "Couldn't deserialize TransformComponent::Rotation");
					comp.Rotation = rotation.as<DirectX::XMFLOAT3>();

					SK_CORE_ASSERT(scaling, "Couldn't deserialize TransformComponent::Scaling");
					comp.Scaling = scaling.as<DirectX::XMFLOAT3>();

					SK_CORE_TRACE(" - Transfrom Component");
				}

				auto spriteRendererComponent = entity["SpriteRendererComponent"];
				if (spriteRendererComponent)
				{
					auto color = spriteRendererComponent["Color"];
					auto textureFilePath = spriteRendererComponent["Texture"];
					auto tilingfactor = spriteRendererComponent["TilingFactor"];
					auto thickness = spriteRendererComponent["Thickness"];
					auto geometry = spriteRendererComponent["Geometry"];

					auto& comp = deserializedEntity.AddOrReplaceComponent<SpriteRendererComponent>();

					SK_CORE_ASSERT(color, "Couldn't deserialize SpriteRendererComponent::Color");
					comp.Color = color.as<DirectX::XMFLOAT4>();

					SK_CORE_ASSERT(textureFilePath, "Couldn't deserialize SpriteRendererComponent::Texture");
					comp.TextureHandle = textureFilePath.as<UUID>();

					SK_CORE_ASSERT(tilingfactor, "Couldn't deserialize SpriteRendererComponent::TilingFactor");
					comp.TilingFactor = tilingfactor.as<float>();

					SK_CORE_TRACE(" - Sprite Renderer Component: Texture {0}", textureFilePath);
				}

				auto circleRendererComponent = entity["CircleRendererComponent"];
				if (circleRendererComponent)
				{
					auto color = circleRendererComponent["Color"];
					auto thickness = circleRendererComponent["Thickness"];
					auto fade = circleRendererComponent["Fade"];

					auto& comp = deserializedEntity.AddOrReplaceComponent<CircleRendererComponent>();
					
					SK_CORE_ASSERT(color, "Couldn't deserialize CircleRendererCompnent::Color");
					comp.Color = color.as<DirectX::XMFLOAT4>();

					SK_CORE_ASSERT(thickness, "Couldn't deserialize CirlceRendererComponent::Thickness");
					comp.Thickness = thickness.as<float>();

					SK_CORE_ASSERT(fade, "Couldn't deserialize CirlceRendererComponent::Fade");
					comp.Fade = fade.as<float>();

					SK_CORE_TRACE(" - Cirlce Renderer Component");
				}

				auto cameraComponent = entity["CameraComponent"];
				if (cameraComponent)
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

					auto& comp = deserializedEntity.AddOrReplaceComponent<CameraComponent>(SceneCamera(projection, aspecRatio, ps, os));
					SK_CORE_TRACE(" - Camera Component: Type {}", Convert::ProjectionToString(projection));
				}

				auto rigidBody2DComponent = entity["RigidBody2DComponent"];
				if (rigidBody2DComponent)
				{
					auto type = rigidBody2DComponent["Type"];
					auto fixedRotation = rigidBody2DComponent["FixedRotation"];

					auto& comp = deserializedEntity.AddOrReplaceComponent<RigidBody2DComponent>();

					SK_CORE_ASSERT(type, "Couldn't desirialize RigidBody2DComponent::Type");
					comp.Type = Convert::StringToBodyType(type.as<std::string>());

					SK_CORE_ASSERT(fixedRotation, "Couldn't desirialize RigidBody2DComponent::FixedRotation");
					comp.FixedRotation = fixedRotation.as<bool>();

					SK_CORE_TRACE(" - RigidBody2D Component: Type {}", Convert::BodyTypeToString(comp.Type));
				}

				auto boxCollider2DComponent = entity["BoxCollider2DComponent"];
				if (boxCollider2DComponent)
				{
					auto size = boxCollider2DComponent["Size"];
					auto offset = boxCollider2DComponent["Offset"];
					auto rotation = boxCollider2DComponent["Rotation"];
					auto density = boxCollider2DComponent["Density"];
					auto friction = boxCollider2DComponent["Friction"];
					auto restitution = boxCollider2DComponent["Restitution"];
					auto restitutionThreshold = boxCollider2DComponent["RestitutionThreshold"];

					auto& comp = deserializedEntity.AddOrReplaceComponent<BoxCollider2DComponent>();

					SK_CORE_ASSERT(size, "Couldn't desirialize BoxCollider2DComponent::Size");
					comp.Size = size.as<DirectX::XMFLOAT2>();

					SK_CORE_ASSERT(offset, "Couldn't desirialize BoxCollider2DComponent::Offset");
					comp.Offset = offset.as<DirectX::XMFLOAT2>();

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

					SK_CORE_TRACE(" - BoxCollider2D Component");
				}

				auto cirlceCollider2DComponent = entity["CircleCollider2DComponent"];
				if (cirlceCollider2DComponent)
				{
					auto radius = cirlceCollider2DComponent["Radius"];
					auto offset = cirlceCollider2DComponent["Offset"];
					auto rotation = cirlceCollider2DComponent["Rotation"];
					auto density = cirlceCollider2DComponent["Density"];
					auto friction = cirlceCollider2DComponent["Friction"];
					auto restitution = cirlceCollider2DComponent["Restitution"];
					auto restitutionThreshold = cirlceCollider2DComponent["RestitutionThreshold"];

					auto& comp = deserializedEntity.AddOrReplaceComponent<CircleCollider2DComponent>();

					SK_CORE_ASSERT(radius, "Couldn't desirialize CircleCollider2DComponent::Radius");
					comp.Radius = radius.as<float>();

					SK_CORE_ASSERT(offset, "Couldn't desirialize CircleCollider2DComponent::Offset");
					comp.Offset = offset.as<DirectX::XMFLOAT2>();

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

					SK_CORE_TRACE(" - CircleCollider2D Component");
				}
			}
		}
		SK_CORE_INFO("Scene Deserialization tock: {:.4f}ms", timer.Stop().MilliSeconds());
		return true;
	}

}
