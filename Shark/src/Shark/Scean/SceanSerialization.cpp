#include "skpch.h"
#include "SceanSerialization.h"

#include "Shark/Scean/Entity.h"
#include "Shark/Scean/Components.h"

#include <yaml-cpp/yaml.h>
#include <fstream>

#include <DirectXMath.h>

namespace YAML {

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

	//// bool
	//template <>
	//struct convert<bool> {
	//	static Node encode(bool rhs) { return rhs ? Node("true") : Node("false"); }
	//
	//	YAML_CPP_API static bool decode(const Node& node, bool& rhs);
	//};
	//
	//static bool decode(const Node& node, std::pair<T, U>& rhs) {
	//	if (!node.IsSequence())
	//		return false;
	//	if (node.size() != 2)
	//		return false;

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

	SceanSerializer::SceanSerializer(const Ref<Scean>& scean)
		: m_Scean(scean)
	{
	}

	template<typename Component, typename SerializeFunction>
	void SerializeComponent(YAML::Emitter& out, Entity entity, SerializeFunction func)
	{
		if (entity.HasComponent<Component>())
		{
			out << YAML::Key << "Component" << YAML::Value << "asdfe";
			out << YAML::BeginMap;

			auto& comp = entity.GetComponent<Component>();
			func(out, comp);

			out << YAML::EndMap;
		}
	}

	static bool SerializeEntity(YAML::Emitter& out, Entity entity)
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
		out << YAML::Key << "Entity" << YAML::Value << "54758432"; // TODO: Entity ID;

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

			out << YAML::EndMap;
		}

		out << YAML::EndMap;

		return true;
	}

	bool SceanSerializer::Serialize(const std::string& filepath)
	{
		YAML::Emitter out;

		SK_CORE_TRACE("Searializing Scean to {0}", filepath);

		out << YAML::BeginMap << YAML::Key << "Scean" << YAML::Value << "Untitled";
		out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;
		
		m_Scean->m_Registry.each([&](auto& entityID)
			{
				Entity entity{ entityID, m_Scean.get() };
				SerializeEntity(out, entity);
			});


		out << YAML::EndSeq << YAML::EndMap;

		std::ofstream fout(filepath);
		fout << out.c_str();

		return true;
	}

	bool SceanSerializer::Deserialize(const std::string& filepath)
	{
		YAML::Node in = YAML::LoadFile(filepath);
		if (!in["Scean"])
			return false;

		SK_CORE_TRACE("Deserializing Scean from: {0}", filepath);

		auto entities = in["Entities"];
		if (entities)
		{
			for (auto entity : entities)
			{
				entt::entity entityID = m_Scean->m_Registry.create();
				Entity deserializedEntity = { entityID, m_Scean.get() };


				auto tagComponent = entity["TagComponent"];
				if (tagComponent)
				{
					auto tag = tagComponent["Tag"].as<std::string>();
					deserializedEntity.AddComponent<TagComponent>(tag);
					SK_CORE_TRACE("Deserializing Entity. ID: {0}, Tag: {1}", (uint32_t)entityID, tag);
				}


				auto transformComponent = entity["TransformComponent"];
				if (transformComponent)
				{
					auto position = transformComponent["Position"].as<DirectX::XMFLOAT3>();
					auto rotation = transformComponent["Rotation"].as<DirectX::XMFLOAT3>();
					auto scaling = transformComponent["Scaling"].as<DirectX::XMFLOAT3>();
					deserializedEntity.AddComponent<TransformComponent>(position, rotation, scaling);
					SK_CORE_TRACE("Added Transfrom Component");
				}

				auto spriteRendererComponent = entity["SpriteRendererComponent"];
				if (spriteRendererComponent)
				{
					auto color = spriteRendererComponent["Color"].as<DirectX::XMFLOAT4>();
					deserializedEntity.AddComponent<SpriteRendererComponent>(color);
					SK_CORE_TRACE("Added Sprite Renderer Component");
				}

				auto cameraComponent = entity["CameraComponent"];
				if (cameraComponent)
				{
					SceanCamera::Projection type = (SceanCamera::Projection)cameraComponent["Type"].as<int>();
					float aspecratio = cameraComponent["Aspecratio"].as<float>();
					float perspectiveFOV = cameraComponent["PerspectiveFOV"].as<float>();
					float perspectiveNear = cameraComponent["PerspectiveNear"].as<float>();
					float perspectiveFar = cameraComponent["PerspectiveFar"].as<float>();
					float orthographicZoom = cameraComponent["OrthographicZoom"].as<float>();
					float orthographicNear = cameraComponent["OrthographicNear"].as<float>();
					float orthographicFar = cameraComponent["OrthographicFar"].as<float>();
							
					SceanCamera sceancamera;
					sceancamera.SetPerspective(aspecratio, perspectiveFOV, perspectiveNear, perspectiveFar);
					sceancamera.SetOrthographic(aspecratio, orthographicZoom, orthographicNear, orthographicFar);

					deserializedEntity.AddComponent<CameraComponent>(sceancamera);
					SK_CORE_TRACE("Added Camera Component");
				}
			}
		}
		return true;
	}

}
