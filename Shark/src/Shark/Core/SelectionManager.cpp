#include "skpch.h"
#include "SelectionManager.h"

#include "Shark/Scene/Scene.h"
#include "Shark/Scene/Entity.h"

namespace Shark {

	struct SelectionContextData
	{
		Ref<Scene> m_Scene;
		std::map<UUID, std::vector<UUID>> m_Selections;
	};

	static SelectionContextData s_SelectionData;

	void SelectionManager::DeselectAll()
	{
		for (auto& [contextID, selections] : s_SelectionData.m_Selections)
			selections.clear();
	}

	void SelectionManager::DeselectAll(UUID contextID)
	{
		auto& selections = s_SelectionData.m_Selections[contextID];
		selections.clear();
	}

	void SelectionManager::Select(UUID contextID, UUID id)
	{
		auto& selections = s_SelectionData.m_Selections[contextID];
		selections.push_back(id);
	}

	void SelectionManager::Select(UUID contextID, std::span<UUID> entities)
	{
		auto& selections = s_SelectionData.m_Selections[contextID];
		selections.insert(selections.end(), entities.begin(), entities.end());
	}

	void SelectionManager::Unselect(UUID contextID, UUID id)
	{
		auto& selections = s_SelectionData.m_Selections[contextID];
		std::erase(selections, id);
	}

	void SelectionManager::Toggle(UUID contextID, UUID id, bool select)
	{
		if (select)
			Select(contextID, id);
		else
			Unselect(contextID, id);
	}

	bool SelectionManager::AnySelected(UUID contextID)
	{
		return !s_SelectionData.m_Selections[contextID].empty();
	}

	bool SelectionManager::IsSelected(UUID contextID, UUID id)
	{
		const auto& selections = s_SelectionData.m_Selections[contextID];
		return std::find(selections.begin(), selections.end(), id) != selections.end();
	}

	const std::vector<UUID>& SelectionManager::GetSelections(UUID contextID)
	{
		return s_SelectionData.m_Selections[contextID];
	}

	UUID SelectionManager::GetFirstSelected(UUID contextID)
	{
		return s_SelectionData.m_Selections[contextID].front();
	}

	UUID SelectionManager::GetLastSelected(UUID contextID)
	{
		return s_SelectionData.m_Selections[contextID].back();
	}

	bool SelectionManager::IsEntityOrAncestorSelected(UUID contextID, Entity entity)
	{
		if (IsSelected(contextID, entity.GetUUID()))
			return true;
		if (entity.HasParent())
			return IsEntityOrAncestorSelected(contextID, entity.Parent());
		return false;
	}

}
