﻿
using System;
using System.Collections.ObjectModel;
using System.Diagnostics.Tracing;

namespace Shark
{
	public class Entity
	{
		public readonly ulong ID;

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
		//protected virtual void OnCollishionBegin(Collider2D collider) { }
		//protected virtual void OnCollishionEnd(Collider2D collider) { }
		//protected virtual void OnTriggerBegin(Collider2D collider) { }
		//protected virtual void OnTriggerEnd(Collider2D collider) { }

		public event Action<Collider2D> OnCollishionBegin;
		public event Action<Collider2D> OnCollishionEnd;
		public event Action<Collider2D> OnTriggerBegin;
		public event Action<Collider2D> OnTriggerEnd;

		internal void InvokeOnCollishionBegin(Collider2D collider) => OnCollishionBegin?.Invoke(collider);
		internal void InvokeOnCollishionEnd(Collider2D collider) => OnCollishionEnd?.Invoke(collider);
		internal void InvokeOnTriggerBegin(Collider2D collider) => OnTriggerBegin?.Invoke(collider);
		internal void InvokeOnTriggerEnd(Collider2D collider) => OnTriggerEnd?.Invoke(collider);

		public TransformComponent Transform
			=> GetComponent<TransformComponent>();

		public Transform WorldTransform
			=> Transform.WorldTransform;

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

		public string Name
		{
			get => GetComponent<TagComponent>().Tag;
			set => GetComponent<TagComponent>().Tag = value;
		}

		public Entity Parent
		{
			get => InternalCalls.Entity_GetParent(ID);
			set => InternalCalls.Entity_SetParent(ID, value.ID);
		}

		public Entity[] Children
			=> InternalCalls.Entity_GetChildren(ID);


		public bool HasComponent<T>() where T : Component
			=> InternalCalls.Entity_HasComponent(ID, typeof(T));

		public T GetComponent<T>() where T : Component, new()
		{
			if (!HasComponent<T>())
				return null;

			T comp = new T();
			comp.Entity = this;
			return comp;
		}

		public T AddComponent<T>() where T : Component, new()
		{
			InternalCalls.Entity_AddComponent(ID, typeof(T));
			return GetComponent<T>();
		}

		public void RemoveComponent<T>() where T : Component
			=> InternalCalls.Entity_RemoveComponent(ID, typeof(T));


		public T As<T>() where T : Entity
			=> InternalCalls.Entity_GetInstance(ID) as T;

		public T Instantiate<T>(string name) where T : Entity
		{
			object instance = InternalCalls.Entity_Instantiate(typeof(T), name);
			return instance as T;
		}
		
		public Entity Instantiate(string name)
		{
			ulong id = InternalCalls.Entity_CreateEntity(name);
			return new Entity(id);
		}

		public void DestroyEntity(Entity entity, bool destroyChildren = true)
			=> InternalCalls.Entity_DestroyEntity(entity.ID, destroyChildren);

		public Entity CloneEntity(Entity entity)
		{
			ulong id = InternalCalls.Entity_CloneEntity(entity.ID);
			return new Entity(id);
		}

		public Entity FindEntityByName(string name)
		{
			ulong id = InternalCalls.Entity_FindEntityByName(name);
			return new Entity(id);
		}

		public Entity FindChildEntityByName(string name, bool recusive = true)
		{
			ulong id = InternalCalls.Entity_FindChildEntityByName(ID, name, recusive);
			return new Entity(id);
		}

		public Entity GetEntityByID(ulong entityID)
			=> new Entity(entityID);

	}

}
