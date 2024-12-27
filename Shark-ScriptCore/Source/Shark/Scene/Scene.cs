using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Shark
{
    public class Scene
    {
        internal static Dictionary<ulong, Entity> s_EntityCache = new();

        public static Entity CreateEntity(string tag = "Unnamed")
        {
            unsafe
            {
                return new Entity(InternalCalls.Scene_CreateEntity(tag));
            }
        }

        public static Entity InstantiatePrefab(Prefab prefab)
        {
            unsafe
            {
                return new Entity(InternalCalls.Scene_InstantiatePrefab(prefab.Handle, null, null, null));
            }
        }
        
        public static Entity InstantiatePrefab(Prefab prefab, Vector3 translation)
        {
            unsafe
            {
                return new Entity(InternalCalls.Scene_InstantiatePrefab(prefab.Handle, &translation, null, null));
            }
        }
        
        public static Entity InstantiatePrefab(Prefab prefab, Transform transform)
        {
            unsafe
            {
                return new Entity(InternalCalls.Scene_InstantiateChildPrefab(prefab.Handle, 0, &transform.Translation, &transform.Rotation, &transform.Scale));
            }
        }
        
        public static Entity InstantiatePrefab(Prefab prefab, Entity parent)
        {
            unsafe
            {
                return new Entity(InternalCalls.Scene_InstantiateChildPrefab(prefab.Handle, parent.ID, null, null, null));
            }
        }
        
        public static Entity InstantiatePrefab(Prefab prefab, Entity parent, Vector3 translation)
        {
            unsafe
            {
                return new Entity(InternalCalls.Scene_InstantiateChildPrefab(prefab.Handle, parent.ID, &translation, null, null));
            }
        }
        
        public static Entity InstantiatePrefab(Prefab prefab, Entity parent, Transform transform)
        {
            unsafe
            {
                return new Entity(InternalCalls.Scene_InstantiateChildPrefab(prefab.Handle, parent.ID, &transform.Translation, &transform.Rotation, &transform.Scale));
            }
        }


        public static void DestroyEntity(Entity entity)
        {
            if (entity == null)
                return;

            unsafe
            {
                if (!InternalCalls.Scene_IsEntityValid(entity.ID))
                    return;

                s_EntityCache.Remove(entity.ID);

                InternalCalls.Scene_DestroyEntity(entity.ID);
            }
        }

        public static Entity? FindEntityByTag(string tag)
        {
            unsafe
            {
                ulong entityID = InternalCalls.Scene_FindEntityByTag(tag);
                return FindEntityByID(entityID);
            }
        }

        public static Entity? FindEntityByID(ulong entityID)
        {
            unsafe
            {
                if (s_EntityCache.ContainsKey(entityID) && s_EntityCache[entityID] != null)
                {
                    var entity = s_EntityCache[entityID];

                    if (!InternalCalls.Scene_IsEntityValid(entity.ID))
                    {
                        s_EntityCache.Remove(entityID);
                        entity = null;
                    }

                    return entity;
                }

                if (!InternalCalls.Scene_IsEntityValid(entityID))
                    return null;

                Entity newEntity = new(entityID);
                s_EntityCache[entityID] = newEntity;
                newEntity.DestroyedEvent += OnEntityDestroyed;
                return newEntity;
            }
        }

        private static void OnEntityDestroyed(Entity entity)
        {
            entity.DestroyedEvent -= OnEntityDestroyed;
            s_EntityCache.Remove(entity.ID);
        }

    }
}
