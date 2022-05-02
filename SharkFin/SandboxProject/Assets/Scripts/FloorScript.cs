using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using Shark;

namespace Sandbox
{
	public class FloorScript : Entity
	{
		private Color m_InitColor;
		private Color m_CollishionColor;
		private UUID m_PlayerUUID;
		private static Random s_Rng = new Random();

		private SpriteRendererComponent m_SpriteRenderer;

		private static Color[] m_Colors = new Color[]{
			Color.White, Color.Back, Color.Red,
			Color.Green, Color.Blue, Color.Yellow,
			Color.Cyan, Color.Magenta, Color.Gray
		};

		void OnCreate()
		{
			m_SpriteRenderer = GetComponent<SpriteRendererComponent>();
			m_InitColor = m_SpriteRenderer.Color;
			m_CollishionColor = m_Colors[s_Rng.Next(0, m_Colors.Length)];

			m_PlayerUUID = Scene.GetUUIDFromTag("Player");
		}

		void OnCollishionBegin(Entity entity)
		{
			if (entity.UUID == m_PlayerUUID)
				m_SpriteRenderer.Color = m_CollishionColor;
		}
		
		void OnCollishionEnd(Entity entity)
		{
			if (entity.UUID == m_PlayerUUID)
				m_SpriteRenderer.Color = m_InitColor;
		}

	}

}
