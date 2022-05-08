
namespace Shark
{
	public class TransformComponent : Component
	{
		public Vector3 Translation
		{
			get => InternalCalls.TransformComponent_GetTranslation(m_EntityUUID);
			set => InternalCalls.TransformComponent_SetTranslation(m_EntityUUID, value);
		}

		public Vector3 Rotation
		{
			get => InternalCalls.TransformComponent_GetRotation(m_EntityUUID);
			set => InternalCalls.TransformComponent_SetRotation(m_EntityUUID, value);
		}

		public Vector3 Scaling
		{
			get => InternalCalls.TransformComponent_GetScaling(m_EntityUUID);
			set => InternalCalls.TransformComponent_SetScaling(m_EntityUUID, value);
		}

		public TransformComponent(UUID owner)
			: base(owner)
		{
		}
	}

}
