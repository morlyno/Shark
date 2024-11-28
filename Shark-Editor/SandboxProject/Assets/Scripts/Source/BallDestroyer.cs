using Shark;

namespace Sandbox
{
	
	public class BallDestroyer : Entity
	{
		protected override void OnCreate()
		{
			CollishionBeginEvent += (entity) =>
			{
				if (entity.Name == "Ball")
					Destroy(entity);
			};
		}
	}
	
}
