
using Shark;

namespace Sandbox
{
	public class Bullet : Entity
	{
		private float m_MaxLifeTime = 5.0f;
		private float m_LifeTime = 0.0f;

		protected override void OnCreate()
		{
			Transform.Scale = new Vector3(0.2f);

			var renderer = AddComponent<CircleRendererComponent>();
			renderer.Color = Color.Cyan;

			var rigidBody = AddComponent<RigidBody2DComponent>();
			rigidBody.Bullet = true;
			var collider = AddComponent<CircleCollider2DComponent>();
			collider.Density = 0.2f;
			collider.Friction = 0.1f;
			collider.Restitution = 1.0f;
		}

		protected override void OnUpdate(TimeStep ts)
		{
			m_LifeTime += ts;
			if (m_LifeTime >= m_MaxLifeTime)
				Scene.Destroy(this);
		}
	}
}
