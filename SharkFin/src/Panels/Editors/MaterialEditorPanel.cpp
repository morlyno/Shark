#include "skfpch.h"
#include "MaterialEditorPanel.h"

#include "Shark/Asset/AssetManager.h"
#include "Shark/Render/Renderer.h"

#include "Shark/UI/UI.h"
#include "Shark/UI/Theme.h"

#include "Shark/Debug/Profiler.h"

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
		ImGui::Begin(m_Name.c_str());
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

		UI::Text("Shader: {}", material->GetMaterial()->GetShader()->GetName());
		UI::Text("Name: {}", material->GetMaterial()->GetName());

		bool changed = false;

		if (ImGui::CollapsingHeader("Albedo", ImGuiTreeNodeFlags_DefaultOpen))
		{
			bool hasTexture = true;
			Ref<Texture2D> displayTexture = AssetManager::GetAssetAsync<Texture2D>(material->GetAlbedoMap());
			if (!displayTexture)
			{
				AssetHandle handle = Project::GetActiveEditorAssetManager()->GetEditorAsset("Resources/Textures/NoImagePlaceholder.sktex");
				displayTexture = AssetManager::GetAssetAsync<Texture2D>(handle);
				hasTexture = false;
			}

			if (UI::TextureEdit("albedo texture", displayTexture, textureSize, hasTexture))
			{
				if (displayTexture)
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
				AssetHandle handle = Project::GetActiveEditorAssetManager()->GetEditorAsset("Resources/Textures/NoImagePlaceholder.sktex");
				displayTexture = AssetManager::GetAssetAsync<Texture2D>(handle);
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
				AssetHandle handle = Project::GetActiveEditorAssetManager()->GetEditorAsset("Resources/Textures/NoImagePlaceholder.sktex");
				displayTexture = AssetManager::GetAssetAsync<Texture2D>(handle);
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
				AssetHandle handle = Project::GetActiveEditorAssetManager()->GetEditorAsset("Resources/Textures/NoImagePlaceholder.sktex");
				displayTexture = AssetManager::GetAssetAsync<Texture2D>(handle);
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
			Project::GetActiveEditorAssetManager()->SaveAsset(m_MaterialHandle);
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
			AssetHandle sphereSourceHandle = Project::GetActiveEditorAssetManager()->GetEditorAsset("Resources/Meshes/Default/Sphere.gltf");
			m_Sphere = AssetManager::CreateMemoryOnlyAsset<Mesh>(sphereSourceHandle);
		}

		Ref<Mesh> sphere = AssetManager::GetAsset<Mesh>(m_Sphere);

		Entity entity = m_Scene->CreateEntity("Sphere");
		MeshComponent& meshComp = entity.AddComponent<MeshComponent>();
		meshComp.Mesh = sphere->Handle;
		meshComp.Material = m_MaterialHandle;
		meshComp.SubmeshIndex = 0;

		Entity lightEntity = m_Scene->CreateEntity("Light");
		lightEntity.Transform().Translation = { -4.0f, 3.0f, -4.0f };
		lightEntity.AddComponent<PointLightComponent>();

		Entity skyEntity = m_Scene->CreateEntity("Sky");
		auto& skyComp = skyEntity.AddComponent<SkyComponent>();
		skyComp.SceneEnvironment = Project::GetActiveEditorAssetManager()->GetEditorAsset("Resources/Environment/lenong_2_4k.hdr");
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
					ImGuiWindow* window = ImGui::GetCurrentWindow();
					m_ViewportSize.x = ImGui::GetContentRegionAvail().x;
					m_ViewportSize.y = ImGui::GetContentRegionAvail().y;
					m_NeedsResize = true;
				}

				Ref<Image2D> finalImage = m_Renderer->GetFinalPassImage();
				UI::Image(finalImage, { (float)finalImage->GetWidth(), (float)finalImage->GetHeight() });

				ImGui::TableSetColumnIndex(1);
				m_MaterialEditor->DrawInline();

				if (ImGui::Button("Save"))
				{
					Project::GetActiveEditorAssetManager()->SaveAsset(m_MaterialHandle);
				}

				ImGui::EndTable();
			}
		}
		ImGui::End();

	}

}
