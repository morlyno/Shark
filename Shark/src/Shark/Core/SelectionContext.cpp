#include "skpch.h"
#include "SelectionContext.h"

#include "Shark/Scene/Scene.h"
#include "Shark/Scene/Entity.h"

#include "Shark/Debug/Profiler.h"

namespace Shark {

	struct SelectionContextData
	{
		Ref<Scene> m_Scene;
		std::vector<Entity> m_Selected;
	};

	static SelectionContextData s_SelectionData;

	bool SelectionContext::AnySelected()
	{
		return s_SelectionData.m_Selected.size();
	}

	bool SelectionContext::IsSingleSelection()
	{
		return s_SelectionData.m_Selected.size() == 1;
	}

	bool SelectionContext::IsMultiSelection()
	{
		return s_SelectionData.m_Selected.size() > 1;
	}

	bool SelectionContext::IsSelected(Entity entity)
	{
		SK_PERF_SCOPED("SelectionContext::IsSelected");
		return std::find(s_SelectionData.m_Selected.begin(), s_SelectionData.m_Selected.end(), entity) != s_SelectionData.m_Selected.end();
	}

	const std::vector<Entity>& SelectionContext::GetSelected()
	{
		return s_SelectionData.m_Selected;
	}


	const Entity SelectionContext::GetFirstSelected()
	{
		SK_CORE_VERIFY(AnySelected());
		return s_SelectionData.m_Selected.front();
	}

	void SelectionContext::Clear()
	{
		s_SelectionData.m_Selected.clear();
	}

	void SelectionContext::Select(Entity entity)
	{
		s_SelectionData.m_Selected.push_back(entity);
	}

	void SelectionContext::Select(std::span<Entity> entities)
	{
		s_SelectionData.m_Selected.insert(s_SelectionData.m_Selected.end(), entities.begin(), entities.end());
	}

	void SelectionContext::Unselect(Entity entity)
	{
		std::erase(s_SelectionData.m_Selected, entity);
	}

	void SelectionContext::ClearAndSelect(Entity entity)
	{
		Clear();
		Select(entity);
	}

	void SelectionContext::ClearAndSelect(std::span<Entity> entities)
	{
		Clear();
		Select(entities);
	}

	void SelectionContext::SetActiveScene(Ref<Scene> scene)
	{
		s_SelectionData.m_Scene = scene;
	}

	Ref<Scene> SelectionContext::GetActiveScene()
	{
		return s_SelectionData.m_Scene;
	}

}
