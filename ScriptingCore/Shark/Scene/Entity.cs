using System;
using System.Collections.Generic;

namespace Shark
{
	public class Entity
	{
		public ulong ID { get; internal set; }

		private Dictionary<Type, Component> m_ComponentCache = new Dictionary<Type, Component>();

		public Entity()
		{
		}

		public Entity(ulong id)
		{
			ID = id;
		}

		protected virtual void OnCreate() { }
		protected virtual void OnDestroy() { }
		protected virtual void OnUpdate(TimeStep ts) { }
		protected virtual void OnPhysicsUpdate(TimeStep fixedTimeStep) { }
		protected virtual void OnUIRender() { }
		protected virtual void OnCollishionBegin(Entity entity, bool isSensor) { }
		protected virtual void OnCollishionEnd(Entity entity, bool isSensor) { }

		public TransformComponent Transform
			=> GetComponent<TransformComponent>();

		public string Name
		{
			get => GetComponent<TagComponent>().Tag;
			set => GetComponent<TagComponent>().Tag = value;
		}

		// Remove Component on RigidBody2D BoxCollider2D CircleCollider2D Components
		// will not work at the moment because there is no system to destroy b2Body/b2Fixtures
		// when the components get remove

		public bool HasComponent<T>() where T : Component
			=> InternalCalls.Entity_HasComponent(ID, typeof(T));

		public T GetComponent<T>() where T : Component, new()
		{
			Type type = typeof(T);
			m_ComponentCache.TryGetValue(type, out Component comp);

			if (comp == null)
			{
				comp = new T();
				comp.Entity = this;
				m_ComponentCache[type] = comp;
			}

			return comp as T;
		}

		public T AddComponent<T>() where T : Component, new()
		{
			InternalCalls.Entity_AddComponent(ID, typeof(T));
			return GetComponent<T>();
		}

		public void Entity_RemoveComponent<T>() where T : Component
		{
			Type type = typeof(T);
			InternalCalls.Entity_RemoveComponent(ID, type);
			m_ComponentCache.Remove(type);
		}

	}

}
