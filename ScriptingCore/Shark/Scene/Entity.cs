using System;
using System.Runtime.CompilerServices;

namespace Shark
{
	public class Entity
	{
		private UUID m_Handle = new UUID(0);
		public UUID UUID => m_Handle;

		public Entity() {}

		public Entity(UUID handle)
		{
			m_Handle = handle;
		}

		protected virtual void OnCreate() {}
		protected virtual void OnDestroy() {}
		protected virtual void OnUpdate(TimeStep ts) {}
		protected virtual void OnCollishionBegin(Entity entity) {}
		protected virtual void OnCollishionEnd(Entity entity) {}


		public TransformComponent Transform => new TransformComponent(m_Handle);

		public string Name
		{
			get => InternalCalls.Entity_GetName(m_Handle);
			set => InternalCalls.Entity_SetName(m_Handle, value);
		}

		// Remove Component on RigidBody2D BoxCollider2D CircleCollider2D Components
		// will not work at the moment because there is no system to destroy b2Body/b2Fixtures
		// when the components get remove

		public bool HasComponent<T>() where T : Component
		{
			return InternalCalls.Entity_HasComponent(m_Handle, typeof(T));
		}

		public T GetComponent<T>() where T : Component
		{
			return (T)Activator.CreateInstance(typeof(T), m_Handle);
		}

		public T GetOrAddComponent<T>() where T : Component
		{
			if (!HasComponent<T>())
				InternalCalls.Entity_AddComponent(m_Handle, typeof(T));
			return GetComponent<T>();
		}
		public T AddComponent<T>() where T : Component
		{
			InternalCalls.Entity_AddComponent(m_Handle, typeof(T));
			return GetComponent<T>();
		}

	}

}
