
using System;

namespace Shark
{
	public static class Scene
	{
		// Creates new Entity with Script of type T
		public static T Instantiate<T>(string name) where T : Entity
		{
			return (T)InternalCalls.Scene_InstantiateScript(typeof(T), name);
		}

		// Creates new Entity
		public static Entity Instantiate(string name)
		{
			/*UUID uuid =*/ InternalCalls.Scene_CreateEntity(name, UUID.Null, out UUID out_uuid);
			return new Entity(out_uuid);
		}

		// Destroyes Entity
		public static void Destroy(Entity entity)
		{
			InternalCalls.Scene_DestroyEntity(entity.UUID);
		}

		// Creates new Entity
		public static Entity CloneEntity(Entity entity)
		{
			InternalCalls.Scene_CloneEntity(entity.UUID, out UUID uuid);
			return GetEntityByUUID(uuid);
		}

		// Get Entity by UUID
		// returns either a script or a basic Entity
		// if the uuid is invalid the return value is null
		public static Entity GetEntityByUUID(UUID uuid)
		{
			Entity entity = InternalCalls.Scene_GetScriptObject(uuid);
			if (entity != null)
				return entity;
			if (InternalCalls.Scene_IsValidEntityHandle(uuid))
				return new Entity(uuid);
			return null;
		}

		public static UUID GetUUIDFromTag(string tag)
		{
			InternalCalls.Scene_GetUUIDFromTag(tag, out UUID uuid);
			return uuid;
		}

		public static Entity GetEntityByTag(string tag)
		{
			InternalCalls.Scene_GetUUIDFromTag(tag, out UUID uuid);
			return GetEntityByUUID(uuid);
		}

		// Returns the active Camera UUID
		public static UUID GetActiveCameraUUID()
		{
			InternalCalls.Scene_GetActiveCameraUUID(out UUID uuid);
			return uuid;
		}

		public static Entity GetActiveCameraEntity()
		{
			UUID uuid = GetActiveCameraUUID();
			return GetEntityByUUID(uuid);
		}

	}

}
