using Shark;

using System.Collections.Generic;

namespace Sandbox
{
	
	public class BallDestroyer : Entity
	{
		protected override void OnCreate()
		{
			OnCollishionBegin += (collider) =>
			{
				Entity entity = collider.Entity;
				if (entity.Name == "Ball")
					DestroyEntity(entity);
			};
		}
	}
	
}
