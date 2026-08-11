#include "skpch.h"
#include "SoundConfigEditor.h"

#include "Shark/Asset/AssetManager.h"
#include "Shark/Audio/SoundConfig.h"
#include "Shark/UI/UICore.h"
#include "Shark/UI/Controls.h"

namespace Shark {

	SoundConfigEditor::SoundConfigEditor(const std::string& panelName, const AssetMetaData& metadata)
		: EditorPanel(panelName), m_Asset(metadata.Handle)
	{
	}

	void SoundConfigEditor::OnImGuiRender(bool& showWindow)
	{
		if (m_DockWindow)
		{
			ImGui::SetNextWindowDockID(m_DockspaceID, ImGuiCond_Always);
			m_DockWindow = false;
		}

		ImGuiWindowFlags windowFlags = 0;
		if (m_ConfigDirty)
			windowFlags |= ImGuiWindowFlags_UnsavedDocument;

		if (ImGui::Begin(m_PanelName.c_str(), &showWindow, windowFlags))
		{
			auto soundConfig = AssetManager::GetAssetAsync<SoundConfig>(m_Asset);

			if (soundConfig)
			{
				UI::BeginControlsGrid();
				m_ConfigDirty |= UI::ControlAsset("Audio Source", AssetType::AudioFile, soundConfig->AudioSourceHandle);
				m_ConfigDirty |= UI::Control("Loop", soundConfig->IsLooping);
				ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal | ImGuiSeparatorFlags_SpanAllColumns);
				m_ConfigDirty |= UI::Control("Volume Multiplier", soundConfig->VolumeMultiplier);
				m_ConfigDirty |= UI::Control("Pitch Multiplier", soundConfig->PitchMultiplier);
				UI::EndControlsGrid();
			}

			ImGui::BeginHorizontal(UI::GenerateID(), ImGui::GetContentRegionAvail(), 1.0f);

			ImGui::Spring();
			if (ImGui::Button("Save"))
			{
				Project::GetEditorAssetManager()->SaveAsset(m_Asset);
				m_ConfigDirty = false;
				// #TODO popup when window closes to save asset
			}

			ImGui::EndHorizontal();

		}
		ImGui::End();
	}

	void SoundConfigEditor::DockWindow(ImGuiID dockspaceID)
	{
		m_DockspaceID = dockspaceID;
		m_DockWindow = true;
	}

}
