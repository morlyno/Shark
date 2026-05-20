#include "skpch.h"
#include "SoundPanel.h"

#include "Shark/Core/Application.h"
#include "Shark/Audio/AudioEngine.h"
#include "Shark/Audio/Sound.h"

#include "Shark/UI/UICore.h"
#include "Shark/UI/Controls.h"

#include "Shark/File/FileSystem.h"


namespace Shark {

	void SoundPanel::OnImGuiRender(bool& isOpen)
	{
		if (!isOpen)
			return;

		if (ImGui::Begin(m_PanelName, &isOpen))
		{
			const auto audioEngine = Application::Get().GetAudioEngine();
			const auto scene = audioEngine->GetActiveScene();
			const auto sounds = audioEngine->GetAllSound();

			uint32_t index = 0;
			for (const auto& soundObject : sounds)
			{
				UI::BeginControlsGrid();

				UI::ControlAsset("Asset", AssetType::AudioFile, soundObject.Audio);
				UI::ControlEntity("Entity", scene, soundObject.EntityID);
				UI::Control("State", soundObject.Sound->GetPlayState());
				UI::Control("Finished", soundObject.Sound->Finished());

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
