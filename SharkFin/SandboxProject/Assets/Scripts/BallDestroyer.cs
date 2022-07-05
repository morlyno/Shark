using Shark;

using System.Collections.Generic;

namespace Sandbox
{
	
	public class BallDestroyer : Entity
	{
		protected override void OnCollishionEnd(Entity entity, bool isSensor)
		{
			if (entity.Name == "Ball")
			{
				Scene.Destroy(entity);
			}
		}

	}
	
}
