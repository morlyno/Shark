
using Shark;

namespace Sandbox
{
	
	public class PlatformScript : Entity
	{
		private Vector2 m_LeftPoint;
		private Vector2 m_RightPoint;

		private float m_Lerp = 0.0f;
		private float m_LerpSpeed = 0.3f;
		private float m_Direction = 1.0f;

		private RigidBody2DComponent m_RigidBody;

		protected override void OnCreate()
		{
			m_RigidBody = GetComponent<RigidBody2DComponent>();

			Entity markerLeft = Scene.GetEntityByTag("PlatformMarkerLeft");
			Entity markerRight = Scene.GetEntityByTag("PlatformMarkerRight");

			m_LeftPoint = markerLeft.Transform.Translation.XY;
			m_RightPoint = markerRight.Transform.Translation.XY;
		}

		protected override void OnUpdate(float ts)
		{
			m_Lerp += ts * m_LerpSpeed * m_Direction;
			if (m_Lerp > 1.0f)
			{
				m_Lerp = 1.0f;
				m_Direction = -1.0f;
			}
			if (m_Lerp < 0.0f)
			{
				m_Lerp = 0.0f;
				m_Direction = 1.0f;
			}

			m_RigidBody.Position = Vector2.Lerp(m_LeftPoint, m_RightPoint, m_Lerp);
		}
	}
	
}
