#pragma once

namespace Shark {

	class Scene;
	class Entity;

	class SelectionContext
	{
	public:
		static bool AnySelected();
		static bool IsSingleSelection();
		static bool IsMultiSelection();
		static bool IsSelected(Entity entity);

		static const std::vector<Entity>& GetSelected();
		static const Entity GetFirstSelected();

		static void Clear();
		static void Select(Entity entity);
		static void Select(std::span<Entity> entities);
		static void Unselect(Entity entity);

		static void ClearAndSelect(Entity entity);
		static void ClearAndSelect(std::span<Entity> entities);

		static void SetActiveScene(Ref<Scene> scene);
		static Ref<Scene> GetActiveScene();

	};

}
