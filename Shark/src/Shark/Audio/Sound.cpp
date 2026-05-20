#include "skpch.h"
#include "Sound.h"

#include "Shark/Core/Application.h"
#include "Shark/Audio/AudioEngine.h"

namespace Shark::Audio {

	Sound::Sound(std::function<void(Sound*)> callback)
		: m_OnFinished(std::move(callback))
	{
	}

	Sound::~Sound()
	{
	}

	void Sound::Initialize(AssetHandle audioAsset, MiniAudioEngine* audioEngine)
	{
		ScopedTimer timer("Sound.Initialize");

		m_Finished = false;
		m_PlayState = PlayState::Stopped;

		char sourceFile[std::numeric_limits<uint64_t>::digits10 + 2];
		*fmt::format_to(sourceFile, "{}", audioAsset).out = '\0';

		const uint32_t flags = MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_STREAM;
		auto result = ma_sound_init_from_file(audioEngine->GetEngine(), sourceFile, flags, nullptr, nullptr, &m_Sound);
		SK_CORE_ASSERT(result == MA_SUCCESS, "Failed to initialize sound '{}'", audioAsset);

		ma_sound_set_end_callback(&m_Sound, [](void* _This, ma_sound* sound)
		{
			// #TODO #audio this should be handled by an audio thread
			Application::Get().SubmitToMainThread([This = static_cast<Sound*>(_This)]()
			{
				SK_CORE_TRACE_TAG("Audio", "Sound at end");
				This->StopSound(true);
			});

			//static_cast<Sound*>(_This)->StopSound(true);
		}, this);

		m_Ready = result == MA_SUCCESS;	
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
	}

	bool Sound::Pause()
	{
		if (!m_Ready)
			return false;

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

}
