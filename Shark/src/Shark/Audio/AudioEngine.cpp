#include "skpch.h"

#define MINIAUDIO_IMPLEMENTATION
#include "AudioEngine.h"
#include "Shark/Asset/AssetManager.h"

#include "Shark/Audio/Sound.h"
#include "Shark/Audio/VFS.h"
#include "Shark/Audio/AudioFile.h"
#include "Shark/Audio/SoundConfig.h"

#include "Shark/Scene/Scene.h"
#include "Shark/Scene/Entity.h"
#include "Shark/Scene/Components.h"

#include "Shark/Utils/String.h"

namespace Shark {

	namespace utils {

		static std::string_view TryGetEntityName(Scene* scene, UUID entityID)
		{
			if (!scene)
				return {};

			auto entity = scene->TryGetEntityByUUID(entityID);
			return entity ? entity.GetName() : std::string_view{};
		}

		static std::pair<Ref<SoundConfig>, AssetHandle> GetConfigAndSource(AssetHandle audioAsset)
		{
			auto assetType = AssetManager::GetAssetType(audioAsset);
			switch (assetType)
			{
				case AssetType::SoundConfig:
				{
					auto soundConfig = AssetManager::GetAsset<SoundConfig>(audioAsset);
					return {
						soundConfig,
						soundConfig->AudioSourceHandle
					};
				}
				case AssetType::AudioFile:
				{
					return {
						nullptr,
						audioAsset
					};
				}
			}

			SK_CORE_ASSERT(false, "Invalid audio asset type '{}'", assetType);
			return {
				nullptr,
				AssetHandle::Invalid
			};
		}

	}

	static void MALogCallback(void* pUserData, ma_uint32 level, const char* pMessage)
	{
		std::string_view message = pMessage;
		String::StripBack(message, "\n");

		switch (level)
		{
			case MA_LOG_LEVEL_DEBUG:   SK_CORE_DEBUG_TAG("Audio", message); break;
			case MA_LOG_LEVEL_INFO:    SK_CORE_WARN_TAG("Audio", message);  break;
			case MA_LOG_LEVEL_WARNING: SK_CORE_INFO_TAG("Audio", message);  break;
			case MA_LOG_LEVEL_ERROR:   SK_CORE_ERROR_TAG("Audio", message); break;
		}
	}

	MiniAudioEngine::MiniAudioEngine()
	{
		ma_engine_config config = ma_engine_config_init();
		ma_allocation_callbacks* allocationCallbacks = nullptr;


#if SK_TRACK_MEMORY
		config.allocationCallbacks = {
			nullptr,
			[](size_t sz, void*)          { return Allocator::ModuleAllocate("miniaudio", sz); },
			[](void* p, size_t sz, void*) { return Allocator::ModuleReallocate("miniaudio", p, sz); },
			[](void* p, void*)            {        Allocator::ModuleFree("miniaudio", p);}
		};

		allocationCallbacks = &config.allocationCallbacks;
#endif

		ma_log_init(allocationCallbacks, &m_Log);
		ma_log_register_callback(&m_Log, ma_log_callback_init(&MALogCallback, nullptr));

		config.pLog = &m_Log;
		config.channels = 2;

		Audio::InitailizeVFS(&m_VFS);
		config.pResourceManagerVFS = &m_VFS;

		auto result = ma_engine_init(&config, &m_Engine);
		SK_CORE_VERIFY(result == MA_SUCCESS);

		// Limit active sound to 32 for now
		// this seems to be the most common amount
		// #audio user settings / backend query?
		m_MaximumSounds = 32;
		m_Sounds.resize(m_MaximumSounds);
		for (size_t i = 0; i < m_MaximumSounds; i++)
		{
			m_Sounds[i].Sound = sknew Audio::Sound(std::bind_front(&MiniAudioEngine::OnSoundFinished, this));
			m_AvailableSounds.push(i);
		}

		SK_CORE_INFO_TAG("Audio",
						 "Audio engine initialized\n"
						 "\t----------------------------\n"
						 "\tDevice name:    {}\n"
						 "\tSample Rate:    {}\n"
						 "\tChannels:       {}\n"
						 "\t----------------------------\n"
						 "\tMaximum Sounds: {}\n"
						 "\tListeners:      {}\n"
						 "\t----------------------------",
						 m_Engine.pDevice->playback.name,
						 ma_engine_get_sample_rate(&m_Engine),
						 ma_engine_get_channels(&m_Engine),
						 m_MaximumSounds,
						 ma_engine_get_listener_count(&m_Engine));
	}

	MiniAudioEngine::~MiniAudioEngine()
	{
		StopAll();

		for (auto& object : m_Sounds)
			delete object.Sound;

		ma_engine_uninit(&m_Engine);
	}

	void MiniAudioEngine::OnScenePlay(Ref<Scene> scene)
	{
		m_ActiveScene = scene;

		auto entities = m_ActiveScene->GetAllEntitysWith<AudioComponent>();
		for (auto ent : entities)
		{
			Entity entity{ ent, m_ActiveScene };
			auto& audioComponent = entity.GetComponent<AudioComponent>();
			if (!audioComponent.PlayOnWake)
				continue;

			StartPlayback(entity.GetUUID());
		}
	}

	void MiniAudioEngine::OnSceneStop(Ref<Scene> scene)
	{
		StopAll();
		m_ActiveScene = nullptr;
	}

	bool MiniAudioEngine::HasActiveSound(UUID entityID)
	{
		const auto i = std::ranges::find(m_Sounds, entityID, &SoundObject::EntityID);
		if (i == m_Sounds.end())
			return false;

		return i->Sound->IsPlaying();
	}

	bool MiniAudioEngine::StartPlayback(UUID audioEntityID)
	{
		// #audio #Investigate StartPlayback called multiple times
		// 
		// starting multiple sound is possible like this
		// but the can't be controlled independently
		//

		Entity entity = m_ActiveScene->GetEntityByID(audioEntityID);
		auto& audioComponent = entity.GetComponent<AudioComponent>();
		if (!audioComponent.Audio)
			return false;

		AssetHandle audioSource;
		Ref<SoundConfig> soundConfig;

		{
			auto [sc, as] = utils::GetConfigAndSource(audioComponent.Audio);
			if (as == AssetHandle::Invalid)
				return false;

			soundConfig = sc;
			audioSource = as;
		}

		return StartPlayback(audioSource, soundConfig, audioEntityID, &audioComponent) != SK_INVALID_SOUND_ID;
	}

	bool MiniAudioEngine::StopPlayback(UUID audioEntityID)
	{
		auto range = std::ranges::remove_if(m_ActiveSounds, [this, audioEntityID](SoundID soundID)
		{
			return m_Sounds[soundID].EntityID == audioEntityID;
		});

		for (auto id : std::views::reverse(range))
		{
			// NOTE: this removes the sound from active sounds and uninitializes it in the callback
			//       thats why the range is reversed
			SK_CORE_TRACE_TAG("Audio", "Stop Playback of {} for {}", m_Sounds[id].Audio, utils::TryGetEntityName(m_ActiveScene.Raw(), audioEntityID));
			m_Sounds[id].Sound->Stop();
		}

		return true;
	}

	bool MiniAudioEngine::PausePlayback(UUID audioEntityID)
	{
		for (auto soundID : m_ActiveSounds)
		{
			auto& sound = m_Sounds[soundID];
			if (sound.EntityID != audioEntityID)
				continue;

			SK_CORE_TRACE_TAG("Audio", "Pause Playback of {} for {}", m_Sounds[soundID].Audio, utils::TryGetEntityName(m_ActiveScene.Raw(), audioEntityID));
			sound.Sound->Pause();
		}

		return true;
	}

	bool MiniAudioEngine::ResumePlayback(UUID audioEntityID)
	{
		for (auto soundID : m_ActiveSounds)
		{
			auto& sound = m_Sounds[soundID];
			if (sound.EntityID != audioEntityID)
				continue;

			if (sound.Sound->GetPlayState() == Audio::PlayState::Paused)
			{
				SK_CORE_TRACE_TAG("Audio", "Resume Playback of {} for {}", m_Sounds[soundID].Audio, utils::TryGetEntityName(m_ActiveScene.Raw(), audioEntityID));
				sound.Sound->Play();
			}
		}

		return true;
	}

	SoundID MiniAudioEngine::StartSoundPlayback(Ref<SoundConfig> soundConfig, UUID attachedEntityID, bool useComponent)
	{
		if (!soundConfig->AudioSourceHandle)
			return SK_INVALID_SOUND_ID;

		AudioComponent* component = nullptr;

		if (useComponent)
		{
			Entity entity = m_ActiveScene->TryGetEntityByUUID(attachedEntityID);
			if (entity && entity.HasComponent<AudioComponent>())
			{
				component = &entity.GetComponent<AudioComponent>();
				// #audio should component audio source and sound config audio source match?
			}
		}

		return StartPlayback(soundConfig->AudioSourceHandle, soundConfig, attachedEntityID, component);
	}

	void MiniAudioEngine::StopSoundPlayback(SoundID soundID)
	{
		if (soundID >= m_MaximumSounds)
			return;

		auto& soundObject = m_Sounds[soundID];
		soundObject.Sound->Stop();
	}

	void MiniAudioEngine::PauseSoundPlayback(SoundID soundID)
	{
		if (soundID >= m_MaximumSounds)
			return;

		auto& soundObject = m_Sounds[soundID];
		soundObject.Sound->Pause();
	}

	void MiniAudioEngine::ResumeSoundPlayback(SoundID soundID)
	{
		if (soundID >= m_MaximumSounds)
			return;

		auto& soundObject = m_Sounds[soundID];
		if (soundObject.Sound->GetPlayState() == Audio::PlayState::Paused)
		{
			// Only resume when sound was paused
			soundObject.Sound->Play();
		}
	}

	bool MiniAudioEngine::IsSoundPlaying(SoundID soundID)
	{
		if (soundID >= m_MaximumSounds)
			return false;

		auto& soundObject = m_Sounds[soundID];
		return soundObject.Sound->IsPlaying();
	}

	bool MiniAudioEngine::IsSoundFinished(SoundID soundID)
	{
		if (soundID >= m_MaximumSounds)
			return false;

		auto& soundObject = m_Sounds[soundID];
		return soundObject.Sound->Finished();
	}

	void MiniAudioEngine::StopAll()
	{
		SK_CORE_TRACE_TAG("Audio", "Stop all sounds");
		for (auto soundId : m_ActiveSounds)
		{
			m_Sounds[soundId].Sound->StopSound(false);
			m_Sounds[soundId].Uninitialize();
			m_AvailableSounds.push(soundId);
		}

		m_ActiveSounds.clear();
		m_SoundsPlaying = 0;
	}

	Audio::Sound* MiniAudioEngine::GetSound(UUID entityID) const
	{
		const auto soundObject = std::ranges::find(m_Sounds, entityID, &SoundObject::EntityID);
		if (soundObject == m_Sounds.end())
			return nullptr;

		return soundObject->Sound;
	}

	const Ref<Scene>& MiniAudioEngine::GetActiveScene() const
	{
		return m_ActiveScene;
	}

	void MiniAudioEngine::OnSoundFinished(Audio::Sound* sound)
	{
		const auto soundObject = std::ranges::find(m_Sounds, sound, &SoundObject::Sound);
		if (soundObject == m_Sounds.end())
			return;

		const auto index = std::distance(m_Sounds.begin(), soundObject);

		// #audio #Investigate system to reuse a sound if needed

		SK_CORE_INFO_TAG("Audio", "Sound '{}' finished for {}", soundObject->Audio, utils::TryGetEntityName(m_ActiveScene.Raw(), soundObject->EntityID));
		soundObject->Uninitialize();
		m_AvailableSounds.push(index);
		m_SoundsPlaying -= 1;

		auto count = std::erase(m_ActiveSounds, index);
		SK_CORE_ASSERT(count == 1);

		SK_CORE_TRACE_TAG("Audio", "Sound available at index {}", index);
	}

	void MiniAudioEngine::FreeLowestPrioritySound()
	{
		// #TODO #audio priority system

		if (!m_AvailableSounds.empty())
			return;

		SoundID id = m_ActiveSounds.front();
		auto& object = m_Sounds[id];

		if (object.Sound->IsPlaying())
			object.Sound->StopSound(false);

		object.Uninitialize();
		std::erase(m_ActiveSounds, id);
		
		m_AvailableSounds.push(id);
		m_SoundsPlaying -= 1;

		SK_CORE_WARN_TAG("Audio", "Freed sound at index {}", id);
	}

	SoundID MiniAudioEngine::StartPlayback(AssetHandle audioSource, Ref<SoundConfig> soundConfig, UUID attachedEntityID, AudioComponent* component)
	{
		if (!audioSource)
			return SK_INVALID_SOUND_ID;

		if (m_AvailableSounds.empty())
		{
			FreeLowestPrioritySound();
		}

		auto soundID = m_AvailableSounds.front();
		m_AvailableSounds.pop();

		auto& object = m_Sounds.at(soundID);
		object.EntityID = attachedEntityID;
		object.Audio = audioSource;
		object.Config = soundConfig;

		// #TODO #audio initialize sound on audio thread
		object.Sound->Initialize(audioSource, this);
		object.Sound->ApplySoundConfig(soundConfig);

		if (component)
		{
			object.Sound->SetVolume(component->VolumeMultiplier);
			object.Sound->SetPitch(component->PitchMultiplier);
		}

		object.Sound->Play();
		m_ActiveSounds.push_back(soundID);

		SK_CORE_INFO_TAG("Audio", "Started Playback of {} for {}", audioSource, utils::TryGetEntityName(m_ActiveScene.Raw(), attachedEntityID));
		return soundID;
	}

	Ref<AudioFile> MiniAudioEngine::QueryFileInfo(AssetHandle handle)
	{
		ScopedTimer timer("QueryFileInfo");

		auto filepath = fmt::to_string(handle);

		ma_result result;
		ma_decoder decoder;
		ma_decoder_config config = ma_decoder_config_init_default();
		result = ma_decoder_init_vfs(&m_VFS, filepath.c_str(), &config, &decoder);
		if (result != MA_SUCCESS)
		{
			SK_CORE_VERIFY(result == MA_SUCCESS, "Result: {}", result);
			return nullptr;
		}

		ma_format format;
		ma_uint32 channels;
		ma_uint32 sampleRate;
		result = ma_decoder_get_data_format(&decoder, &format, &channels, &sampleRate, nullptr, 0);
		if (result != MA_SUCCESS)
		{
			ma_decoder_uninit(&decoder);
			SK_CORE_VERIFY(result == MA_SUCCESS, "Result: {}", result);
			return nullptr;
		}

		ma_uint64 pcmLength;
		result = ma_decoder_get_length_in_pcm_frames(&decoder, &pcmLength);
		SK_CORE_VERIFY(result == MA_SUCCESS, "Result: {}", result);

		ma_file_info info;
		result = ma_vfs_info(decoder.data.vfs.pVFS, decoder.data.vfs.file, &info);
		if (result != MA_SUCCESS)
		{
			ma_decoder_uninit(&decoder);
			return nullptr;
		}

		SK_CORE_TRACE_TAG("Audio",
						  "File info '{}'\n"
						  "\t Format:      {}\n"
						  "\t Channels:    {}\n"
						  "\t Sample rate: {}\n"
						  "\t PCM frames:  {}\n"
						  "\t Duration:    {}s",
						  handle,
						  format,
						  channels,
						  sampleRate,
						  pcmLength,
						  static_cast<double>(pcmLength) / static_cast<double>(sampleRate));

		ma_decoder_uninit(&decoder);

		auto audioFile        = Ref<AudioFile>::Create();
		audioFile->Lenght     = pcmLength;
		audioFile->SampleRate = sampleRate;
		audioFile->Channels   = static_cast<uint16_t>(channels);
		audioFile->BitDepth   = static_cast<uint16_t>(ma_get_bytes_per_sample(format));
		audioFile->FileSize   = info.sizeInBytes;
		return audioFile;
	}

	bool MiniAudioEngine::IsStreaming(AssetHandle audio)
	{
		Ref<AudioFile> audioFile = AssetManager::GetAsset<AudioFile>(audio);

		return audioFile->LenghtInSenconds() > 30.0;
	}

	void SoundObject::Uninitialize()
	{
		Sound->Uninitialize();
		EntityID = UUID::Invalid;
		Audio    = AssetHandle::Invalid;
	}

}
