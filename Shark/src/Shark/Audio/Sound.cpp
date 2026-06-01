#include "skpch.h"
#include "Sound.h"

#include "Shark/Core/Application.h"
#include "Shark/Audio/AudioEngine.h"
#include "Shark/Audio/SoundConfig.h"

namespace Shark::Audio {

	Sound::Sound(std::function<void(Sound*)> callback)
		: m_OnFinished(std::move(callback))
	{
	}

	Sound::~Sound()
	{
	}

	bool Sound::Initialize(AssetHandle audioAsset, MiniAudioEngine* audioEngine)
	{
		ScopedTimer timer("Sound.Initialize");

		m_Finished = false;
		m_PlayState = PlayState::Stopped;
		auto sourceFileID = fmt::to_string(audioAsset);

		uint32_t flags = MA_SOUND_FLAG_DECODE;
		if (audioEngine->IsStreaming(audioAsset))
			flags |= MA_SOUND_FLAG_STREAM;

		auto result = ma_sound_init_from_file(audioEngine->GetEngine(), sourceFileID.c_str(), flags, nullptr, nullptr, &m_Sound);
		if (result != MA_SUCCESS)
		{
			SK_CORE_ASSERT(result == MA_SUCCESS, "Failed to initialize sound '{}' with error {}", audioAsset, result);
			m_Ready = false;
			return false;
		}

		ma_sound_set_end_callback(&m_Sound, [](void* _This, ma_sound* sound)
		{
			// #TODO #audio this should be handled by an audio thread
			Application::Get().SubmitToMainThread([This = static_cast<Sound*>(_This)]() { This->StopSound(true); });
		}, this);

		

		m_Ready = true;
		return true;
	}

	void Sound::ApplySoundConfig(Ref<SoundConfig> soundConfig)
	{
		if (!soundConfig)
			return;

		m_Volume = soundConfig->VolumeMultiplier;
		m_Pitch = soundConfig->PitchMultiplier;
		ma_sound_set_volume(&m_Sound, soundConfig->VolumeMultiplier);
		ma_sound_set_pitch(&m_Sound, soundConfig->PitchMultiplier);

		SetLooping(soundConfig->IsLooping);
	}

	void Sound::Uninitialize()
	{
		if (m_Ready)
		{
			ma_sound_uninit(&m_Sound);
			m_Ready = false;
		}
	}

	bool Sound::Play()
	{
		if (!m_Ready)
			return false;

		ma_sound_start(&m_Sound);
		m_Finished = false;
		m_PlayState = PlayState::Playing;
		return true;
	}

	bool Sound::Stop()
	{
		if (!m_Ready)
			return false;

		StopSound(true);
		return true;
	}

	bool Sound::Pause()
	{
		if (!m_Ready)
			return false;

		// #audio should this be possible when sound is stopped

		ma_sound_stop(&m_Sound);
		m_PlayState = PlayState::Paused;
		return true;
	}

	bool Sound::IsPlaying()
	{
		return m_PlayState == PlayState::Playing;
	}

	bool Sound::Finished() const
	{
		return m_Finished;
	}

	bool Sound::StopSound(bool invokeCallback)
	{
		if (!m_Ready)
			return false;

		if (m_Finished)
			return true;

		ma_sound_stop(&m_Sound);
		m_Finished = true;
		m_PlayState = PlayState::Stopped;

		if (invokeCallback && m_OnFinished)
			m_OnFinished(this);

		return true;
	}

	void Sound::SetLooping(bool loop)
	{
		m_Looping = loop;
		ma_sound_set_looping(&m_Sound, loop);
	}

	void Sound::SetVolume(float multiplier)
	{
		// apply only the multiplier
		ma_sound_set_volume(&m_Sound, m_Volume * multiplier);
	}

	void Sound::SetPitch(float multiplier)
	{
		ma_sound_set_pitch(&m_Sound, m_Pitch * multiplier);
	}

	float Sound::GetVolume() const
	{
		return ma_sound_get_volume(&m_Sound);
	}

	float Sound::GetPitch() const
	{
		return ma_sound_get_pitch(&m_Sound);
	}

}
