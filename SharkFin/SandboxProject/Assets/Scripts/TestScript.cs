using Shark;
using System;

namespace Sandbox
{
	public class TestScript : Entity
	{
		private float m_AnimationTimer = 0.0f;
		private float m_AnimationSpeed = 2.0f;
		private float m_XRadius = 2.0f;
		private float m_YRadius = 2.0f;

		private SpriteRendererComponent m_SpriteRenderer;
		private float m_DeltaMode = 1.0f;
		private Color m_DeltaColor = new Color(1.0f, 0.0f, 0.0f, 0.0f);
		private int m_ColorIndex = 0;

		private Vector3 m_Offset;

		public Vector3 Offset { set { m_Offset = value; } }

		void OnCreate()
		{
			m_SpriteRenderer = GetOrAddComponent<SpriteRendererComponent>();
			m_SpriteRenderer.Color = Color.Back;

			m_Offset = Transform.Translation;
		}

		void OnUpdate(TimeStep ts)
		{
			m_AnimationTimer += ts * m_AnimationSpeed;
			m_AnimationSpeed %= (float)Math.PI;

			var translation = Transform.Translation;
			translation.X = (float)Math.Sin(m_AnimationTimer) * m_XRadius;
			translation.Y = (float)Math.Cos(m_AnimationTimer) * m_YRadius;
			Transform.Translation = translation + m_Offset;

			var color = m_SpriteRenderer.Color;
			color += m_DeltaColor * ts;
			if (color[m_ColorIndex] >= 1.0f || color[m_ColorIndex] < 0.0f)
			{
				if (color[m_ColorIndex] > 1.0f)
					color[m_ColorIndex] = 1.0f;
				if (color[m_ColorIndex] < 0.0f)
					color[m_ColorIndex] = 0.0f;

				m_DeltaColor[m_ColorIndex] = 0.0f;
				m_ColorIndex++;
				if (m_ColorIndex >= 3)
				{
					m_DeltaMode = -m_DeltaMode;
					m_ColorIndex = 0;
				}

				m_DeltaColor[m_ColorIndex] = m_DeltaMode;
			}

			m_SpriteRenderer.Color = color;
		}

	}

}
