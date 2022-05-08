
namespace Shark
{
	public class CircleRendererComponent : Component
	{
		public Color Color
		{
			get => InternalCalls.CircleRendererComponent_GetColor(m_EntityUUID);
			set => InternalCalls.CircleRendererComponent_SetColor(m_EntityUUID, value);
		}

		public float Thickness
		{
			get => InternalCalls.CircleRendererComponent_GetThickness(m_EntityUUID);
			set => InternalCalls.CircleRendererComponent_SetThickness(m_EntityUUID, value);
		}

		public float Fade
		{
			get => InternalCalls.CircleRendererComponent_GetFade(m_EntityUUID);

			set => InternalCalls.CircleRendererComponent_SetFade(m_EntityUUID, value);
		}

		public CircleRendererComponent(UUID entityUUID)
			: base(entityUUID)
		{
		}
	}
}
