
namespace Shark
{
	public class BoxCollider2DComponent : PhysicsCollider2D
	{
		public Vector2 Size
		{
			get => InternalCalls.BoxCollider2DComponent_GetSize(m_EntityUUID);
			set => InternalCalls.BoxCollider2DComponent_SetSize(m_EntityUUID, value);
		}

		public Vector2 Offset
		{
			get => InternalCalls.BoxCollider2DComponent_GetOffset(m_EntityUUID);
			set => InternalCalls.BoxCollider2DComponent_SetOffset(m_EntityUUID, value);
		}

		public float Rotation
		{
			get => InternalCalls.BoxCollider2DComponent_GetRotation(m_EntityUUID);
			set => InternalCalls.BoxCollider2DComponent_SetRotation(m_EntityUUID, value);
		}

		public BoxCollider2DComponent(UUID owner)
			: base(owner, InternalCalls.BoxCollider2DComponent_GetNativeHandle(owner))
		{
		}

	}

}
