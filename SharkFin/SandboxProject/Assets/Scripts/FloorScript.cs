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
		private Color m_CollishionColor = Color.Cyan;
		private UUID m_PlayerUUID;

		private SpriteRendererComponent m_SpriteRenderer;

		void OnCreate()
		{
			m_SpriteRenderer = GetComponent<SpriteRendererComponent>();
			m_InitColor = m_SpriteRenderer.Color;

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
