#include "skpch.h"
#include "LayerStack.h"

#include "Shark/Debug/Profiler.h"

namespace Shark {

	LayerStack::~LayerStack()
	{
		for (auto l : m_Layers)
		{
			l->OnDetach();
			skdelete l;
		}
	}

	void LayerStack::Clear()
	{
		for (auto l : m_Layers)
		{
			l->OnDetach();
			skdelete l;
		}
		m_Layers.clear();
		m_LayerStackIndex = 0;
	}

	void LayerStack::PushLayer(Layer* layer)
	{
		m_Layers.emplace(m_Layers.begin() + m_LayerStackIndex, layer);
		++m_LayerStackIndex;
	}

	void LayerStack::PushOverlay(Layer* layer)
	{
		m_Layers.emplace_back(layer);
	}

	void LayerStack::PopLayer(Layer* layer)
	{
		auto it = std::find(m_Layers.begin(), m_Layers.end(), layer);
		if (it != m_Layers.end())
		{
			m_Layers.erase(it);
			--m_LayerStackIndex;
		}
	}

	void LayerStack::PopOverlay(Layer* layer)
	{
		auto it = std::find(m_Layers.begin(), m_Layers.end(), layer);
		if (it != m_Layers.end())
		{
			m_Layers.erase(it);
		}
	}

}