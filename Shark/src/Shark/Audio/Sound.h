#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Asset/Asset.h"

#include <miniaudio/miniaudio.h>

namespace Shark {
	class MiniAudioEngine;
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

		void Initialize(AssetHandle audioAsset, MiniAudioEngine* audioEngine);
		void Uninitialize();

		bool IsReady() const { return m_Ready; }
		PlayState GetPlayState() const { return m_PlayState; }

		bool Play();
		bool Stop();
		bool Pause();
		void SetLooping(bool loop);

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
		// #TODO #audio volume, pitch, fade in/out

		friend class MiniAudioEngine;
	};

}
