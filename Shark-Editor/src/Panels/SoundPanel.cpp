#include "skpch.h"
#include "SoundPanel.h"

#include "Shark/Core/Project.h"
#include "Shark/Core/Application.h"

#include "Shark/Asset/AssetManager.h"
#include "Shark/Asset/AssetManager/EditorAssetManager.h"

#include "Shark/Audio/AudioEngine.h"
#include "Shark/Audio/Sound.h"

#include "Shark/UI/UICore.h"
#include "Shark/UI/Controls.h"

namespace Shark {

	namespace utils {

		std::string GetAssetName(AssetHandle handle)
		{
			if (!handle)
				return "";

			if (!AssetManager::IsValidAssetHandle(handle))
			{
				return "Invalid";
			}

			auto assetManager = Project::GetEditorAssetManager();

			const bool isMemoryAsset = assetManager->IsMemoryAsset(handle);
			const bool valid = isMemoryAsset || assetManager->HasExistingFilePath(handle);

			if (isMemoryAsset)
				return fmt::format("{}", handle);

			const auto& metadata = assetManager->GetMetadata(handle);
			return metadata.FilePath.string();
		}

	}

	void SoundPanel::OnImGuiRender(bool& isOpen)
	{
		if (!isOpen)
			return;

		if (ImGui::Begin(m_PanelName, &isOpen))
		{
			const auto audioEngine = Application::Get().GetAudioEngine();
			const auto& scene = audioEngine->GetActiveScene();
			const auto sounds = audioEngine->GetAllSound();

			UI::CheckboxButton("Hide Inactive", &m_ShowActive);
			ImGui::SameLine();
			UI::CheckboxButton("Simple", &m_Simple);

			uint32_t i = 0;
			for (const auto& soundObject : sounds)
			{
				auto index = i++;
				if (m_ShowActive && !soundObject.Sound->IsReady())
					continue;

				if (m_Simple)
				{
					std::string infoString = fmt::format("{} | {} | {}", index, utils::GetAssetName(soundObject.Audio), soundObject.Sound->GetPlayState());
					ImGui::Text(infoString);
					continue;
				}

				UI::BeginControlsGrid();
				UI::ControlAsset("Asset", AssetType::AudioFile, soundObject.Audio);
				UI::ControlEntity("Entity", scene, soundObject.EntityID);
				UI::Control("State", soundObject.Sound->GetPlayState());

				if (soundObject.Sound->IsReady())
				{
					UI::Control("Play/Pause", [sound = soundObject.Sound]()
					{
						ImGui::BeginHorizontal("sound_controls");

						if (ImGui::Button("Play"))
						{
							sound->Play();
						}

						if (ImGui::Button("Pause"))
						{
							sound->Pause();
						}

						if (ImGui::Button("Stop"))
						{
							sound->Stop();
						}

						ImGui::EndHorizontal();
						return false;
					});

				}

				UI::EndControlsGrid();
			}

		}
		ImGui::End();
	}

}
