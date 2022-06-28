
using System;

namespace Shark
{
	public static class Scene
	{
		public static T Instantiate<T>(string name) where T : Entity
			=> InternalCalls.Scene_InstantiateScript(typeof(T), name) as T;

		public static Entity Instantiate(string name)
		{
			InternalCalls.Scene_CreateEntity(name, 0, out ulong id);
			return new Entity(id);
		}

		public static void Destroy(Entity entity)
			=> InternalCalls.Scene_DestroyEntity(entity.ID);

		public static Entity CloneEntity(Entity entity)
		{
			InternalCalls.Scene_CloneEntity(entity.ID, out ulong id);
			return GetEntityByID(id);
		}

		// Get Entity by UUID
		// returns either a script or a basic Entity
		// if the uuid is invalid the return value is null
		public static Entity GetEntityByID(ulong id)
		{
			Entity entity = InternalCalls.Scene_GetScriptObject(id);
			if (entity != null)
				return entity;
			if (InternalCalls.Scene_IsValidEntityHandle(id))
				return new Entity(id);
			return null;
		}

		public static ulong GetIDFromTag(string tag)
		{
			if (InternalCalls.Scene_GetUUIDFromTag(tag, out ulong id))
				return id;
			return 0;
		}

		public static Entity GetEntityByTag(string tag)
		{
			if (InternalCalls.Scene_GetUUIDFromTag(tag, out ulong id))
				return GetEntityByID(id);
			return null;
		}

		public static ulong GetActiveCameraID()
		{
			InternalCalls.Scene_GetActiveCameraUUID(out ulong id);
			return id;
		}

		public static Entity GetActiveCamera()
		{
			ulong id = GetActiveCameraID();
			return GetEntityByID(id);
		}

	}

}
