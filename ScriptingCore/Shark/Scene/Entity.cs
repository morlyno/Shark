using System;
using System.Runtime.CompilerServices;

namespace Shark
{
	public class Entity
	{
		private UUID m_Handle = new UUID(0);
		public UUID Handle => m_Handle;

		public Entity() {}
		public Entity(UUID handle)
		{
			m_Handle = handle;
		}

		public TransformComponent Transform => new TransformComponent(m_Handle);

		public string Name
		{
			get => InternalCalls.Entity_GetName(m_Handle);
			set => InternalCalls.Entity_SetName(m_Handle, value);
		}

		public bool HasComponent<T>() where T : Component
		{
			return InternalCalls.Entity_HasComponent(m_Handle, typeof(T));
		}

		public T GetComponent<T>() where T : Component
		{
			return (T)Activator.CreateInstance(typeof(T), m_Handle);
		}

		public T AddComponent<T>() where T : Component
		{
			InternalCalls.Entity_AddComponent(m_Handle, typeof(T));
			return GetComponent<T>();
		}

	}

}
