using Shark;
using System.Runtime.CompilerServices;

namespace Sandbox
{
    public class AnimationTest : Entity
    {
        enum State
        {
            Idle, Attacking
        };

        AnimationComponent m_Animation;
        AudioComponent m_Audio;

        public Animation Idle;
        public Animation Attack;
        public Animation Run;
        public bool Blend = true;

        State m_State = State.Idle;

        protected override void OnCreate()
        {
            m_Animation = GetComponent<AnimationComponent>()!;
            m_Audio = GetComponent<AudioComponent>()!;
        }

        protected override void OnUpdate(float ts)
        {
            if (Input.IsKeyPressed(KeyCode.W))
            {
                m_Animation.Transition(Run, 2.0f, true);
            }

            if (Input.IsKeyReleased(KeyCode.W))
            {
                m_Animation.Transition(Idle, 2.0f, true);
            }

            switch (m_State)
            {
                case State.Idle:      UpdateIdle(ts);      break;
                case State.Attacking: UpdateAttacking(ts); break;
            }
        }

        void UpdateIdle(float ts)
        {
            if (Input.IsKeyPressed(KeyCode.H))
            {
                m_Audio.Play();
                m_Animation.Transition(Attack, 0.2f, false);
                m_State = State.Attacking;
            }
        }

        void UpdateAttacking(float ts)
        {
            if (m_Animation.Finished())
            {
                m_Animation.Transition(Idle, 0.5f, true);
                m_State = State.Idle;
            }
        }

    }
}
