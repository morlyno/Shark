
namespace Shark
{
	public class SpriteRendererComponent : Component
	{
		public Color Color
		{
			get => InternalCalls.SpriteRendererComponent_GetColor(m_EntityUUID);
			set => InternalCalls.SpriteRendererComponent_SetColor(m_EntityUUID, value);
		}

		public AssetHandle TextureHandle
		{
			get => InternalCalls.SpriteRendererComponent_GetTextureHandle(m_EntityUUID);
			set => InternalCalls.SpriteRendererComponent_SetTextureHandle(m_EntityUUID, value);
		}

		public float TilingFactor
		{
			get => InternalCalls.SpriteRendererComponent_GetTilingFactor(m_EntityUUID);
			set => InternalCalls.SpriteRendererComponent_SetTilingFactor(m_EntityUUID, value);
		}

		public SpriteRendererComponent(UUID owner)
			: base(owner)
		{
		}
	}

}
