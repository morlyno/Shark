
using Shark;

namespace Sandbox
{
	public class Bullet : Entity
	{
		private float m_MaxLifeTime = 5.0f;
		private float m_LifeTime = 0.0f;

		public bool DestroyOnHit = false;

		protected override void OnCreate()
		{
			Transform.Scale = new Vector3(0.2f);

			var renderer = CreateComponent<CircleRendererComponent>();
			renderer.Color = Color.Cyan;

			var rigidBody = CreateComponent<RigidBody2DComponent>();
			rigidBody.Bullet = true;
			var collider = CreateComponent<CircleCollider2DComponent>();
			collider.Density = 0.2f;
			collider.Friction = 0.0f;
			collider.Restitution = 1.0f;

			CollishionBeginEvent += (c) =>
			{
				if (DestroyOnHit)
					Destroy();
			};
		}

		protected override void OnUpdate(float ts)
		{
			m_LifeTime += ts;
			if (m_LifeTime >= m_MaxLifeTime)
				Destroy();
		}

	}
}
