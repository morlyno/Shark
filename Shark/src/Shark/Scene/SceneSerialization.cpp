#include "skpch.h"
#include "SceneSerialization.h"

#include "Shark/Scene/Entity.h"
#include "Shark/Scene/Components/Components.h"

#include <yaml-cpp/yaml.h>
#include <fstream>

#include <DirectXMath.h>

namespace YAML {

	template<>
	struct convert<DirectX::XMFLOAT2>
	{
		static bool decode(const Node& node, DirectX::XMFLOAT2& f2)
		{
			if (!node.IsSequence() || node.size() != 2)
				return false;

			f2.x = node[0].as<float>();
			f2.y = node[1].as<float>();

			return true;
		}
	};

	template<>
	struct convert<DirectX::XMFLOAT3>
	{
		static bool decode(const Node& node, DirectX::XMFLOAT3& f3)
		{
			if (!node.IsSequence() || node.size() != 3)
				return false;

			f3.x = node[0].as<float>();
			f3.y = node[1].as<float>();
			f3.z = node[2].as<float>();

			return true;
		}
	};

	template<>
	struct convert<DirectX::XMFLOAT4>
	{
		static bool decode(const Node& node, DirectX::XMFLOAT4& f4)
		{
			if (!node.IsSequence() || node.size() != 4)
				return false;

			f4.x = node[0].as<float>();
			f4.y = node[1].as<float>();
			f4.z = node[2].as<float>();
			f4.w = node[3].as<float>();

			return true;
		}
	};

}

namespace Shark {

	YAML::Emitter& operator<<(YAML::Emitter& out, const DirectX::XMFLOAT2& f2)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << f2.x << f2.y << YAML::EndSeq;
		return out;
	}

	YAML::Emitter& operator<<(YAML::Emitter& out, const DirectX::XMFLOAT3& f3)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << f3.x << f3.y << f3.z << YAML::EndSeq;
		return out;
	}

	YAML::Emitter& operator<<(YAML::Emitter& out, const DirectX::XMFLOAT4& f4)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << f4.x << f4.y << f4.z << f4.w << YAML::EndSeq;
		return out;
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

			out << YAML::EndMap;
		}
		
		if (entity.HasComponent<CameraComponent>())
		{
			out << YAML::Key << "CameraComponent" << YAML::Value;
			out << YAML::BeginMap;

			auto& comp = entity.GetComponent<CameraComponent>();
			auto& cam = comp.Camera;

			out << YAML::Key << "Type" << YAML::Value << (int)cam.GetProjectionType();
			out << YAML::Key << "Aspecratio" << YAML::Value << cam.GetAspectratio();

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

	bool SceneSerializer::Serialize(const std::string& filepath)
	{
		YAML::Emitter out;

		SK_CORE_TRACE("Searializing Scene to {0}", filepath);

		out << YAML::BeginMap << YAML::Key << "Scene" << YAML::Value << "Untitled";
		out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;
		
		m_Scene->m_Registry.each([&](auto& entityID)
		{
			Entity entity{ entityID, Weak(m_Scene) };
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

		return true;
	}

	bool SceneSerializer::Deserialize(const std::string& filepath)
	{
		YAML::Node in = YAML::LoadFile(filepath);
		if (!in["Scene"])
			return false;

		SK_CORE_TRACE("Deserializing Scene from: {0}", filepath);

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
					auto tag = tagComponent["Tag"].as<std::string>();
					auto& comp = deserializedEntity.AddComponent<TagComponent>();
					comp.Tag = tag;
					SK_CORE_TRACE(" - Tag Compoenent: {0}", tag);
				}


				auto transformComponent = entity["TransformComponent"];
				if (transformComponent)
				{
					auto position = transformComponent["Position"].as<DirectX::XMFLOAT3>();
					auto rotation = transformComponent["Rotation"].as<DirectX::XMFLOAT3>();
					auto scaling = transformComponent["Scaling"].as<DirectX::XMFLOAT3>();
					auto& comp = deserializedEntity.AddComponent<TransformComponent>();
					comp = { position, rotation, scaling };
					SK_CORE_TRACE(" - Transfrom Component");
				}

				auto spriteRendererComponent = entity["SpriteRendererComponent"];
				if (spriteRendererComponent)
				{
					auto color = spriteRendererComponent["Color"].as<DirectX::XMFLOAT4>();
					auto textureFilePath = spriteRendererComponent["Texture"].as<std::string>();
					auto tilingfactor = spriteRendererComponent["TilingFactor"].as<float>();
					auto& comp = deserializedEntity.AddComponent<SpriteRendererComponent>();
					comp.Color = color;
					comp.Texture = textureFilePath.empty() ? nullptr : Texture2D::Create(textureFilePath);
					comp.TilingFactor = tilingfactor;
					SK_CORE_TRACE(" - Sprite Renderer Component: Texture {0}", textureFilePath);
				}

				auto cameraComponent = entity["CameraComponent"];
				if (cameraComponent)
				{
					SceneCamera::Projection type = (SceneCamera::Projection)cameraComponent["Type"].as<int>();
					float aspecratio = cameraComponent["Aspecratio"].as<float>();
					float perspectiveFOV = cameraComponent["PerspectiveFOV"].as<float>();
					float perspectiveNear = cameraComponent["PerspectiveNear"].as<float>();
					float perspectiveFar = cameraComponent["PerspectiveFar"].as<float>();
					float orthographicZoom = cameraComponent["OrthographicZoom"].as<float>();
					float orthographicNear = cameraComponent["OrthographicNear"].as<float>();
					float orthographicFar = cameraComponent["OrthographicFar"].as<float>();
					bool isMainCamera = cameraComponent["IsMainCamera"].as<bool>();
							
					SceneCamera Scenecamera;
					Scenecamera.SetPerspective(aspecratio, perspectiveFOV, perspectiveNear, perspectiveFar);
					Scenecamera.SetOrthographic(aspecratio, orthographicZoom, orthographicNear, orthographicFar);
					Scenecamera.SetProjectionType(type);

					auto& comp = deserializedEntity.AddComponent<CameraComponent>();
					comp.Camera = Scenecamera;
					if (isMainCamera)
						m_Scene->m_ActiveCameraID = deserializedEntity;
					SK_CORE_TRACE(" - Camera Component: Type {0}, MainCamera {0}", type == SceneCamera::Projection::Perspective ? "Perspective" : "Othographic", isMainCamera);
				}

				auto rigidbodyComponent = entity["RigidBodyComponent"];
				if (rigidbodyComponent)
				{
					RigidBodySpecs specs;
					specs.Type = (BodyType)rigidbodyComponent["Type"].as<int>();
					specs.AllowSleep = rigidbodyComponent["AllowSleep"].as<bool>();
					specs.Awake = rigidbodyComponent["Awake"].as<bool>();
					specs.Enabled = rigidbodyComponent["Enabled"].as<bool>();
					specs.FixedRotation = rigidbodyComponent["FixedRotation"].as<bool>();
					specs.Position = rigidbodyComponent["Position"].as<DirectX::XMFLOAT2>();
					specs.Angle = rigidbodyComponent["Angle"].as<float>();
				
					auto& comp = deserializedEntity.AddComponent<RigidBodyComponent>();
					comp.Body.SetState(specs);
					SK_CORE_TRACE(" - RigidBody Component: Type {0}", specs.Type == BodyType::Static ? "Static" : "Dinamic");
				}

				auto colliderComponent = entity["ColliderComponent"];
				if (colliderComponent)
				{
					ColliderSpecs specs;
					specs.Shape = (ShapeType)colliderComponent["Shape"].as<int>();
					specs.Friction = colliderComponent["Friction"].as<float>();
					specs.Density = colliderComponent["Density"].as<float>();
					specs.Restitution = colliderComponent["Restitution"].as<float>();
					auto [width, height] = colliderComponent["Size"].as<DirectX::XMFLOAT2>();
					specs.Width = width;
					specs.Height = height;

					auto& comp = deserializedEntity.AddComponent<BoxColliderComponent>();
					comp.Collider.SetState(specs);
					SK_CORE_TRACE(" - Collider Component: Type Box");
				}
			}
		}
		return true;
	}

}
