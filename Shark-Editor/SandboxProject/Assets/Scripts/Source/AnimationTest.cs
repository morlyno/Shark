using Shark;

namespace Sandbox
{
    public class AnimationTest : Entity
    {
        AnimationComponent m_Animation;
        AudioComponent m_Audio;

        ulong m_LastAnimtion = 0;
        bool m_Attacking = false;

        protected override void OnCreate()
        {
            m_Animation = GetComponent<AnimationComponent>()!;
            m_Audio = GetComponent<AudioComponent>()!;
        }

        protected override void OnUpdate(float ts)
        {
            if (m_Animation.Finished())
            {
                m_Animation.AnimationIndex = m_LastAnimtion;
                m_Animation.Loop = true;
                m_Attacking = false;
            }

            if (!m_Attacking && Input.IsKeyPressed(KeyCode.H))
            {
                m_Audio.Play();
                m_LastAnimtion = m_Animation.AnimationIndex;
                m_Animation.AnimationIndex = 0;
                m_Animation.PlaybackPosition = 0.0f;
                m_Animation.Loop = false;
                m_Attacking = true;
            }
        }
    }
}
