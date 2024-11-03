#include "PhysicsDebugPanel.h"

#include "Shark/Event/ApplicationEvent.h"

#include "Shark/Scene/Entity.h"
#include "Shark/Scene/Components.h"

#include "Shark/UI/UICore.h"
#include "Shark/UI/Controls.h"

#include "Shark/Debug/Profiler.h"

#include <box2d/b2_body.h>
#include "box2d/b2_fixture.h"
#include "box2d/b2_joint.h"

template<typename Char>
struct fmt::formatter<b2Vec2, Char> : fmt::formatter<float, Char>
{
	template<typename FormatContext>
	auto format(const b2Vec2& vec, FormatContext& ctx) const -> decltype(ctx.out())
	{
		auto&& out = ctx.out();
		detail::write(out, "[");

		fmt::formatter<float, Char>::format(vec.x, ctx);
		detail::write(out, ", ");
		fmt::formatter<float, Char>::format(vec.y, ctx);

		detail::write(out, "]");

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
			auto view = m_Scene->GetAllEntitysWith<RigidBody2DComponent>();

			ImGui::Text("RigidBodies: %llu", view.size());

			for (auto entityID : view)
			{
				Entity entity{ entityID, m_Scene };
				const std::string& name = entity.Name();
				ImGui::PushID((int)(uint64_t)entity.GetUUID());
				if (ImGui::TreeNodeEx(name.c_str(), UI::DefaultHeaderFlags))
				{
					RigidBody2DComponent& rigidBody = entity.GetComponent<RigidBody2DComponent>();
					b2Body* body = rigidBody.RuntimeBody;
					if (body)
					{
						UI::BeginControlsGrid();
						UI::Control("Type", utils::Box2DBodyTypeToString(body->GetType()));
						UI::Control("Position", fmt::to_string(body->GetPosition()));
						UI::Control("Angle", fmt::to_string(body->GetAngle()));
						UI::Control("Linear Velocity", fmt::format("{0:2.7f}", body->GetLinearVelocity()));
						UI::Control("Linear Damping", fmt::format("{0:2.7f}", body->GetLinearDamping()));
						UI::Control("Angular Velocity", fmt::to_string(body->GetAngularVelocity()));
						UI::Control("Mass", fmt::to_string(body->GetMass()));
						UI::Control("IsBuller", fmt::to_string(body->IsBullet()));
						UI::Control("IsAwake", fmt::to_string(body->IsAwake()));
						UI::Control("IsEnabled", fmt::to_string(body->IsEnabled()));

						b2Fixture* fixture = body->GetFixtureList();
						if (fixture && UI::ControlHeader("Fixture"))
						{
							SK_CORE_ASSERT(fixture->GetNext() == nullptr, "Multy Fixture bodies not allowed at the moment");
							UI::Control("Type", fmt::to_string(fixture->GetType()));
							UI::Control("IsSensor", fmt::to_string(fixture->IsSensor()));
							UI::Control("Density", fmt::to_string(fixture->GetDensity()));
							UI::Control("Density", fmt::to_string(fixture->GetDensity()));
							UI::Control("Restitution", fmt::to_string(fixture->GetRestitution()));
							UI::Control("RestitutionThreshold", fmt::to_string(fixture->GetRestitutionThreshold()));
							b2MassData massData;
							fixture->GetMassData(&massData);
							UI::Control("Mass", fmt::to_string(massData.mass));
							UI::Control("Center", fmt::to_string(glm::vec2(massData.center.x, massData.center.y)));
							UI::Control("Inertia", fmt::to_string(massData.I));
							ImGui::TreePop();
						}

						b2JointEdge* jointEdge = body->GetJointList();
						if (jointEdge && jointEdge->joint && UI::ControlHeader("Joint"))
						{
							b2Joint* joint = jointEdge->joint;
							UI::Control("Type", fmt::to_string(joint->GetType()));
							Entity bodyAEntity = m_Scene->TryGetEntityByUUID((UUID)joint->GetBodyA()->GetUserData().pointer);
							Entity bodyBEntity = m_Scene->TryGetEntityByUUID((UUID)joint->GetBodyB()->GetUserData().pointer);
							UI::Control("Body A", (const std::string&)bodyAEntity.GetName());
							UI::Control("Body B", (const std::string&)bodyBEntity.GetName());
							UI::Control("Anchor A", fmt::to_string(Phyiscs2DUtils::ToGLMVec(joint->GetAnchorA())));
							UI::Control("Anchor B", fmt::to_string(Phyiscs2DUtils::ToGLMVec(joint->GetAnchorB())));
							const float invdt = 1.0f / m_Scene->GetPhysicsScene().GetTimeStep();
							UI::Control("Reaction Force", fmt::to_string(Phyiscs2DUtils::ToGLMVec(joint->GetReactionForce(invdt))));
							UI::Control("Reaction Torque", fmt::to_string(joint->GetReactionTorque(invdt)));
							UI::Control("Collider Connected", fmt::to_string(joint->GetCollideConnected()));
							ImGui::TreePop();
						}
						UI::EndControlsGrid();
					}
					ImGui::TreePop();
				}
				ImGui::PopID();
			}
		}

		ImGui::End();
	}

}
