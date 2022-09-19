using System;
using System.Collections.Generic;

namespace Shark
{
	public class Entity
	{
		public readonly ulong ID;
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
		protected virtual void OnUpdate(float ts) { }
		protected virtual void OnPhysicsUpdate(float fixedTimeStep) { }
		protected virtual void OnUIRender() { }
		protected virtual void OnCollishionBegin(Collider2D collider) { }
		protected virtual void OnCollishionEnd(Collider2D collider) { }
		protected virtual void OnTriggerBegin(Collider2D collider) { }
		protected virtual void OnTriggerEnd(Collider2D collider) { }


		public TransformComponent Transform
			=> GetComponent<TransformComponent>();

		public Vector3 Translation
		{
			get => Transform.Translation;
			set => Transform.Translation = value;
		}

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
			if (!HasComponent<T>())
				return null;

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

		public void RemoveComponent<T>() where T : Component
		{
			Type type = typeof(T);
			InternalCalls.Entity_RemoveComponent(ID, type);
			m_ComponentCache.Remove(type);
		}

	}

}
