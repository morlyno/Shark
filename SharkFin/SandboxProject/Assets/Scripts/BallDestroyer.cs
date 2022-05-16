using Shark;

using System.Collections.Generic;

namespace Sandbox
{
	
	public class BallDestroyer : Entity
	{
		private List<Entity> m_DestroyList = new List<Entity>(5);

		protected override void OnUpdate(TimeStep ts)
		{
			foreach (Entity entity in m_DestroyList)
			{
				Scene.Destroy(entity);
			}
			m_DestroyList.Clear();
		}

		protected override void OnCollishionEnd(Entity entity)
		{
			if (entity.Name == "Ball")
			{
				m_DestroyList.Add(entity);
			}
		}

	}
	
}
