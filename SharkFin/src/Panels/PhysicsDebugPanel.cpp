#include "skfpch.h"
#include "PhysicsDebugPanel.h"

#include "Shark/Event/ApplicationEvent.h"

#include "Shark/Scene/Entity.h"
#include "Shark/Scene/Components/RigidBody2DComponent.h"

#include "Shark/UI/UI.h"

#include "Shark/Debug/Instrumentor.h"

#include <imgui.h>
#include <box2d/b2_body.h>

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

	PhysicsDebugPanel::PhysicsDebugPanel()
	{
		SK_PROFILE_FUNCTION();

	}

	void PhysicsDebugPanel::OnImGuiRender(bool& shown)
	{
		SK_PROFILE_FUNCTION();

		if (!shown)
			return;

		if (!ImGui::Begin("Physics Debug", &shown))
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
				if (ImGui::TreeNodeEx(name.c_str(), UI::DefualtTreeNodeFlags | ImGuiTreeNodeFlags_Selected))
				{
					RigidBody2DComponent& rigidBody = entity.GetComponent<RigidBody2DComponent>();
					b2Body* body = rigidBody.RuntimeBody;
					if (body)
					{
						UI::BeginControlsGrid(syncID);
						UI::Control("Type", utils::Box2DBodyTypeToString(body->GetType()));
						UI::Control("Position", fmt::to_string(body->GetPosition()));
						UI::Control("Angle", fmt::to_string(body->GetAngle()));
						UI::Control("Linear Velocity", fmt::to_string(body->GetLinearVelocity()));
						UI::Control("Angular Velocity", fmt::to_string(body->GetAngularVelocity()));
						UI::Control("Mass", fmt::to_string(body->GetMass()));
						UI::Control("IsBuller", fmt::to_string(body->IsBullet()));
						UI::Control("IsAwake", fmt::to_string(body->IsAwake()));
						UI::Control("IsEnabled", fmt::to_string(body->IsEnabled()));
						UI::EndControls();
					}
					ImGui::TreePop();
				}
				ImGui::PopID();
			}
		}

		ImGui::End();
	}

	void PhysicsDebugPanel::OnEvent(Event& event)
	{
		SK_PROFILE_FUNCTION();

		EventDispacher dispacher(event);
		dispacher.DispachEvent<SceneChangedEvent>([this](auto& event) { m_Scene = event.GetScene(); return false; });
	}

}
