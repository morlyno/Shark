using System;

namespace Shark
{
	public class Entity
	{
		private UUID m_Handle = new UUID(0);
		private Scene m_Scene = null;

		public UUID Handle => m_Handle;
		public Scene Scene => m_Scene;

		public virtual void OnCreate() {}
		public virtual void OnDestroy() {}
		public virtual void OnUpdate(TimeStep ts) {}
	}

}
