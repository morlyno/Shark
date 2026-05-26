#include "skpch.h"

#define MINIAUDIO_IMPLEMENTATION
#include "AudioEngine.h"
#include "Shark/Asset/AssetManager.h"

#include "Shark/Audio/Sound.h"
#include "Shark/Audio/VFS.h"
#include "Shark/Audio/AudioFile.h"

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

	void MiniAudioEngine::StartPlayback(UUID audioEntityID)
	{
		Entity entity = m_ActiveScene->GetEntityByID(audioEntityID);
		auto& audioComponent = entity.GetComponent<AudioComponent>();
		if (!audioComponent.Audio)
			return;

		if (m_AvailableSounds.empty())
		{
			FreeLowestPrioritySound();
		}

		auto soundIndex = m_AvailableSounds.front();
		m_AvailableSounds.pop();

		//QueryFileInfo(audioComponent.Audio);
		auto& object = m_Sounds.at(soundIndex);
		object.EntityID = audioEntityID;
		object.Audio = audioComponent.Audio;
		// #TODO #audio initialize sound on audio thread
		object.Sound->Initialize(audioComponent.Audio, this);
		object.Sound->SetLooping(audioComponent.Loop);
		object.Sound->Play();
		m_SoundsPlaying += 1;

		SK_CORE_INFO_TAG("Audio", "Started Playback of {} for {}", audioComponent.Audio, entity.GetName());
	}

	void MiniAudioEngine::StopAll()
	{
		for (auto& sound : m_Sounds)
		{
			sound.Sound->StopSound(false);
			sound.Uninitialize();
		}

		m_SoundsPlaying = 0;
	}

	Ref<Scene> MiniAudioEngine::GetActiveScene() const
	{
		return m_ActiveScene;
	}

	void MiniAudioEngine::OnSoundFinished(Audio::Sound* sound)
	{
		const auto soundObject = std::ranges::find(m_Sounds, sound, &SoundObject::Sound);
		if (soundObject == m_Sounds.end())
			return;

		const auto index = std::distance(m_Sounds.begin(), soundObject);

		SK_CORE_INFO_TAG("Audio", "Sound '{}' finished for {}", soundObject->Audio, utils::TryGetEntityName(m_ActiveScene.Raw(), soundObject->EntityID));
		soundObject->Uninitialize();
		m_AvailableSounds.push(index);
		m_SoundsPlaying -= 1;

		SK_CORE_TRACE_TAG("Audio", "Sound available at index {}", index);
	}

	void MiniAudioEngine::FreeLowestPrioritySound()
	{
		// #TODO #audio priority system

		if (!m_AvailableSounds.empty())
			return;

		size_t index = 0;
		auto& object = m_Sounds[index];

		if (object.Sound->IsPlaying())
			object.Sound->StopSound(false);

		object.Uninitialize();
		m_AvailableSounds.push(index);
		m_SoundsPlaying -= 1;

		SK_CORE_TRACE_TAG("Audio", "Freed sound at index {}", index);
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
