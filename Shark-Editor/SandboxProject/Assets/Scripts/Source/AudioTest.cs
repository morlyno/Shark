
using Shark;
using System.Collections.Generic;

namespace Sandbox
{
    public class AudioTest : Entity
    {
        AudioComponent m_AudioComponent;
        public float Volume = 1.0f;
        public float Pitch = 1.0f;

        public bool SetVolume = false;
        public bool SetPitch = false;

        public SoundConfig Sound;

        bool m_UseComponent = false;
        List<SoundID> m_ActiveSounds = new List<SoundID>();

        static readonly KeyCode[] s_SoundKeys = [KeyCode.D1, KeyCode.D2, KeyCode.D3];

        protected override void OnCreate()
        {
            if (!HasComponent<AudioComponent>())
                m_AudioComponent = CreateComponent<AudioComponent>()!;
            else
                m_AudioComponent = GetComponent<AudioComponent>()!;

            // #TODO not serialized attribute
            SetVolume = false;
            SetPitch = false;
        }

        protected override void OnUpdate(float ts)
        {
            m_ActiveSounds.RemoveAll(id => Audio.Finished(id));

            if (Input.IsKeyPressed(KeyCode.Space))
            {
                if (m_AudioComponent.IsPlaying())
                    m_AudioComponent.Pause();
                else
                    m_AudioComponent.Resume();
            }

            if (Input.IsKeyReleased(KeyCode.S))
            {
                m_AudioComponent.Stop();
            }

            if (Input.IsKeyPressed(KeyCode.P))
            {
                m_AudioComponent.Play();
            }

            if (Input.IsKeyPressed(KeyCode.N))
            {
                m_UseComponent = !m_UseComponent;
                if (m_UseComponent)
                    m_ActiveSounds.Add(Audio.StartPlayback(Sound, m_AudioComponent));
                else
                    m_ActiveSounds.Add(Audio.StartPlayback(Sound, ID));

            }

            for (int i = 0; i < m_ActiveSounds.Count && i < s_SoundKeys.Length; i++)
            {
                if (Input.IsKeyPressed(s_SoundKeys[i]))
                {
                    Audio.PausePlayback(m_ActiveSounds[i]);
                }
                else if (Input.IsKeyReleased(s_SoundKeys[i]))
                {
                    Audio.ResuePlayback(m_ActiveSounds[i]);
                }
            }

            if (SetVolume)
            {
                m_AudioComponent.VolumeMultiplier = Volume;
                SetVolume = false;
            }

            if (SetPitch)
            {
                m_AudioComponent.PitchMultiplier = Pitch;
                SetPitch = false;
            }

        }
    }
}
