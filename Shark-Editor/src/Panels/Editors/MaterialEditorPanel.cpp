#include "MaterialEditorPanel.h"

#include "Shark/Core/SelectionManager.h"
#include "Shark/Asset/AssetManager.h"

#include "Shark/File/FileSystem.h"

#include "Shark/UI/UICore.h"
#include "Shark/UI/Theme.h"
#include "Shark/UI/EditorResources.h"

#include "Shark/Debug/Profiler.h"

#include <glm/gtc/type_ptr.hpp>

namespace Shark {

	  ///////////////////////////////////////////////////////////////////////////////////////////////
	 /// Material Editor ///////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////

	MaterialEditor::MaterialEditor(const std::string& name, AssetHandle assetHandle)
		: m_Name(name), m_MaterialHandle(assetHandle)
	{
	}

	void MaterialEditor::Draw()
	{
		ImGui::Begin(m_Name.c_str(), nullptr, ImGuiWindowFlags_NoFocusOnAppearing);
		DrawInline();
		ImGui::End();
	}

	void MaterialEditor::DrawInline()
	{
		if (!m_MaterialHandle)
			return;

		UI::ScopedItemFlag readOnly(ImGuiItemFlags_ReadOnly, m_Readonly);

		const ImVec2 textureSize = { 64, 64 };

		AsyncLoadResult materialResult = AssetManager::GetAssetAsync<MaterialAsset>(m_MaterialHandle);
		if (!materialResult.Ready)
			return;

		Ref<MaterialAsset> material = materialResult;

		ImGui::Text(fmt::format("Shader: {}", material->GetMaterial()->GetShader()->GetName()));
		ImGui::Text(fmt::format("Name: {}", material->GetMaterial()->GetName()));

		bool changed = false;

		if (ImGui::CollapsingHeader("Albedo", ImGuiTreeNodeFlags_DefaultOpen))
		{
			bool hasTexture = true;
			Ref<Texture2D> displayTexture = AssetManager::GetAssetAsync<Texture2D>(material->GetAlbedoMap());
			if (!displayTexture)
			{
				displayTexture = EditorResources::AlphaBackground;
				hasTexture = false;
			}

			if (UI::TextureEdit("albedo texture", displayTexture, textureSize, hasTexture))
			{
				// uncomment if the AlphaBackground texture ever shows up
				if (displayTexture/* && hasTexture*/)
					material->SetAlbedoMap(displayTexture->Handle);
				else
					material->ClearAlbedoMap();

				changed = true;
			}

			ImGui::SameLine();
			ImGui::ColorEdit3("Color", glm::value_ptr(material->GetAlbedoColor()), ImGuiColorEditFlags_NoInputs);
			if (ImGui::IsItemDeactivatedAfterEdit())
				changed = true;
		}

		if (ImGui::CollapsingHeader("Normal", ImGuiTreeNodeFlags_DefaultOpen))
		{
			bool hasTexture = true;
			Ref<Texture2D> displayTexture = AssetManager::GetAssetAsync<Texture2D>(material->GetNormalMap());
			if (!displayTexture)
			{
				displayTexture = EditorResources::AlphaBackground;
				hasTexture = false;
			}

			if (UI::TextureEdit("normal texture", displayTexture, textureSize, hasTexture))
			{
				if (displayTexture)
					material->SetNormalMap(displayTexture->Handle);
				else
					material->ClearNormalMap();

				changed = true;
			}

			ImGui::SameLine();

			bool usingNormalMap = material->IsUsingNormalMap();
			if (ImGui::Checkbox("Enable", &usingNormalMap))
			{
				material->SetUsingNormalMap(usingNormalMap);
				changed = true;
			}
		}

		if (ImGui::CollapsingHeader("Metalness", ImGuiTreeNodeFlags_DefaultOpen))
		{
			bool hasTexture = true;
			Ref<Texture2D> displayTexture = AssetManager::GetAssetAsync<Texture2D>(material->GetMetalnessMap());
			if (!displayTexture)
			{
				displayTexture = EditorResources::AlphaBackground;
				hasTexture = false;
			}

			if (UI::TextureEdit("metalness texture", displayTexture, textureSize, hasTexture))
			{
				if (displayTexture)
					material->SetMetalnessMap(displayTexture->Handle);
				else
					material->ClearMetalnessMap();

				changed = true;
			}

			ImGui::SameLine();
			ImGui::SliderFloat("Metalness Value", &material->GetMetalness(), 0.0f, 1.0f);
			if (ImGui::IsItemDeactivatedAfterEdit())
				changed = true;
		}

		if (ImGui::CollapsingHeader("Roughness", ImGuiTreeNodeFlags_DefaultOpen))
		{
			bool hasTexture = true;
			Ref<Texture2D> displayTexture = AssetManager::GetAssetAsync<Texture2D>(material->GetRoughnessMap());
			if (!displayTexture)
			{
				displayTexture = EditorResources::AlphaBackground;
				hasTexture = false;
			}

			if (UI::TextureEdit("roughness texture", displayTexture, textureSize, hasTexture))
			{
				if (displayTexture)
					material->SetRoughnessMap(displayTexture->Handle);
				else
					material->ClearRoughnessMap();

				changed = true;
			}

			ImGui::SameLine();
			ImGui::SliderFloat("Roughness Value", &material->GetRoughness(), 0.0f, 1.0f);
			if (ImGui::IsItemDeactivatedAfterEdit())
				changed = true;
		}

		if (changed)
		{
			Project::GetEditorAssetManager()->SaveAsset(m_MaterialHandle);
		}

	}

	  ///////////////////////////////////////////////////////////////////////////////////////////////
	 /// Material Editor Panel /////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////

	MaterialEditorPanel::MaterialEditorPanel(const std::string& panelName, const AssetMetaData& metadata)
		: EditorPanel(panelName)
	{
		m_MaterialEditor = Scope<MaterialEditor>::Create("", metadata.Handle);
		SetAsset(metadata);
	}

	MaterialEditorPanel::~MaterialEditorPanel()
	{
		if (AssetManager::IsValidAssetHandle(m_Sphere))
			AssetManager::DeleteMemoryAsset(m_Sphere);
	}

	void MaterialEditorPanel::SetAsset(const AssetMetaData& metadata)
	{
		if (!AssetManager::IsValidAssetHandle(metadata.Handle))
			return;

		const glm::vec2 DefaultViewportSize = { 1280.0f, 720.0f };
		if (m_ViewportSize.x == 0.0f || m_ViewportSize.y == 0.0f)
			m_ViewportSize = DefaultViewportSize;

		m_MaterialHandle = metadata.Handle;
		m_MaterialEditor->SetMaterial(metadata.Handle);

		m_Scene = Ref<Scene>::Create();
		m_Scene->SetViewportSize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
		SceneRendererSpecification specification;
		m_Renderer = Ref<SceneRenderer>::Create(m_Scene);

		if (!AssetManager::IsValidAssetHandle(m_Sphere))
		{
			AssetHandle sphereSourceHandle = Project::GetEditorAssetManager()->GetEditorAsset("Resources/Meshes/Default/Sphere.gltf");
			m_Sphere = AssetManager::CreateMemoryOnlyAsset<Mesh>(sphereSourceHandle);
		}

		Ref<Mesh> sphere = AssetManager::GetAsset<Mesh>(m_Sphere);

		Entity entity = m_Scene->CreateEntity("Sphere");
		auto& meshComp = entity.AddComponent<StaticMeshComponent>();
		meshComp.StaticMesh = sphere->Handle;
		meshComp.MaterialTable->SetMaterial(0, m_MaterialHandle);

		Entity lightEntity = m_Scene->CreateEntity("Light");
		lightEntity.Transform().Translation = { -4.0f, 3.0f, -4.0f };
		lightEntity.AddComponent<PointLightComponent>();

		Entity skyEntity = m_Scene->CreateEntity("Sky");
		auto& skyComp = skyEntity.AddComponent<SkyComponent>();
		skyComp.SceneEnvironment = Project::GetEditorAssetManager()->GetEditorAsset("Resources/Environment/lenong_2_4k.hdr");
		skyComp.Intensity = 0.8f;
		skyComp.Lod = 4.5f;

		Entity cameraEntity = m_Scene->CreateEntity("Camera");
		cameraEntity.AddComponent<CameraComponent>(true);
		cameraEntity.Transform().Translation.z = -5.0f;

		m_Scene->SetActiveCamera(cameraEntity.GetUUID());
	}

	void MaterialEditorPanel::DockWindow(ImGuiID dockspace)
	{
		m_DockWindowID = dockspace;
		m_DockWindow = true;
	}

	void MaterialEditorPanel::OnUpdate(TimeStep ts)
	{
		SK_PROFILE_FUNCTION();

		if (!m_Active || !m_MaterialHandle)
			return;

		if (m_NeedsResize && m_ViewportSize.x != 0 && m_ViewportSize.y != 0)
		{
			m_Scene->SetViewportSize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
			m_Renderer->Resize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
			m_NeedsResize = false;
		}

		m_Scene->OnRenderRuntime(m_Renderer);
	}

	void MaterialEditorPanel::OnImGuiRender(bool& shown, bool& destroy)
	{
		if (!shown || !m_MaterialHandle)
			return;

		if (!m_Active)
		{
			shown = false;
			destroy = true;
			return;
		}

		if (m_DockWindow)
		{
			ImGui::SetNextWindowDockID(m_DockWindowID, ImGuiCond_Always);
			m_DockWindow = false;
		}

		if (ImGui::Begin(m_PanelName.c_str(), &m_Active))
		{
			if (ImGui::BeginTable("MaterialEditor", 2, ImGuiTableFlags_Resizable))
			{
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);

				ImVec2 size = ImGui::GetContentRegionAvail();
				if (size.x <= 0 || size.y <= 0)
					size = { (float)m_ViewportSize.x, (float)m_ViewportSize.y };

				if ((float)m_ViewportSize.x != size.x || (float)m_ViewportSize.y != size.y)
				{
					m_ViewportSize.x = size.x;
					m_ViewportSize.y = size.y;
					m_NeedsResize = true;
				}

				Ref<Image2D> finalImage = m_Renderer->GetFinalPassImage();
				UI::Image(finalImage, { (float)finalImage->GetWidth(), (float)finalImage->GetHeight() });

				ImGui::TableSetColumnIndex(1);
				m_MaterialEditor->DrawInline();

				if (ImGui::Button("Save"))
				{
					Project::GetEditorAssetManager()->SaveAsset(m_MaterialHandle);
				}

				ImGui::EndTable();
			}
		}
		ImGui::End();

	}

	MaterialPanel::MaterialPanel(const std::string& panelName)
		: Panel(panelName)
	{
		m_MaterialEditor = Scope<MaterialEditor>::Create();
	}

	MaterialPanel::~MaterialPanel()
	{
	}

	void MaterialPanel::OnImGuiRender(bool& isShown)
	{
		ImGui::Begin(m_PanelName.c_str(), &isShown);

		Entity nextEntity;
		if (SelectionManager::AnySelected(m_Context->GetID()))
		{
			UUID lastSelectedID = SelectionManager::GetLastSelected(m_Context->GetID());
			Entity lastSelected = m_Context->TryGetEntityByUUID(lastSelectedID);
			// TODO(moro): Add MeshFilterComponent when the RootEntityID got fixed
			if (lastSelected && lastSelected.HasAny<MeshComponent, SubmeshComponent, StaticMeshComponent>())
			{
				nextEntity = lastSelected;
			}
		}

		if (nextEntity != m_SelectedEntity)
		{
			m_SelectedEntity = nextEntity;
			m_MaterialEditor->SetMaterial(AssetHandle::Invalid);
		}

		bool selectedMaterialValid = false;
		if (m_SelectedEntity && ImGui::BeginListBox("##materialList", { ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight() * 5.0f }))
		{
			const auto listMaterials = [this, &selectedMaterialValid](Ref<MaterialTable> meshMaterialTable, Ref<MaterialTable> overrideTable, AssetHandle overrideHandle, uint32_t overrideSlot)
			{
				for (uint32_t slot = 0; slot < meshMaterialTable->GetSlotCount(); slot++)
				{
					AssetHandle material;
					bool readonly = true;

					if (overrideTable && overrideTable->HasMaterial(slot))
					{
						material = overrideTable->GetMaterial(slot);
						readonly = false;
					}
					else if (!overrideTable && overrideSlot == slot)
					{
						material = overrideHandle;
						readonly = false;
					}

					if (!material && meshMaterialTable->HasMaterial(slot))
					{
						material = meshMaterialTable->GetMaterial(slot);
						readonly = true;
					}

					// Select first valid material
					if (material && !m_MaterialEditor->GetMaterial())
					{
						m_MaterialEditor->SetMaterial(material);
						m_MaterialEditor->SetReadonly(readonly);
					}

					const auto& name = GetMaterialName(material);
					UI::ScopedDisabled disabled(!material);
					UI::ScopedColorConditional textBrighter(ImGuiCol_Text, UI::Colors::Theme::TextBrighter, !readonly);
					if (ImGui::Selectable(name.c_str(), material == m_MaterialEditor->GetMaterial()))
					{
						m_MaterialEditor->SetMaterial(material);
						m_MaterialEditor->SetReadonly(readonly);
					}

					if (material == m_MaterialEditor->GetMaterial())
						selectedMaterialValid = true;
				}
			};

			if (m_SelectedEntity.HasComponent<StaticMeshComponent>())
			{
				const auto& component = m_SelectedEntity.GetComponent<StaticMeshComponent>();
				if (auto mesh = AssetManager::GetAssetAsync<Mesh>(component.StaticMesh))
					listMaterials(mesh->GetMaterials(), component.MaterialTable, AssetHandle::Invalid, -1);
			}
			else if (m_SelectedEntity.HasComponent<MeshComponent>())
			{
				const auto& component = m_SelectedEntity.GetComponent<MeshComponent>();
				if (auto mesh = AssetManager::GetAssetAsync<Mesh>(component.Mesh))
					listMaterials(mesh->GetMaterials(), nullptr, AssetHandle::Invalid, -1);
			}
			else if (m_SelectedEntity.HasComponent<SubmeshComponent>())
			{
				const auto& component = m_SelectedEntity.GetComponent<SubmeshComponent>();
				if (auto mesh = AssetManager::GetAssetAsync<Mesh>(component.Mesh))
				{
					auto meshSource = AssetManager::GetAsset<MeshSource>(mesh->GetMeshSource());
					if (meshSource->HasSubmesh(component.SubmeshIndex))
					{
						const auto& submesh = meshSource->GetSubmesh(component.SubmeshIndex);
						listMaterials(mesh->GetMaterials(), nullptr, component.Material, submesh.MaterialIndex);
					}
				}
			}
			ImGui::EndListBox();
		}

		if (!selectedMaterialValid)
			m_MaterialEditor->SetMaterial(AssetHandle::Invalid);

		m_MaterialEditor->DrawInline();

		if (ImGui::BeginPopupContextWindow())
		{
			bool readonly = m_MaterialEditor->IsReadonly();
			if (ImGui::Checkbox("Readonly", &readonly))
				m_MaterialEditor->SetReadonly(readonly);

			ImGui::EndPopup();
		}

		ImGui::End();
	}

	std::string MaterialPanel::GetMaterialName(AssetHandle handle) const
	{
		if (!handle)
			return "";

		Ref<MaterialAsset> material = AssetManager::GetAsset<MaterialAsset>(handle);
		auto& name = material->GetName();
		if (!name.empty())
			return name;

		if (AssetManager::IsMemoryAsset(handle))
			return fmt::to_string(handle);

		const auto& metadata = Project::GetEditorAssetManager()->GetMetadata(handle);
		return FileSystem::GetStemString(metadata.FilePath);
	}

}
