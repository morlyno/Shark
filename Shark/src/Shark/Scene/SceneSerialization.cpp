#include "skpch.h"
#include "SceneSerialization.h"

#include "Shark/Scene/Entity.h"
#include "Shark/Scene/Components/Components.h"
#define SK_YAMLUTILS_ALL 1
#include "Shark/Utility/YAMLUtils.h"

#include <yaml-cpp/yaml.h>
#include <fstream>

#include <DirectXMath.h>

#include "Shark/Debug/Instrumentor.h"

namespace Shark {

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

		if (entity.HasComponent<RigidBodyComponent>())
		{
			out << YAML::Key << "RigidBodyComponent" << YAML::Value;
			out << YAML::BeginMap;

			auto& comp = entity.GetComponent<RigidBodyComponent>();
			auto& body = comp.Body;

			out << YAML::Key << "Type" << YAML::Value << (int)body.GetType();
			out << YAML::Key << "AllowSleep" << YAML::Value << body.IsSleepingAllowed();
			out << YAML::Key << "Awake" << YAML::Value << body.IsAwake();
			out << YAML::Key << "Enabled" << YAML::Value << body.IsEnabled();
			out << YAML::Key << "FixedRotation" << YAML::Value << body.IsFixedRoation();
			out << YAML::Key << "Position" << YAML::Value << body.GetPosition();
			out << YAML::Key << "Angle" << YAML::Value << body.GetAngle();

			out << YAML::EndMap;

		}

		if (entity.HasComponent<BoxColliderComponent>())
		{
			out << YAML::Key << "ColliderComponent" << YAML::Value;
			out << YAML::BeginMap;

			auto& comp = entity.GetComponent<BoxColliderComponent>();
			auto& collider = comp.Collider;

			out << YAML::Key << "Shape" << YAML::Value << (int)collider.GetShape();
			out << YAML::Key << "Friction" << YAML::Value << collider.GetFriction();
			out << YAML::Key << "Density" << YAML::Value << collider.GetDensity();
			out << YAML::Key << "Restitution" << YAML::Value << collider.GetRestituion();
			out << YAML::Key << "Size" << YAML::Value << collider.GetSize();

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
						comp.Geometry = geometry.as<Geometry>();


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

				auto rigidbodyComponent = entity["RigidBodyComponent"];
				if (rigidbodyComponent)
				{
					auto type = rigidbodyComponent["Type"];
					auto allowSleep = rigidbodyComponent["AllowSleep"];
					auto awake = rigidbodyComponent["Awake"];
					auto enabled = rigidbodyComponent["Enabled"];
					auto fixedRotation = rigidbodyComponent["FixedRotation"];
					auto position = rigidbodyComponent["Position"];
					auto angle = rigidbodyComponent["Angle"];

					RigidBodySpecs specs;

					SK_CORE_VERIFY(type, "Couldn't deserialize RigigBodyComponent::Type");
					if (type)
						specs.Type = type.as<Shark::BodyType>();

					SK_CORE_VERIFY(allowSleep, "Couldn't deserialize RigigBodyComponent::AllowSleep");
					if (allowSleep)
						specs.AllowSleep = allowSleep.as<bool>();
					
					SK_CORE_VERIFY(awake, "Couldn't deserialize RigigBodyComponent::Awake");
					if (awake)
						specs.Awake = awake.as<bool>();

					SK_CORE_VERIFY(enabled, "Couldn't deserialize RigigBodyComponent::Enabled");
					if (enabled)
						specs.Enabled = enabled.as<bool>();

					SK_CORE_VERIFY(fixedRotation, "Couldn't deserialize RigigBodyComponent::FixedRotation");
					if (fixedRotation)
						specs.FixedRotation = fixedRotation.as<bool>();

					SK_CORE_VERIFY(position, "Couldn't deserialize RigigBodyComponent::Position");
					if (position)
						specs.Position = position.as<DirectX::XMFLOAT2>();

					SK_CORE_VERIFY(angle, "Couldn't deserialize RigigBodyComponent::Angle");
					if (angle)
						specs.Angle = angle.as<float>();

					auto& comp = deserializedEntity.AddComponent<RigidBodyComponent>();
					comp.Body.SetState(specs);
					SK_CORE_TRACE(" - RigidBody Component: Type {}", specs.Type == BodyType::Static ? "Static" : "Dinamic");
				}

				auto colliderComponent = entity["ColliderComponent"];
				if (colliderComponent)
				{
					auto shape = colliderComponent["Shape"];
					auto friction = colliderComponent["Friction"];
					auto density = colliderComponent["Density"];
					auto restitution = colliderComponent["Restitution"];
					auto size = colliderComponent["Size"];
					
					ColliderSpecs specs;

					SK_CORE_VERIFY(shape, "Couldn't deserialize ColliderComponent::Shape");
					if (shape)
						specs.Shape = shape.as<ShapeType>();

					SK_CORE_VERIFY(friction, "Couldn't deserialize ColliderComponent::Firction");
					if (friction)
						specs.Friction = friction.as<float>();

					SK_CORE_VERIFY(density, "Couldn't deserialize ColliderComponent::Density");
					if (density)
						specs.Density = density.as<float>();

					SK_CORE_VERIFY(restitution, "Couldn't deserialize ColliderComponent::Restitution");
					if (restitution)
						specs.Restitution = restitution.as<float>();

					SK_CORE_VERIFY(size, "Couldn't deserialize ColliderComponent::Size");
					if (size)
					{
						auto [width, height] = size.as<DirectX::XMFLOAT2>();
						specs.Width = width;
						specs.Height = height;
					}

					auto& comp = deserializedEntity.AddComponent<BoxColliderComponent>();
					comp.Collider.SetState(specs);
					SK_CORE_TRACE(" - Collider Component: Type Box");
				}
			}
		}
		SK_CORE_INFO("==========================================================================================");
		return true;
	}

}
