using Shark;

namespace Sandbox
{
	public class DoorSensor : Entity
	{
		public Entity Door;

		private Vector3 m_DoorPos;
		private float m_DoorMove = 6;

		private bool m_OpenDoor = false;
		private bool m_CloseDoor = false;
		private ulong m_CollishionCount = 0;

		protected override void OnCreate()
		{
			m_DoorPos = Door.WorldTransform.Translation;

			OnCollishionBegin += CollishionBegin;
			OnCollishionEnd += CollishionEnd;
		}

		protected override void OnUpdate(float ts)
		{
			if (m_OpenDoor)
			{
				var rigidBody = Door.GetComponent<RigidBody2DComponent>();
				rigidBody.Position += Vector2.Up * m_DoorMove;
				m_OpenDoor = false;
			}

			if (m_CloseDoor)
			{
				var rigidBody = Door.GetComponent<RigidBody2DComponent>();
				rigidBody.Position = m_DoorPos.XY;
				m_CloseDoor = false;
			}
		}

		protected void CollishionBegin(Collider2D collider)
		{
			Entity entity = collider.Entity;
			//if (!(entity is PlayerController))
			//	return;

			if (m_CollishionCount == 0)
				m_OpenDoor = true;

			m_CollishionCount++;
		}

		protected void CollishionEnd(Collider2D collider)
		{
			Entity entity = collider.Entity;
			//if (!(entity is PlayerController))
			//	return;

			if (m_CollishionCount == 1)
				m_CloseDoor = true;

			m_CollishionCount--;
		}

	}

}
