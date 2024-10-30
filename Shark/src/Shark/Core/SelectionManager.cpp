#include "skpch.h"
#include "SelectionManager.h"

#include "Shark/Scene/Scene.h"

#include "Shark/Debug/Profiler.h"

namespace Shark {

	struct SelectionContextData
	{
		Ref<Scene> m_Scene;
		std::map<SelectionContext, std::vector<UUID>> m_Selections;
	};

	static SelectionContextData s_SelectionData;

	void SelectionManager::Clear(SelectionContext context)
	{
		auto& selections = s_SelectionData.m_Selections[context];
		selections.clear();
	}

	void SelectionManager::ClearAll()
	{
		for (auto& selections : s_SelectionData.m_Selections)
			selections.second.clear();
	}

	void SelectionManager::Select(SelectionContext context, UUID id)
	{
		auto& selections = s_SelectionData.m_Selections[context];
		selections.push_back(id);
	}

	void SelectionManager::Select(SelectionContext context, std::span<UUID> entities)
	{
		auto& selections = s_SelectionData.m_Selections[context];
		std::ranges::merge(selections, entities, selections.end());
	}

	void SelectionManager::Unselect(SelectionContext context, UUID id)
	{
		auto& selections = s_SelectionData.m_Selections[context];
		std::erase(selections, id);
	}

	void SelectionManager::Toggle(SelectionContext context, UUID id, bool select)
	{
		if (select)
			Select(context, id);
		else
			Unselect(context, id);
	}

	bool SelectionManager::AnySelected(SelectionContext context)
	{
		return !s_SelectionData.m_Selections[context].empty();
	}

	bool SelectionManager::IsSelected(SelectionContext context, UUID id)
	{
		const auto& selections = s_SelectionData.m_Selections[context];
		return std::find(selections.begin(), selections.end(), id) != selections.end();
	}

	const std::vector<UUID>& SelectionManager::GetSelections(SelectionContext context)
	{
		return s_SelectionData.m_Selections[context];
	}

	UUID SelectionManager::GetFirstSelected(SelectionContext context)
	{
		return s_SelectionData.m_Selections[context].front();
	}

	UUID SelectionManager::GetLastSelected(SelectionContext context)
	{
		return s_SelectionData.m_Selections[context].back();
	}

	bool SelectionManager::IsEntityOrAncestorSelected(Entity entity)
	{
		if (IsSelected(SelectionContext::Entity, entity.GetUUID()))
			return true;
		if (entity.HasParent())
			return IsEntityOrAncestorSelected(entity.Parent());
		return false;
	}

	void SelectionManager::SetActiveScene(Ref<Scene> scene)
	{
		s_SelectionData.m_Scene = scene;
	}

	Ref<Scene> SelectionManager::GetActiveScene()
	{
		return s_SelectionData.m_Scene;
	}

}
