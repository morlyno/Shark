#include "skpch.h"
#include "LayerStack.h"

#include "Shark/Debug/Instrumentor.h"

namespace Shark {

	LayerStack::~LayerStack()
	{
		SK_PROFILE_FUNCTION();

		for (auto l : Layers)
		{
			l->OnDetach();
			delete l;
		}
	}

	void LayerStack::PushLayer(Layer* layer)
	{
		SK_PROFILE_FUNCTION();

		Layers.emplace(Layers.begin() + LayerStackIndex, layer);
		++LayerStackIndex;
	}

	void LayerStack::PushOverlay(Layer* layer)
	{
		SK_PROFILE_FUNCTION();

		Layers.emplace_back(layer);
	}

	void LayerStack::PopLayer(Layer* layer)
	{
		SK_PROFILE_FUNCTION();

		auto it = std::find(Layers.begin(), Layers.end(), layer);
		if (it != Layers.end())
		{
			Layers.erase(it);
			--LayerStackIndex;
		}
	}

	void LayerStack::PopOverlay(Layer* layer)
	{
		SK_PROFILE_FUNCTION();

		auto it = std::find(Layers.begin(), Layers.end(), layer);
		if (it != Layers.end())
		{
			Layers.erase(it);
		}
	}

}