
using Shark;

namespace Sandbox
{
	public class StressTest : Entity
	{
		private System.Random m_Random = new System.Random();

		public int Range = 10;

		protected override void OnCreate()
		{
			for (int y = -Range; y <= Range; y++)
			{
				for (int x = -Range; x <= Range; x++)
				{
					Entity entity = Instantiate("Cube");
					entity.Parent = this;
					var transform = entity.Transform.LocalTransform;
					transform.Translation = new Vector3(x, y, 0.0f);
					transform.Translation.XY += new Vector2(((float)m_Random.NextDouble() - 0.5f) * 0.2f, ((float)m_Random.NextDouble() - 0.5f) * 0.2f);
					transform.Scale.XY = new Vector2(0.5f);
					entity.Transform.LocalTransform = transform;

					entity.AddComponent<RigidBody2DComponent>();
					entity.AddComponent<BoxCollider2DComponent>();
					var spriteRenderer = entity.AddComponent<SpriteRendererComponent>();
					spriteRenderer.Color = new Color(
						(float)m_Random.NextDouble(),
						(float)m_Random.NextDouble(),
						(float)m_Random.NextDouble()
					);
				}
			}

		}
	}
}
