#include "skpch.h"
#include "SceneSerialization.h"

#include "Shark/Scene/Entity.h"
#include "Shark/Scene/Components.h"
#define SK_YAMLUTILS_ALL 1
#include "Shark/Utility/YAMLUtils.h"

#include <yaml-cpp/yaml.h>
#include <fstream>

#include <DirectXMath.h>

#include "Shark/Debug/Instrumentor.h"

namespace Shark {

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

	SceneSerializer::SceneSerializer(const Ref<Scene>& scene)
		: m_Scene(scene)
	{
	}

	static bool SerializeEntity(YAML::Emitter& out, Entity entity, const Ref<Scene>& scene)
	{
		if (!out.good())
		{
#ifdef SK_DEBUG
			SK_CORE_ERROR("Bad Yaml Emitter! Skipping Entity {0}", (uint32_t)entity);
			SK_CORE_ERROR("Yaml Error:");
			SK_CORE_ERROR(out.GetLastError());
			SK_DEBUG_BREAK();
#endif
			return false;
		}

		if (!entity)
			return false;

		out << YAML::BeginMap;
		out << YAML::Key << "Entity" << YAML::Value << "54758432"; // TODO: Entity UUID;

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
			out << YAML::Key << "Texture" << YAML::Value << (comp.Texture ? comp.Texture->GetFilePath() : "");
			out << YAML::Key << "TilingFactor" << YAML::Value << comp.TilingFactor;
			out << YAML::Key << "Geometry" << YAML::Value << comp.Geometry;

			out << YAML::EndMap;
		}
		
		if (entity.HasComponent<CameraComponent>())
		{
			out << YAML::Key << "CameraComponent" << YAML::Value;
			out << YAML::BeginMap;

			auto& comp = entity.GetComponent<CameraComponent>();
			auto& cam = comp.Camera;

			out << YAML::Key << "Type" << YAML::Value << (int)cam.GetProjectionType();
			out << YAML::Key << "Aspectratio" << YAML::Value << cam.GetAspectratio();

			out << YAML::Key << "PerspectiveFOV" << YAML::Value << cam.GetPerspectiveFOV();
			out << YAML::Key << "PerspectiveNear" << YAML::Value << cam.GetPerspectiveNear();
			out << YAML::Key << "PerspectiveFar" << YAML::Value << cam.GetPerspectiveFar();

			out << YAML::Key << "OrthographicZoom" << YAML::Value << cam.GetOrthographicZoom();
			out << YAML::Key << "OrthographicNear" << YAML::Value << cam.GetOrthographicNear();
			out << YAML::Key << "OrthographicFar" << YAML::Value << cam.GetOrthographicFar();

			bool mainCamera = entity == scene->GetActiveCamera();
			out << YAML::Key << "IsMainCamera" << YAML::Value << mainCamera;

			out << YAML::EndMap;
		}

		if (entity.HasComponent<RigidBody2DComponent>())
		{
			out << YAML::Key << "RigidBody2DComponent" << YAML::Value;
			out << YAML::BeginMap;

			const auto& comp = entity.GetComponent<RigidBody2DComponent>();

			out << YAML::Key << "Type" << YAML::Value << BodyTypeToString(comp.Type);
			out << YAML::Key << "FixedRotation" << YAML::Value << comp.FixedRotation;

			out << YAML::EndMap;
		}

		if (entity.HasComponent<BoxCollider2DComponent>())
		{
			out << YAML::Key << "BoxCollider2DComponent" << YAML::Value;
			out << YAML::BeginMap;

			const auto& comp = entity.GetComponent<BoxCollider2DComponent>();

			out << YAML::Key << "Size" << YAML::Value << comp.Size;
			out << YAML::Key << "Offset" << YAML::Value << comp.LocalOffset;
			out << YAML::Key << "Rotation" << YAML::Value << comp.LocalRotation;
			out << YAML::Key << "Density" << YAML::Value << comp.Density;
			out << YAML::Key << "Friction" << YAML::Value << comp.Friction;
			out << YAML::Key << "Restitution" << YAML::Value << comp.Restitution;
			out << YAML::Key << "RestitutionThreshold" << YAML::Value << comp.RestitutionThreshold;

			out << YAML::EndMap;

		}

		out << YAML::EndMap;

		return true;
	}

	bool SceneSerializer::Serialize(const std::filesystem::path& filepath)
	{
		SK_PROFILE_FUNCTION();
			
		YAML::Emitter out;

		SK_CORE_INFO("==========================================================================================");
		SK_CORE_INFO("Searializing Scene to {0}", filepath);

		out << YAML::BeginMap << YAML::Key << "Scene" << YAML::Value << "Untitled";
		out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;
		
		m_Scene->m_Registry.each([&](auto& entityID)
		{
			Entity entity{ entityID, m_Scene };
			SerializeEntity(out, entity, m_Scene);
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
		SK_CORE_INFO("==========================================================================================");

		return true;
	}

	bool SceneSerializer::Deserialize(const std::filesystem::path& filepath)
	{
		SK_PROFILE_FUNCTION();

		YAML::Node in = YAML::LoadFile(filepath);
		if (!in["Scene"])
			return false;

		SK_CORE_INFO("==========================================================================================");
		SK_CORE_INFO("Deserializing Scene from: {0}", filepath);

		auto entities = in["Entities"];
		if (entities)
		{
			for (auto entity : entities)
			{
				entt::entity entityID = m_Scene->m_Registry.create();
				Entity deserializedEntity = { entityID, Weak(m_Scene) };

				SK_CORE_TRACE("Deserializing Entity ID: {0}", (uint32_t)entityID);

				auto tagComponent = entity["TagComponent"];
				if (tagComponent)
				{
					auto tag = tagComponent["Tag"];
					SK_CORE_VERIFY(tag, "Couldn't deserialize TagComponent::Tag")
					auto& comp = deserializedEntity.AddComponent<TagComponent>();
					comp.Tag = tag.as<std::string>("Unkown Deserialized Entity");
					SK_CORE_TRACE(" - Tag Compoenent: {0}", tag);
				}


				auto transformComponent = entity["TransformComponent"];
				if (transformComponent)
				{
					auto position = transformComponent["Position"];
					auto rotation = transformComponent["Rotation"];
					auto scaling = transformComponent["Scaling"];
					auto& comp = deserializedEntity.AddComponent<TransformComponent>();

					SK_CORE_VERIFY(position, "Couldn't deserialize TransformComponent::Position");
					if (position)
						comp.Position = position.as<DirectX::XMFLOAT3>();

					SK_CORE_VERIFY(rotation, "Couldn't deserialize TransformComponent::Rotation");
					if (rotation)
						comp.Rotation = rotation.as<DirectX::XMFLOAT3>();

					SK_CORE_VERIFY(scaling, "Couldn't deserialize TransformComponent::Scaling");
					if (scaling)
						comp.Scaling = scaling.as<DirectX::XMFLOAT3>();

					SK_CORE_TRACE(" - Transfrom Component");
				}

				auto spriteRendererComponent = entity["SpriteRendererComponent"];
				if (spriteRendererComponent)
				{
					auto color = spriteRendererComponent["Color"];
					auto textureFilePath = spriteRendererComponent["Texture"];
					auto tilingfactor = spriteRendererComponent["TilingFactor"];
					auto geometry = spriteRendererComponent["Geometry"];

					auto& comp = deserializedEntity.AddComponent<SpriteRendererComponent>();

					SK_CORE_VERIFY(color, "Couldn't deserialize SpriteRendererComponent::Color");
					if (color)
						comp.Color = color.as<DirectX::XMFLOAT4>();

					SK_CORE_VERIFY(textureFilePath, "Couldn't deserialize SpriteRendererComponent::Texture")
					if (textureFilePath)
					{
						auto texFilePath = textureFilePath.as<std::filesystem::path>();
						if (!texFilePath.empty())
							comp.Texture = Texture2D::Create(texFilePath);
					}

					SK_CORE_VERIFY(tilingfactor, "Couldn't deserialize SpriteRendererComponent::TilingFactor");
					if (tilingfactor)
						comp.TilingFactor = tilingfactor.as<float>();

					SK_CORE_VERIFY(geometry, "Couldn't deserialize SpriteRendererComponent::Geometry");
					if (geometry)
						comp.Geometry = geometry.as<SpriteRendererComponent::GeometryType>();


					SK_CORE_TRACE(" - Sprite Renderer Component: Texture {0}", textureFilePath);
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
					SK_CORE_VERIFY(type, "Couldn't deserialize CameraComponent::Projection");
					if (type)
						projection = type.as<SceneCamera::Projection>();

					SK_CORE_VERIFY(aspectratio, "Couldn't deserialize CameraComponent::AspectRatio");
					if (aspectratio)
						aspecRatio = aspectratio.as<float>();

					SK_CORE_VERIFY(perspectiveFOV, "Couldn't deserialize CameraComponent::PerspectiveFOV");
					if (perspectiveFOV)
						ps.FOV = perspectiveFOV.as<float>();

					SK_CORE_VERIFY(perspectiveNear, "Couldn't deserialize CameraComponent::PerspectiveNear");
					if (perspectiveNear)
						ps.Near = perspectiveNear.as<float>();

					SK_CORE_VERIFY(perspectiveFar, "Couldn't deserialize CameraComponent::PerspectiveFar");
					if (perspectiveFar)
						ps.Far = perspectiveFar.as<float>();

					SK_CORE_VERIFY(orthographicZoom, "Couldn't deserialize CameraComponent::OrthographicZoom");
					if (orthographicZoom)
						os.Zoom = orthographicZoom.as<float>();

					SK_CORE_VERIFY(orthographicNear, "Couldn't deserialize CameraComponent::OrthographicNear");
					if (orthographicNear)
						os.Near = orthographicNear.as<float>();

					SK_CORE_VERIFY(orthographicNear, "Couldn't deserialize CameraComponent::OrthographicNear");
					if (orthographicNear)
						os.Far = orthographicFar.as<float>();

					SK_CORE_VERIFY(isMainCamera, "Couldn't deserialize IsMainCamera");
					if (isMainCamera)
						mainCam = isMainCamera.as<bool>();

					auto& comp = deserializedEntity.AddComponent<CameraComponent>();
					comp.Camera = SceneCamera(projection, aspecRatio, ps, os);
					if (mainCam)
						m_Scene->m_ActiveCameraID = deserializedEntity;
					SK_CORE_TRACE(" - Camera Component: Type {}, MainCamera {}", projection == SceneCamera::Projection::Perspective ? "Perspective" : "Othographic", mainCam);
				}

				auto rigidBody2DComponent = entity["RigidBody2DComponent"];
				if (rigidBody2DComponent)
				{
					auto type = rigidBody2DComponent["Type"];
					auto fixedRotation = rigidBody2DComponent["FixedRotation"];

					auto& comp = deserializedEntity.AddComponent<RigidBody2DComponent>();

					SK_CORE_VERIFY(type, "Couldn't desirialize RigidBody2DComponent::Type");
					if (type)
						comp.Type = StringToBodyType(type.as<std::string>());

					SK_CORE_VERIFY(fixedRotation, "Couldn't desirialize RigidBody2DComponent::FixedRotation");
					if (fixedRotation)
						comp.FixedRotation = fixedRotation.as<bool>();

					SK_CORE_TRACE(" - RigidBody2D Component: Type {}", BodyTypeToString(comp.Type));
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

					auto& comp = deserializedEntity.AddComponent<BoxCollider2DComponent>();

					SK_CORE_VERIFY(size, "Couldn't desirialize BoxCollider2DComponent::Size");
					if (size)
						comp.Size = size.as<DirectX::XMFLOAT2>();

					SK_CORE_VERIFY(offset, "Couldn't desirialize BoxCollider2DComponent::Offset");
					if (offset)
						comp.LocalOffset = offset.as<DirectX::XMFLOAT2>();

					SK_CORE_VERIFY(rotation, "Couldn't desirialize BoxCollider2DComponent::Rotation");
					if (rotation)
						comp.LocalRotation = rotation.as<float>();

					SK_CORE_VERIFY(density, "Couldn't desirialize BoxCollider2DComponent::Density");
					if (density)
						comp.Density = density.as<float>();

					SK_CORE_VERIFY(friction, "Couldn't desirialize BoxCollider2DComponent::Friction");
					if (friction)
						comp.Friction = friction.as<float>();

					SK_CORE_VERIFY(restitution, "Couldn't desirialize BoxCollider2DComponent::Restitution");
					if (restitution)
						comp.Restitution = restitution.as<float>();

					SK_CORE_VERIFY(restitutionThreshold, "Couldn't desirialize BoxCollider2DComponent::RestitutionThreshold");
					if (restitutionThreshold)
						comp.RestitutionThreshold = restitutionThreshold.as<float>();

					SK_CORE_TRACE(" - BoxCollider2D Component");
				}
			}
		}
		SK_CORE_INFO("==========================================================================================");
		return true;
	}

}
