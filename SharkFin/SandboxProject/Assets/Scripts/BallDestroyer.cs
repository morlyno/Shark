using Shark;

using System.Collections.Generic;

namespace Sandbox
{
	
	public class BallDestroyer : Entity
	{
		protected override void OnCollishionEnd(Collider2D collider)
		{
			Entity entity = collider.Entity;
			if (entity.Name == "Ball")
			{
				Scene.Destroy(entity);
			}
		}

	}
	
}
