
using Shark;
using System.Runtime.InteropServices;

namespace Sandbox
{
	public class FloorScript : Entity
	{
		private Color m_InitColor;
		private Color m_CollishionColor;

		private SpriteRendererComponent m_SpriteRenderer;
		public AssetHandle m_CollishionTextureHandle = AssetHandle.Invalid;

		private uint m_CollishionCount = 0;

		private static Color[] m_Colors = new Color[]{
			Color.White, Color.Black, Color.Red,
			Color.Green, Color.Blue, Color.Yellow,
			Color.Cyan, Color.Magenta, Color.Gray
		};

		protected override void OnCreate()
		{
			m_SpriteRenderer = GetComponent<SpriteRendererComponent>();
			m_SpriteRenderer.TilingFactor = new Vector2(0.25f);

			m_InitColor = m_SpriteRenderer.Color;
			m_CollishionColor = m_Colors[Random.Int(0, m_Colors.Length)];

			OnCollishionBegin += CollishionBegin;
			OnCollishionEnd += CollishionEnd;
		}

		protected void CollishionBegin(Collider2D collider)
		{
			if (m_CollishionCount++ == 0)
			{
				m_SpriteRenderer.Color = m_CollishionColor;
				m_SpriteRenderer.TextureHandle = m_CollishionTextureHandle;
			}
		}

		protected void CollishionEnd(Collider2D collider)
		{
			if (--m_CollishionCount == 0)
			{
				m_SpriteRenderer.Color = m_InitColor;
				m_SpriteRenderer.TextureHandle = AssetHandle.Invalid;
			}
		}

	}

}
