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

static auto format_as(b2Vec2 v2) { return glm::vec2(v2.x, v2.y); }

namespace Shark {

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
						UI::Control("Type", magic_enum::enum_name(body->GetType()));
						UI::Control("Position", fmt::to_string(body->GetPosition()));
						UI::Control("Angle", fmt::to_string(body->GetAngle()));
						UI::Control("Linear Velocity", fmt::format("{0:2.7f}", body->GetLinearVelocity()));
						UI::Control("Linear Damping", fmt::format("{0:2.7f}", body->GetLinearDamping()));
						UI::Control("Angular Velocity", fmt::to_string(body->GetAngularVelocity()));
						UI::Control("Mass", fmt::to_string(body->GetMass()));
						UI::Control("IsBuller", body->IsBullet());
						UI::Control("IsAwake", body->IsAwake());
						UI::Control("IsEnabled", body->IsEnabled());

						b2Fixture* fixture = body->GetFixtureList();
						if (fixture && UI::ControlHeader("Fixture"))
						{
							SK_CORE_ASSERT(fixture->GetNext() == nullptr, "Multy Fixture bodies not allowed at the moment");
							UI::Control("Type", magic_enum::enum_name(fixture->GetType()));
							UI::Control("IsSensor", fixture->IsSensor());
							UI::Control("Density", fmt::to_string(fixture->GetDensity()));
							UI::Control("Density", fmt::to_string(fixture->GetDensity()));
							UI::Control("Restitution", fmt::to_string(fixture->GetRestitution()));
							UI::Control("RestitutionThreshold", fmt::to_string(fixture->GetRestitutionThreshold()));
							b2MassData massData;
							fixture->GetMassData(&massData);
							UI::Control("Mass", fmt::to_string(massData.mass));
							UI::Control("Center", fmt::to_string(massData.center));
							UI::Control("Inertia", fmt::to_string(massData.I));
							ImGui::TreePop();
						}

						b2JointEdge* jointEdge = body->GetJointList();
						if (jointEdge && jointEdge->joint && UI::ControlHeader("Joint"))
						{
							b2Joint* joint = jointEdge->joint;
							UI::Control("Type", magic_enum::enum_name(joint->GetType()));
							Entity bodyAEntity = m_Scene->TryGetEntityByUUID((UUID)joint->GetBodyA()->GetUserData().pointer);
							Entity bodyBEntity = m_Scene->TryGetEntityByUUID((UUID)joint->GetBodyB()->GetUserData().pointer);
							UI::Control("Body A", (const std::string&)bodyAEntity.GetName());
							UI::Control("Body B", (const std::string&)bodyBEntity.GetName());
							UI::Control("Anchor A", fmt::to_string(joint->GetAnchorA()));
							UI::Control("Anchor B", fmt::to_string(joint->GetAnchorB()));
							const float invdt = 1.0f / m_Scene->GetPhysicsScene().GetTimeStep();
							UI::Control("Reaction Force", fmt::to_string(joint->GetReactionForce(invdt)));
							UI::Control("Reaction Torque", fmt::to_string(joint->GetReactionTorque(invdt)));
							UI::Control("Collider Connected", joint->GetCollideConnected());
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
