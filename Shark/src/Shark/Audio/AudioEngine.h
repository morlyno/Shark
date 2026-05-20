#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Asset/Asset.h"

#include <miniaudio/miniaudio.h>

#include <queue>
#include <span>

namespace Shark {
	class Scene;

	namespace Audio {
		class Sound;
	}
}

namespace Shark {

	struct SoundObject
	{
		Audio::Sound* Sound = nullptr;
		UUID EntityID       = UUID::Invalid;
		AssetHandle Audio   = AssetHandle::Invalid;

		void Uninitialize();
	};

	class MiniAudioEngine
	{
	public:
		MiniAudioEngine();
		~MiniAudioEngine();

		void OnScenePlay(Ref<Scene> scene);
		void OnSceneStop(Ref<Scene> scene);
		void StartPlayback(UUID audioEntityID);

		void StopAll();

		ma_engine* GetEngine() { return &m_Engine; }
		Ref<Scene> GetActiveScene() const;
		auto GetAllSound() const { return std::span(m_Sounds); }

	private:
		void OnSoundFinished(Audio::Sound* sound);
		void FreeLowestPrioritySound();

		void QueryFileInfo(AssetHandle handle);

	private:
		ma_engine m_Engine;
		ma_log m_Log;
		ma_vfs_callbacks m_VFS;

		uint32_t m_MaximumSounds = 0;
		uint32_t m_SoundsPlaying = 0;
		std::vector<SoundObject> m_Sounds;
		std::queue<size_t> m_AvailableSounds;

		Ref<Scene> m_ActiveScene;
	};

}
