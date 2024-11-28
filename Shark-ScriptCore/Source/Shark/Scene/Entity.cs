
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics.Tracing;
using System.Runtime.InteropServices;

namespace Shark
{
	public class Entity
	{
		protected Entity()
		{
		}

		internal Entity(ulong id)
		{
			ID = id;
		}

		protected virtual void OnCreate() { }
		protected virtual void OnDestroy() { }
		protected virtual void OnUpdate(float ts) { }
		protected virtual void OnPhysicsUpdate(float fixedTimeStep) { }
		protected virtual void OnUIRender() { }

		public event Action<Entity>? CollishionBeginEvent;
		public event Action<Entity>? CollishionEndEvent;
		public event Action<Entity>? TriggerBeginEvent;
		public event Action<Entity>? TriggerEndEvent;
		public event Action<Entity>? DestroyedEvent;

		private Dictionary<Type, Component> m_ComponentCache = new();
		private TransformComponent? m_TransformComponent;
		private Entity? m_Parent;

		[HideFromEditor]
		public readonly ulong ID;
		public string Name => GetComponent<TagComponent>()!.Tag;
		public Transform WorldTransform => Transform.WorldTransform;
		public TransformComponent Transform
		{
			get
			{
				if (m_TransformComponent == null)
					m_TransformComponent = GetComponent<TransformComponent>()!;
				return m_TransformComponent;
			}
		}

		public Vector3 Translation
		{
			get => Transform.Translation;
			set => Transform.Translation = value;
		}

		public Vector3 Rotation
		{
			get => Transform.Rotation;
			set => Transform.Rotation = value;
		}

		public Vector3 Scale
		{
			get => Transform.Scale;
			set => Transform.Scale = value;
		}

		public Entity? Parent
		{
			get
			{
				unsafe
				{
					ulong parentID = InternalCalls.Entity_GetParent(ID);

					if (m_Parent == null || m_Parent.ID != parentID)
						m_Parent = InternalCalls.Scene_IsEntityValid(parentID) ? new(parentID) : null;

				}
				return m_Parent;
			}
			set { unsafe { InternalCalls.Entity_SetParent(ID, value?.ID ?? 0); } }
		}

		public Entity[] Children
		{
			get
			{
				Entity[] children;
				unsafe
				{
					var childIDs = InternalCalls.Entity_GetChildren(ID);
					children = new Entity[childIDs.Length];
					for (int i = 0; i < childIDs.Length; i++)
						children[i] = new Entity(childIDs[i]);
				}
				return children;
			}
		}


		public bool HasComponent<T>() where T : Component
		{
			unsafe { return InternalCalls.Entity_HasComponent(ID, typeof(T)); }
		}

		public T? GetComponent<T>() where T : Component, new()
		{
			Type componentType = typeof(T);

			if (!HasComponent<T>())
			{
				m_ComponentCache.Remove(componentType);
				return null;
			}

			if (!m_ComponentCache.ContainsKey(componentType))
			{
				var component = new T { Entity = this };
				m_ComponentCache.Add(componentType, component);
				return component;
			}

			return m_ComponentCache[componentType] as T;
		}

		public T? CreateComponent<T>() where T : Component, new()
		{
			if (HasComponent<T>())
				return GetComponent<T>();

			unsafe { InternalCalls.Entity_AddComponent(ID, typeof(T)); }
			var component = new T { Entity = this };
			m_ComponentCache.Add(typeof(T), component);
			return component;
		}

		public void RemoveComponent<T>() where T : Component
		{
			unsafe { InternalCalls.Entity_RemoveComponent(ID, typeof(T)); }
		}

		public T As<T>() where T : Entity
		{
			throw new NotImplementedException();
		}


		public void Destroy() => Scene.DestroyEntity(this);
		public void Destroy(Entity entity) => Scene.DestroyEntity(entity);

		public Entity? FindEntityByTag(string tag) => Scene.FindEntityByTag(tag);
		public Entity? FindEntityByID(ulong entityID) => Scene.FindEntityByID(entityID);

		internal void InvokeCollishionBeginEvent(ulong entityID) => CollishionBeginEvent?.Invoke(new Entity(entityID));
		internal void InvokeCollishionEndEvent(ulong entityID) => CollishionEndEvent?.Invoke(new Entity(entityID));
		internal void InvokeTriggerBeginEvent(ulong entityID) => TriggerBeginEvent?.Invoke(new Entity(entityID));
		internal void InvokeTriggerEndEvent(ulong entityID) => TriggerEndEvent?.Invoke(new Entity(entityID));
		internal void InvokeOnDestroyed()
		{
			DestroyedEvent?.Invoke(this);
			OnDestroy();
			m_ComponentCache.Clear();
		}

		[UnmanagedCallersOnly]
		internal static void InvokeOnUpdate(Coral.Managed.Interop.NativeInstance<Entity> instance, float ts)
		{
			instance.Get()?.OnUpdate(ts);
		}

	}

}
