#pragma once

namespace Shark {

	class Scene;

	enum class SelectionContext
	{
		Entity = 0, ContentBrowser = 1
	};

	class SelectionManager
	{
	public:
		static void Clear(SelectionContext context);
		static void ClearAll();

		static void Select(SelectionContext context, UUID id);
		static void Select(SelectionContext context, std::span<UUID> ids);
		static void Unselect(SelectionContext context, UUID id);
		static void Toggle(SelectionContext context, UUID id, bool select);

		static bool AnySelected(SelectionContext context);
		static bool IsSelected(SelectionContext context, UUID id);
		static const std::vector<UUID>& GetSelections(SelectionContext context);

		static void SetActiveScene(Ref<Scene> scene);
		static Ref<Scene> GetActiveScene();

	};

}
