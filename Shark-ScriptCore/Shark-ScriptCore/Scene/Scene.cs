
using System;

namespace Shark
{
	public static class Scene
	{
		public static T Instantiate<T>(string name) where T : Entity
			=> InternalCalls.Scene_Instantiate(typeof(T), name) as T;

		public static Entity Instantiate(string name)
		{
			ulong entityID = InternalCalls.Scene_CreateEntity(name, 0);
			return new Entity(entityID);
		}

		public static void Destroy(Entity entity)
			=> InternalCalls.Scene_DestroyEntity(entity.ID);

		public static Entity CloneEntity(Entity entity)
		{
			ulong entityID = InternalCalls.Scene_CloneEntity(entity.ID);
			return GetEntityByID(entityID);
		}

		// Get Entity by UUID
		// returns either a script or a basic Entity
		// if the uuid is invalid the return value is null
		public static Entity GetEntityByID(ulong id)
			=> InternalCalls.Scene_GetEntityByID(id);

		public static ulong GetIDFromTag(string tag)
			=> InternalCalls.Scene_GetIDFromTag(tag);

		public static Entity GetEntityByTag(string tag)
		{
			ulong entityID = InternalCalls.Scene_GetIDFromTag(tag);
			return GetEntityByID(entityID);
		}

		public static ulong GetActiveCameraID()
			=> InternalCalls.Scene_GetActiveCameraID();

		public static Entity GetActiveCamera()
		{
			ulong id = GetActiveCameraID();
			return GetEntityByID(id);
		}

	}

}
