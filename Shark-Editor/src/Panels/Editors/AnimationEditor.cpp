#include "skpch.h"
#include "AnimationEditor.h"

#include "Shark/Asset/AssetManager.h"
#include "Shark/Animation/Animation.h"
#include "Shark/Render/MeshSource.h"

#include "Shark/UI/UICore.h"
#include "Shark/UI/Controls.h"
#include "Shark/UI/Widgets.h"

namespace Shark {

	AnimationEditor::AnimationEditor(const std::string& panelName, const AssetMetaData& metadata)
		: EditorPanel(panelName), m_Asset(metadata.Handle)
	{
	}

	void AnimationEditor::DrawWindow(bool& showWindow)
	{
		auto animation = AssetManager::GetAssetAsync<AnimationAsset>(m_Asset);

		if (animation)
		{
			UI::BeginControlsGrid();
			m_AssetDirty |= UI::ControlAsset("Source", AssetType::MeshSource, animation->m_AnimationSource);

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

	ImGuiWindowFlags AnimationEditor::GetWindowFlags() const
	{
		if (m_AssetDirty)
			return ImGuiWindowFlags_UnsavedDocument;
		return ImGuiWindowRefreshFlags_None;
	}

}
