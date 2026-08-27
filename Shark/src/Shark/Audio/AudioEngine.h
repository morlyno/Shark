#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Asset/Asset.h"

#include <miniaudio/miniaudio.h>

#include <queue>
#include <span>

namespace Shark {
	class Scene;
	class AudioFile;
	class SoundConfig;
	struct AudioComponent;

	namespace Audio {
		class Sound;
	}
}

#define SK_INVALID_SOUND_ID static_cast<uint64_t>(~0);

namespace Shark {

	using SoundID = uint64_t;

	struct SoundObject
	{
		Audio::Sound*    Sound    = nullptr;
		UUID             EntityID = UUID::Invalid;
		AssetHandle      Audio    = AssetHandle::Invalid;
		Ref<SoundConfig> Config   = nullptr;

		void Uninitialize();
	};

	class MiniAudioEngine
	{
	public:
		MiniAudioEngine();
		~MiniAudioEngine();

		void OnScenePlay(Ref<Scene> scene);
		void OnSceneStop(Ref<Scene> scene);

		bool HasActiveSound(UUID entityID);
		bool StartPlayback(UUID audioEntityID);
		bool StopPlayback(UUID audioEntityID);
		bool PausePlayback(UUID audioEntityID);
		bool ResumePlayback(UUID audioEntityID);

		SoundID StartSoundPlayback(Ref<SoundConfig> soundConfig, UUID attachedEntityID, bool useComponent = true);
		void    StopSoundPlayback(SoundID soundID);
		void    PauseSoundPlayback(SoundID soundID);
		void    ResumeSoundPlayback(SoundID soundID);
		bool    IsSoundPlaying(SoundID soundID);
		bool    IsSoundFinished(SoundID soundID);

		void StopAll();
		auto GetAllSound() const { return std::span(m_Sounds); }
		Audio::Sound* GetSound(UUID entityID) const;

		ma_engine* GetEngine() { return &m_Engine; }
		const Ref<Scene>& GetActiveScene() const;
		Ref<AudioFile> QueryFileInfo(AssetHandle handle);

		bool IsStreaming(AssetHandle audioFile);

	private:
		void OnSoundFinished(Audio::Sound* sound);
		void FreeLowestPrioritySound();

		SoundID StartPlayback(AssetHandle audioSource, Ref<SoundConfig> soundConfig, UUID attachedEntityID, AudioComponent* component);

	private:
		ma_engine m_Engine;
		ma_log m_Log;
		ma_vfs_callbacks m_VFS;

		uint32_t m_MaximumSounds = 0;
		uint32_t m_SoundsPlaying = 0;
		std::vector<SoundObject> m_Sounds;
		std::vector<SoundID> m_ActiveSounds;
		std::queue<SoundID> m_AvailableSounds;

		Ref<Scene> m_ActiveScene;
	};

}
