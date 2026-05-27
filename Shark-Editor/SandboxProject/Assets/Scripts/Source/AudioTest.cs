
using Shark;

namespace Sandbox
{
    public class AudioTest : Entity
    {
        AudioComponent m_AudioComponent;

        protected override void OnCreate()
        {
            m_AudioComponent = GetComponent<AudioComponent>()!;
        }

        protected override void OnUpdate(float ts)
        {
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

            if (Input.IsKeyPressed(KeyCode.P, allowRepeate: true))
            {
                m_AudioComponent.Play();
            }
        }
    }
}
