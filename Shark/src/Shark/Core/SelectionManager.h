#pragma once

#include <span>

namespace Shark {

	class Scene;
	class Entity;

	class SelectionManager
	{
	public:
		static void DeselectAll();
		static void DeselectAll(UUID contextID);

		static void Select(UUID contextID, UUID id);
		static void Select(UUID contextID, std::span<UUID> ids);
		static void Unselect(UUID contextID, UUID id);
		static void Toggle(UUID contextID, UUID id, bool select);

		static bool AnySelected(UUID contextID);
		static bool IsSelected(UUID contextID, UUID id);
		static const std::vector<UUID>& GetSelections(UUID contextID);
		static UUID GetFirstSelected(UUID contextID);
		static UUID GetLastSelected(UUID contextID);

		static bool IsEntityOrAncestorSelected(UUID contextID, Entity entity);

		static void SetActiveScene(Ref<Scene> scene);
		static Ref<Scene> GetActiveScene();

	};

}
