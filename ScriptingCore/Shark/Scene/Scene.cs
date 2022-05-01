
using System;

namespace Shark
{
	public static class Scene
	{
		// Creates new Entity with Script of type T
		public static T Instantiate<T>(string name) where T : Entity
		{
			return (T)InternalCalls.Scene_InstantiateScript(name, typeof(T));
		}

		// Creates new Entity
		public static Entity Instantiate(string name)
		{
			UUID out_uuid = new UUID();
			/*UUID uuid =*/ InternalCalls.Scene_CreateEntity(name, UUID.Null, ref out_uuid);
			return new Entity(out_uuid);
		}

		// Destroyes Entity
		public static void Destroy(Entity entity)
		{
			InternalCalls.Scene_DestroyEntity(entity.Handle);
		}

		// Get Entity by UUID
		// returns either a script or a basic Entity
		// if the uuid invalid the return value is null
		public static Entity GetEntityByUUID(UUID uuid)
		{
			Entity entity = InternalCalls.Scene_GetScriptObject(uuid);
			if (entity != null)
				return entity;
			if (InternalCalls.Scene_IsValidEntityHandle(uuid))
				return new Entity(uuid);
			return null;
		}

		// Returns the active Camera UUID
		public static UUID GetActiveCameraUUID()
		{
			return InternalCalls.Scene_GetActiveCameraUUID();
		}

		public static Entity GetActiveCameraEntity()
		{
			UUID uuid = GetActiveCameraUUID();
			return GetEntityByUUID(uuid);
		}

	}

}
