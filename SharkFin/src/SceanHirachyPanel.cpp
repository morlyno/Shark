#include "SceanHirachyPanel.h"

#include <Shark/Scean/Components.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <entt.hpp>

namespace Shark {

	SceanHirachyPanel::SceanHirachyPanel(Ref<Scean> context)
		: m_Context(context)
	{
	}

	void SceanHirachyPanel::SetContext(Ref<Scean> context)
	{
		m_Context = context;
	}

	void SceanHirachyPanel::OnImGuiRender()
	{
		ImGui::Begin("Scean Hirachy");

		m_Context->m_Registry.each([&](auto entityID)
		{
			Entity entity{ entityID, m_Context.get() };
			DrawEntityNode(entity);
		});

		if (ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered())
			m_SelectedEntity = {};

		if (ImGui::BeginPopupContextWindow("Add Entity Popup",ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
		{
			if (ImGui::Selectable("Add Entity"))
			{
				m_Context->CreateEntity("New Entity");
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		ImGui::End();

		if (m_SelectedEntity)
			DrawEntityProperties(m_SelectedEntity);

	}

	void SceanHirachyPanel::DrawEntityNode(Entity entity)
	{
		const auto& tag = entity.GetComponent<TagComponent>();
		constexpr auto trenodefalgs = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
		bool opened = ImGui::TreeNodeEx((void*)(uintptr_t)(uint32_t)entity, trenodefalgs, tag.Tag.c_str());

		if (ImGui::IsItemClicked())
			m_SelectedEntity = entity;

		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::Selectable("Delete Entity"))
			{
				m_Context->DestroyEntity(entity);
				m_SelectedEntity = {};
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		if (opened)
		{
			// Draw Child Entitys
			ImGui::TreePop();
		}
	}

	static void DrawVec3Control(const char* lable, DirectX::XMFLOAT3& vec, float resetVal, const char* fmt)
	{
		ImGui::PushID(lable);

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, 100);

		ImGui::Text(lable);
		ImGui::NextColumn();

		const ImVec2 buttonSize = { 19, 19 };
		const float itemWidth = ImGui::GetColumnWidth() / 3.0f - buttonSize.x - 8.0f;
		constexpr ImGuiSliderFlags sliderFalgs = ImGuiSliderFlags_NoRoundToFormat;

		ImGui::PushItemWidth(itemWidth);

		if (ImGui::Button("X", buttonSize))
			vec.x = resetVal;
		ImGui::SameLine(0.0f, 0.0f);
		ImGui::DragFloat("##X", &vec.x, 1.0f, 0.0f, 0.0f, fmt, sliderFalgs);

		ImGui::SameLine();
		if (ImGui::Button("Y", buttonSize))
			vec.y = resetVal;
		ImGui::SameLine(0.0f, 0.0f);
		ImGui::DragFloat("##Y", &vec.y, 1.0f, 0.0f, 0.0f, fmt, sliderFalgs);

		ImGui::SameLine();
		if (ImGui::Button("Z", buttonSize))
			vec.z = resetVal;
		ImGui::SameLine(0.0f, 0.0f);
		ImGui::DragFloat("##Z", &vec.z, 1.0f, 0.0f, 0.0f, fmt, sliderFalgs);

		ImGui::Columns();

		ImGui::PopID();
	}
	
	template<typename Comp, typename UIFunction>
	static void DrawComponet(Entity entity, const char* lable, UIFunction func)
	{
		if (entity.HasComponent<Comp>())
		{
			ImGui::PushID((void*)typeid(Comp).hash_code());
			bool opened = ImGui::CollapsingHeader(lable, ImGuiTreeNodeFlags_AllowItemOverlap);
			ImGui::SameLine(ImGui::GetWindowContentRegionWidth() + 16 - 23);
			bool removed = ImGui::Button("x", { 19, 19 });

			if (opened)
			{
				auto& comp = entity.GetComponent<Comp>();
				func(comp);
			}

			if (removed)
				entity.RemoveComponent<Comp>();
			ImGui::PopID();
		}
	}
	
	void SceanHirachyPanel::DrawEntityProperties(Entity entity)
	{
		ImGui::Begin("Properties");

		const float AddButtonWidth = ImGui::CalcTextSize("Add", NULL, false).x + ImGui::GetStyle().FramePadding.x * 2.0f;

		auto& tag = entity.GetComponent<TagComponent>();
		char buf[128];
		strcpy_s(buf, tag.Tag.c_str());
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth() - AddButtonWidth - 8);
		if (ImGui::InputText("##Tag", buf, std::size(buf)))
			tag.Tag = buf;

		ImGui::SameLine();
		if (ImGui::Button("Add"))
			ImGui::OpenPopup("Add Component List");
		if (ImGui::BeginPopup("Add Component List"))
		{
			if (ImGui::Selectable("Transform", false, entity.HasComponent<TransformComponent>() ? ImGuiSelectableFlags_Disabled : 0))
			{
				entity.AddComponent<TransformComponent>();
				ImGui::CloseCurrentPopup();
			}
			if (ImGui::Selectable("Sprite Renderer", false, entity.HasComponent<SpriteRendererComponent>() ? ImGuiSelectableFlags_Disabled : 0))
			{
				entity.AddComponent<SpriteRendererComponent>();
				ImGui::CloseCurrentPopup();
			}
			if (ImGui::Selectable("Scean Camera", false, entity.HasComponent<CameraComponent>() ? ImGuiSelectableFlags_Disabled : 0))
			{
				entity.AddComponent<CameraComponent>();
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		DrawComponet<TransformComponent>(entity, "Transform", [](auto& comp)
			{
				DrawVec3Control("Position", comp.Position, 0.0f, "%.2f");
				DirectX::XMFLOAT3 rotation = { DirectX::XMConvertToDegrees(comp.Rotation.x), DirectX::XMConvertToDegrees(comp.Rotation.y), DirectX::XMConvertToDegrees(comp.Rotation.z) };
				DrawVec3Control("Rotation", rotation, 0.0f, "%.2f");
				comp.Rotation = { DirectX::XMConvertToRadians(rotation.x), DirectX::XMConvertToRadians(rotation.y), DirectX::XMConvertToRadians(rotation.z) };
				DrawVec3Control("Scaling", comp.Scaling, 1.0f, "%.2f");
			});

		DrawComponet<SpriteRendererComponent>(entity, "SptrieRenderer", [](auto& comp)
			{
				ImGui::ColorEdit4("Color", &comp.Color.x);
			});

		DrawComponet<CameraComponent>(entity, "Scean Camera", [](auto& comp)
			{
				ImGui::Checkbox("Primary", &comp.Primary);
			});

		ImGui::End();
	}

}
