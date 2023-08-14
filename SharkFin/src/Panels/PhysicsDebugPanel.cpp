#include "skfpch.h"
#include "PhysicsDebugPanel.h"

#include "Shark/Event/ApplicationEvent.h"

#include "Shark/Scene/Entity.h"
#include "Shark/Scene/Components.h"

#include "Shark/UI/UI.h"

#include "Shark/Debug/Profiler.h"

#include <box2d/b2_body.h>
#include "box2d/b2_fixture.h"
#include "box2d/b2_joint.h"

template<typename Char>
struct fmt::formatter<b2Vec2, Char> : fmt::formatter<float, Char>
{
	template<typename FormatContext>
	auto format(const b2Vec2& vec, FormatContext& ctx) -> decltype(ctx.out())
	{
		auto&& out = ctx.out();
		format_to(out, "[");

		fmt::formatter<float, Char>::format(vec.x, ctx);
		format_to(out, ", ");
		fmt::formatter<float, Char>::format(vec.y, ctx);

		format_to(out, "]");

		return out;
	}
};

namespace Shark {

	namespace utils {

		std::string Box2DBodyTypeToString(b2BodyType bodyType)
		{
			switch (bodyType)
			{
				case b2_staticBody:     return SK_STRINGIFY(b2_staticBody);
				case b2_kinematicBody:  return SK_STRINGIFY(b2_kinematicBody);
				case b2_dynamicBody:    return SK_STRINGIFY(b2_dynamicBody);
			}
			return fmt::format("Unkown Body Type: {}", bodyType);
		}

	}

	std::string_view ToStringView(b2Shape::Type shapeType)
	{
		switch (shapeType)
		{
			case b2Shape::e_circle: return SK_STRINGIFY(e_circle);
			case b2Shape::e_edge: return SK_STRINGIFY(e_edge);
			case b2Shape::e_polygon: return SK_STRINGIFY(e_polygon);
			case b2Shape::e_chain: return SK_STRINGIFY(e_chain);
			case b2Shape::e_typeCount: return SK_STRINGIFY(e_typeCount);
		}
		SK_CORE_ASSERT(false, "Unkown b2Shape::Type");
		return "Unkown";
	}

	std::string_view ToStringView(b2JointType jointType)
	{
		switch (jointType)
		{
			case e_unknownJoint: return SK_STRINGIFY(e_unknownJoint);
			case e_revoluteJoint: return SK_STRINGIFY(e_revoluteJoint);
			case e_prismaticJoint: return SK_STRINGIFY(e_prismaticJoint);
			case e_distanceJoint: return SK_STRINGIFY(e_distanceJoint);
			case e_pulleyJoint: return SK_STRINGIFY(e_pulleyJoint);
			case e_mouseJoint: return SK_STRINGIFY(e_mouseJoint);
			case e_gearJoint: return SK_STRINGIFY(e_gearJoint);
			case e_wheelJoint: return SK_STRINGIFY(e_wheelJoint);
			case e_weldJoint: return SK_STRINGIFY(e_weldJoint);
			case e_frictionJoint: return SK_STRINGIFY(e_frictionJoint);
			case e_motorJoint: return SK_STRINGIFY(e_motorJoint);
		}

		SK_CORE_ASSERT(false, "Unkown b2JointType");
		return SK_STRINGIFY(e_unknownJoint);
	}

	PhysicsDebugPanel::PhysicsDebugPanel(const std::string& panelName)
		: Panel(panelName)
	{
	}

	void PhysicsDebugPanel::OnImGuiRender(bool& shown)
	{
		SK_PROFILE_FUNCTION();

		if (!shown)
			return;

		if (!ImGui::Begin(m_PanelName.c_str(), &shown))
		{
			ImGui::End();
			return;
		}

		if (!m_Scene)
		{
			ImGui::Text("No Active Scene");
			ImGui::End();
			return;
		}

		if (m_Scene->IsEditorScene())
		{
			ImGui::TextWrapped("Physics Debug Panel only active when the scene is playing");
			ImGui::End();
			return;
		}

		{
			ImGuiID syncID = UI::GetIDWithSeed("Controls", UI::GetCurrentID());

			auto view = m_Scene->GetAllEntitysWith<RigidBody2DComponent>();

			ImGui::Text("RigidBodies: %llu", view.size());

			for (auto entityID : view)
			{
				Entity entity{ entityID, m_Scene };
				std::string name = entity.GetName();
				ImGui::PushID((int)(uint64_t)entity.GetUUID());
				if (UI::Header(name.c_str(), UI::DefaultHeaderFlags))
				{
					RigidBody2DComponent& rigidBody = entity.GetComponent<RigidBody2DComponent>();
					b2Body* body = rigidBody.RuntimeBody;
					if (body)
					{
						UI::BeginControlsGrid(syncID);
						UI::Property("Type", utils::Box2DBodyTypeToString(body->GetType()));
						UI::Property("Position", fmt::to_string(body->GetPosition()));
						UI::Property("Angle", fmt::to_string(body->GetAngle()));
						UI::Property("Linear Velocity", fmt::format("{0:2.7f}", body->GetLinearVelocity()));
						UI::Property("Linear Damping", fmt::format("{0:2.7f}", body->GetLinearDamping()));
						UI::Property("Angular Velocity", fmt::to_string(body->GetAngularVelocity()));
						UI::Property("Mass", fmt::to_string(body->GetMass()));
						UI::Property("IsBuller", fmt::to_string(body->IsBullet()));
						UI::Property("IsAwake", fmt::to_string(body->IsAwake()));
						UI::Property("IsEnabled", fmt::to_string(body->IsEnabled()));
						UI::EndControlsGrid();

						b2Fixture* fixture = body->GetFixtureList();
						if (fixture && UI::Header("Fixture", UI::DefaultThinHeaderFlags))
						{
							UI::BeginControlsGrid(syncID);
							SK_CORE_ASSERT(fixture->GetNext() == nullptr, "Multy Fixture bodies not allowed at the moment");
							UI::Property("Type", ToStringView(fixture->GetType()));
							UI::Property("IsSensor", fixture->IsSensor());
							UI::Property("Desnity", fixture->GetDensity());
							UI::Property("Desnity", fixture->GetDensity());
							UI::Property("Restitution", fixture->GetRestitution());
							UI::Property("RestitutionThreshold", fixture->GetRestitutionThreshold());
							b2MassData massData;
							fixture->GetMassData(&massData);
							UI::Property("Mass", massData.mass);
							UI::Property("Center", glm::vec2(massData.center.x, massData.center.y));
							UI::Property("Inertia", massData.I);
							UI::EndControlsGrid();
							UI::PopHeader();
						}

						b2JointEdge* jointEdge = body->GetJointList();
						if (jointEdge && jointEdge->joint && UI::Header("Joint", UI::DefaultThinHeaderFlags))
						{
							UI::BeginControlsGrid(syncID);
							b2Joint* joint = jointEdge->joint;
							UI::Property("Type", ToStringView(joint->GetType()));
							Entity bodyAEntity = m_Scene->GetEntityByUUID((UUID)joint->GetBodyA()->GetUserData().pointer);
							Entity bodyBEntity = m_Scene->GetEntityByUUID((UUID)joint->GetBodyB()->GetUserData().pointer);
							UI::Property("Body A", bodyAEntity.GetName());
							UI::Property("Body B", bodyBEntity.GetName());
							UI::Property("Anchor A", Phyiscs2DUtils::ToGLMVec(joint->GetAnchorA()));
							UI::Property("Anchor B", Phyiscs2DUtils::ToGLMVec(joint->GetAnchorB()));
							const float invdt = 1.0f / m_Scene->GetPhysicsScene().GetTimeStep();
							UI::Property("Reaction Force", Phyiscs2DUtils::ToGLMVec(joint->GetReactionForce(invdt)));
							UI::Property("Reaction Torque", joint->GetReactionTorque(invdt));
							UI::Property("Collider Connected", joint->GetCollideConnected());
							UI::EndControlsGrid();
							UI::PopHeader();
						}
					}
					UI::PopHeader();
				}
				ImGui::PopID();
			}
		}

		ImGui::End();
	}

}
