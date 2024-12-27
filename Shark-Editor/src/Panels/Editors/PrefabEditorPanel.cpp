#include "skpch.h"
#include "PrefabEditorPanel.h"

#include "Shark/Core/SelectionManager.h"
#include "Shark/Asset/AssetManager.h"
#include "Shark/Input/Input.h"
#include "Shark/UI/UICore.h"
#include "Shark/UI/Widgets.h"
#include "Shark/Utils/std.h"
#include "EditorSettings.h"

namespace Shark {

	PrefabEditorPanel::PrefabEditorPanel(const std::string& panelName, const AssetMetaData& metadata)
		: EditorPanel(panelName)
	{
		m_DefaultEnvironment = Project::GetActiveEditorAssetManager()->GetEditorAsset("Resources/Environment/lenong_2_4k.hdr");
		SetAsset(metadata);
	}

	PrefabEditorPanel::~PrefabEditorPanel()
	{
	}

	void PrefabEditorPanel::OnUpdate(TimeStep ts)
	{
		if (m_NeedsResize)
		{
			m_Prefab->GetScene()->SetViewportSize(m_ViewportWidth, m_ViewportHeight);
			m_SceneRenderer->Resize(m_ViewportWidth, m_ViewportHeight);
			m_EditorCamera.Resize(m_ViewportWidth, m_ViewportHeight);
			m_NeedsResize = false;
		}

		m_EditorCamera.OnUpdate(ts, m_ViewportHovered || m_ViewportFocused && Input::GetCursorMode() == CursorMode::Locked);

		Ref<Scene> prefabScene = m_Prefab->GetScene();
		prefabScene->OnRenderEditor(m_SceneRenderer, m_EditorCamera);
	}

	void PrefabEditorPanel::OnImGuiRender(bool& shown, bool& destroy)
	{
		if (m_DockWindow)
			ImGui::SetNextWindowDockID(m_DockSpaceID);

		if (!m_Prefab)
			return;

		bool wasShown = shown;
		if (ImGui::Begin(m_PanelName.c_str(), &shown, ImGuiWindowFlags_MenuBar))
		{
			if (ImGui::BeginMenuBar())
			{
				if (ImGui::BeginMenu("File"))
				{
					if (ImGui::MenuItem("Save"))
					{
						PrepareForSerialization();
						Project::GetActiveEditorAssetManager()->SaveAsset(m_Handle);
					}
					
					if (ImGui::MenuItem("Default Sky", nullptr, &m_UseDefaultSky))
					{
						Ref<Scene> scene = m_Prefab->GetScene();
						scene->SetFallbackEnvironment(m_UseDefaultSky ? m_DefaultEnvironment : AssetHandle());
					}
					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("Edit"))
				{
					ImGui::MenuItem("Set Active Camera", nullptr, &m_Prefab->m_SetActiveCamera);
					ImGui::SetItemTooltip("Should the camera be set as active when instantiating this Prefab");



					ImGui::Separator();

					if (ImGui::MenuItem("Fix Prefab Handles"))
					{
						auto entities = m_Prefab->GetScene()->GetAllEntitysWith<PrefabComponent>();
						for (auto ent : entities)
							entities.get<PrefabComponent>(ent).Prefab = m_Prefab->Handle;
					}

					ImGui::EndMenu();
				}

				ImGui::EndMenuBar();
			}

			if (ImGui::BeginTable("##prefabEditorTable", 2, ImGuiTableFlags_Resizable))
			{
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);

				if (ImGui::BeginChild("##PrefabEditor-Hierarchy", ImVec2(0, 0), ImGuiChildFlags_ResizeY))
				{
					m_HierarchyPanel->OnImGuiRender(shown);
				}
				ImGui::EndChild();
				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImGui::GetStyle().WindowPadding * 0.5f);
				if (ImGui::BeginChild("##PrefaEditor-Properties", ImVec2(0, 0), ImGuiChildFlags_AlwaysUseWindowPadding))
				{
					Ref<Scene> scene = m_HierarchyPanel->GetContext();
					if (SelectionManager::AnySelected(scene->GetID()))
					{
						auto view = SelectionManager::GetSelections(scene->GetID()) |
							std::views::transform([scene](UUID uuid) { return scene->TryGetEntityByUUID(uuid); });
						m_HierarchyPanel->DrawEntityProperties({ view.begin(), view.end() });
					}
				}
				ImGui::EndChild();
				ImGui::PopStyleVar();

				ImGui::TableSetColumnIndex(1);
				if (ImGui::BeginChild("##PrefabEditor-Viewport"))
				{
					ImVec2 contentRegion = ImGui::GetContentRegionAvail();
					if (contentRegion.x != m_ViewportWidth || contentRegion.y != m_ViewportHeight)
					{
						m_ViewportWidth = contentRegion.x;
						m_ViewportHeight = contentRegion.y;
						m_NeedsResize = true;
					}

					UI::Image(m_SceneRenderer->GetFinalPassImage(), contentRegion);
					if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
						ImGui::SetWindowFocus();

					m_ViewportFocused = ImGui::IsWindowFocused();
					m_ViewportHovered = ImGui::IsWindowHovered();
				}
				ImGui::EndChild();

				ImGui::EndTable();
			}
		}

		if (wasShown != shown)
			destroy = true;

		ImGui::End();
	}

	void PrefabEditorPanel::OnEvent(Event& event)
	{
		m_EditorCamera.OnEvent(event);
		m_HierarchyPanel->OnEvent(event);
	}

	void PrefabEditorPanel::DockWindow(ImGuiID dockspaceID)
	{
		m_DockSpaceID = dockspaceID;
		m_DockWindow = true;
	}

	void PrefabEditorPanel::SetAsset(const AssetMetaData& metadata)
	{
		m_Handle = metadata.Handle;
		m_Prefab = AssetManager::GetAsset<Prefab>(metadata.Handle);
		if (!m_Prefab)
			return;

		if (ImGuiWindow* window = ImGui::FindWindowByName("##PrefabEditor-Viewport"))
		{
			m_ViewportWidth = (uint32_t)window->Size.x;
			m_ViewportHeight = (uint32_t)window->Size.y;
		}

		Ref<Scene> prefabScene = m_Prefab->GetScene();
		prefabScene->SetViewportSize(m_ViewportWidth, m_ViewportHeight);

		m_SceneRenderer = Ref<SceneRenderer>::Create(prefabScene);
		m_HierarchyPanel = Ref<SceneHierarchyPanel>::Create("Prefab Editor", prefabScene, false);
		m_HierarchyPanel->RegisterEntityCreatedCallback([this](Entity entity)
		{
			entity.AddComponent<PrefabComponent>(m_Prefab->Handle, entity.GetUUID());
			if (!m_Prefab->HasValidRoot())
				m_Prefab->SetRootEntity(entity);
		});
		m_HierarchyPanel->RegisterEntityDestroyedCallback([this](Entity entity)
		{
			if (m_Prefab->GetRootEntityID() == entity.GetUUID())
				m_Prefab->ClearRootEntity();
		});

		if (m_UseDefaultSky)
			prefabScene->SetFallbackEnvironment(m_DefaultEnvironment);
	}

	AssetHandle PrefabEditorPanel::GetAsset() const
	{
		return m_Handle;
	}

	void PrefabEditorPanel::PrepareForSerialization() const
	{
		Ref<Scene> prefabScene = m_Prefab->GetScene();
		auto entities = prefabScene->GetAllEntitysWith<PrefabComponent>();
		if (entities.empty())
		{
			SK_CONSOLE_WARN("Serializing empty Prefab\n{}", Project::GetActiveEditorAssetManager()->GetMetadata(m_Handle).FilePath);
			return;
		}

		auto rootEntities = entities |
			std::views::transform([prefabScene](entt::entity ent) { return Entity(ent, prefabScene); }) |
			std::views::filter([](Entity entity) { return !entity.HasParent(); }) |
			std::ranges::to<std::vector>();

		if (EditorSettings::Get().Prefab_AutoGroupRootEntities && rootEntities.size() > 1)
		{
			Entity root = prefabScene->CreateEntity("Root");
			root.AddComponent<PrefabComponent>(m_Handle, root.GetUUID());
			SK_CONSOLE_WARN("Prefab has more then one root entity. Grouping all root entities under one Entity '{}'", root.GetUUID());

			for (Entity entity : rootEntities)
				root.AddChild(entity);

			rootEntities.clear();
			rootEntities.push_back(root);
			m_Prefab->SetRootEntity(root);
		}

		if (!m_Prefab->HasValidRoot())
		{
			Entity rootEntity = rootEntities.front();
			SK_CONSOLE_WARN("Prefab has an invalid root! Replacing current '{}' with '{}'", m_Prefab->GetRootEntityID(), rootEntity.GetUUID());
			m_Prefab->SetRootEntity(rootEntity);
		}

	}

}
