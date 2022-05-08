
namespace Shark
{
	public enum ComponentType
	{
		None,
		Component,
		Transform,
		SpriteRenderer,
		CameraComponent,

	}


	public class Component
	{
		protected readonly UUID m_EntityUUID;

		public Component(UUID entityUUID)
		{
			m_EntityUUID = entityUUID;
		}
	}

}
