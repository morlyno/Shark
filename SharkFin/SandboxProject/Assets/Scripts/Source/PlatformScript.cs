
using Shark;

namespace Sandbox
{
	
	public class PlatformScript : Entity
	{
		public Entity MarkerStart;
		public Entity MarkerEnd;

		private Vector2 m_StartPoint;
		private Vector2 m_EndPoint;
		public float LerpSpeed = 0.3f;

		private float m_Lerp = 0.0f;
		private float m_Direction = 1.0f;

		private RigidBody2DComponent m_RigidBody;

		protected override void OnCreate()
		{
			m_RigidBody = GetComponent<RigidBody2DComponent>();

			m_StartPoint = MarkerStart.Transform.Translation.XY;
			m_EndPoint = MarkerEnd.Transform.Translation.XY;
		}

		protected override void OnUpdate(float ts)
		{
			m_Lerp += ts * LerpSpeed * m_Direction;
			float t = (float)System.Math.Sin(m_Lerp);
			t = (t + 1.0f) * 0.5f;
			//&if (m_Lerp > 1.0f)
			//&{
			//&	m_Lerp = 1.0f;
			//&	m_Direction = -1.0f;
			//&}
			//&if (m_Lerp < 0.0f)
			//&{
			//&	m_Lerp = 0.0f;
			//&	m_Direction = 1.0f;
			//&}

			m_RigidBody.Position = Vector2.Lerp(m_StartPoint, m_EndPoint, t);
		}
	}
	
}
