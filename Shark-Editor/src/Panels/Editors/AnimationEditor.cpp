#include "skpch.h"
#include "AnimationEditor.h"

#include "Shark/Asset/AssetManager.h"

#include "Shark/UI/UICore.h"
#include "Shark/UI/Controls.h"
#include "Shark/UI/Widgets.h"

namespace Shark {

	AnimationEditor::AnimationEditor(const std::string& panelName, const AssetMetaData& metadata)
		: EditorPanel(panelName), m_Asset(metadata.Handle)
	{
	}

	void AnimationEditor::OnImGuiRender(bool& shown, bool& destroy)
	{
		if (!shown || !m_Asset)
			return;

		if (m_DockWindow)
		{
			ImGui::SetNextWindowDockID(m_DockspaceID, ImGuiCond_Always);
			m_DockWindow = false;
		}

		ImGuiWindowFlags windowFlags = 0;
		if (m_AssetDirty)
			windowFlags |= ImGuiWindowFlags_UnsavedDocument;

		if (ImGui::Begin(m_PanelName.c_str(), &shown, windowFlags))
		{
			auto animation = AssetManager::GetAssetAsync<AnimationAsset>(m_Asset);

			if (animation)
			{
				UI::BeginControlsGrid();
				if (UI::ControlAsset("Source", AssetType::MeshSource, animation->m_AnimationSource))
				{
					animation->m_SkeletonSource = animation->m_AnimationSource;
					m_AssetDirty = true;
				}

				UI::ControlAsset("Skeleton", AssetType::MeshSource, std::as_const(animation->m_SkeletonSource));

				std::span<const std::string> animations;
				if (auto meshSource = AssetManager::GetAssetAsync<MeshSource>(animation->m_AnimationSource))
					animations = meshSource->GetAnimationNames();

				m_AssetDirty |= UI::Control("Name", animation->m_Name, animations);
				UI::EndControlsGrid();
			}


			ImGui::BeginHorizontal(UI::GenerateID(), ImGui::GetContentRegionAvail(), 1.0f);

			ImGui::Spring();
			if (ImGui::Button("Save"))
			{
				Project::GetEditorAssetManager()->SaveAsset(m_Asset);
				m_AssetDirty = false;
				// #TODO popup when window closes to save asset
			}

			ImGui::EndHorizontal();
		}
		ImGui::End();

		if (!shown)
			destroy = true;
	}

	void AnimationEditor::DockWindow(ImGuiID dockspaceID)
	{
		m_DockspaceID = dockspaceID;
		m_DockWindow = true;
	}

	void AnimationEditor::SetAsset(const AssetMetaData& metadata)
	{
		m_Asset = metadata.Handle;
		AssetManager::LoadAssetAsync(metadata.Handle);
	}

	AssetHandle AnimationEditor::GetAsset() const
	{
		return m_Asset;
	}

}
