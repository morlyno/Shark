#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Asset/Asset.h"

#include <miniaudio/miniaudio.h>

namespace Shark {
	class MiniAudioEngine;
	class SoundConfig;
}

namespace Shark::Audio {

	enum class PlayState
	{
		Stopped,
		Playing,
		Paused
	};

	class Sound
	{
	public:
		Sound(std::function<void(Sound*)> callback);
		~Sound();

		bool Initialize(AssetHandle audioAsset, MiniAudioEngine* audioEngine);
		void ApplySoundConfig(Ref<SoundConfig> soundConfig);
		void Uninitialize();

		bool IsReady() const { return m_Ready; }
		PlayState GetPlayState() const { return m_PlayState; }

		bool Play();
		bool Stop();
		bool Pause();
		
		void SetLooping(bool loop);
		void SetVolume(float multiplier);
		void SetPitch(float multiplier);

		bool IsLooping() const  { return m_Looping; }
		float GetVolume() const;
		float GetPitch() const;

		bool IsPlaying();
		bool Finished() const;

	private:
		template<std::invocable<Sound*> TFunc>
		void SetFinishedCallback(TFunc&& func)
		{
			m_OnFinished = std::forward<TFunc>(func);
		}

		bool StopSound(bool invokeCallback);

	private:
		ma_sound m_Sound;
		PlayState m_PlayState = PlayState::Stopped;

		bool m_Ready = false;
		bool m_Finished = false;

		std::function<void(Sound*)> m_OnFinished;

		bool m_Looping = false;
		float m_Volume = 1.0f;
		float m_Pitch = 1.0f;

		friend class MiniAudioEngine;
	};

}
