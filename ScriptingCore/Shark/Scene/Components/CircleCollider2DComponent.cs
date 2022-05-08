
using System;

namespace Shark
{
	public class CircleCollider2DComponent : PhysicsCollider2D
	{
		public float Radius
		{
			get => InternalCalls.CircleCollider2DComponent_GetRadius(m_EntityUUID);
			set => InternalCalls.CircleCollider2DComponent_SetRadius(m_EntityUUID, value);
		}

		public Vector2 Offset
		{
			get => InternalCalls.CircleCollider2DComponent_GetOffset(m_EntityUUID);
			set => InternalCalls.CircleCollider2DComponent_SetOffset(m_EntityUUID, value);
		}

		public float Rotation
		{
			get => InternalCalls.CircleCollider2DComponent_GetRotation(m_EntityUUID);
			set => InternalCalls.CircleCollider2DComponent_SetRotation(m_EntityUUID, value);
		}

		public CircleCollider2DComponent(UUID owner)
			: base(owner, InternalCalls.CircleCollider2DComponent_GetNativeHandle(owner))
		{
		}
	}
}
