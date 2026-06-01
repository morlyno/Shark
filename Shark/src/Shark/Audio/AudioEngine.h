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

	namespace Audio {
		class Sound;
	}
}

namespace Shark {

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

		void StopAll();
		auto GetAllSound() const { return std::span(m_Sounds); }

		ma_engine* GetEngine() { return &m_Engine; }
		Ref<Scene> GetActiveScene() const;
		Ref<AudioFile> QueryFileInfo(AssetHandle handle);

		bool IsStreaming(AssetHandle audioFile);

	private:
		void OnSoundFinished(Audio::Sound* sound);
		void FreeLowestPrioritySound();

	private:
		ma_engine m_Engine;
		ma_log m_Log;
		ma_vfs_callbacks m_VFS;

		uint32_t m_MaximumSounds = 0;
		uint32_t m_SoundsPlaying = 0;
		std::vector<SoundObject> m_Sounds;
		std::vector<size_t> m_ActiveSounds;
		std::queue<size_t> m_AvailableSounds;

		Ref<Scene> m_ActiveScene;
	};

}
