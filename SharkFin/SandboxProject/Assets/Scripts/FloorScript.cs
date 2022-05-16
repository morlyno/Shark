using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using Shark;
using System.Runtime.InteropServices;

namespace Sandbox
{
	public class FloorScript : Entity
	{
		private Color m_InitColor;
		private Color m_CollishionColor;
		private UUID m_PlayerUUID;
		private static Random s_Rng = new Random();

		private SpriteRendererComponent m_SpriteRenderer;
		private AssetHandle m_CollishionTextureHandle = AssetHandle.Null;

		private static Color[] m_Colors = new Color[]{
			Color.White, Color.Back, Color.Red,
			Color.Green, Color.Blue, Color.Yellow,
			Color.Cyan, Color.Magenta, Color.Gray
		};

		protected override void OnCreate()
		{
			m_SpriteRenderer = GetComponent<SpriteRendererComponent>();
			m_SpriteRenderer.TilingFactor = 0.25f;

			m_InitColor = m_SpriteRenderer.Color;
			m_CollishionColor = m_Colors[s_Rng.Next(0, m_Colors.Length)];

			m_CollishionTextureHandle = ResourceManager.GetAssetHandleFromFilePath("Textures/Checkerboard.sktex");

			m_PlayerUUID = Scene.GetUUIDFromTag("Player");
		}

		protected override void OnCollishionBegin(Entity entity)
		{
			if (entity.UUID == m_PlayerUUID)
			{
				m_SpriteRenderer.Color = m_CollishionColor;
				m_SpriteRenderer.TextureHandle = m_CollishionTextureHandle;
			}
		}

		protected override void OnCollishionEnd(Entity entity)
		{
			if (entity.UUID == m_PlayerUUID)
			{
				m_SpriteRenderer.Color = m_InitColor;
				m_SpriteRenderer.TextureHandle = AssetHandle.Null;
			}
		}

	}

}
