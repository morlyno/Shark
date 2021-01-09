#pragma once

#include "Shark/Core/Base.h"
#include "Layer.h"

namespace Shark {

	class LayerStack
	{
	public:
		LayerStack() = default;
		~LayerStack();

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* layer);
		void PopLayer(Layer* layer);
		void PopOverlay(Layer* layer);

		std::vector<Layer*>::iterator begin() { return Layers.begin(); }
		std::vector<Layer*>::iterator end() { return Layers.end(); }
	private:
		std::vector<Layer*> Layers;
		unsigned int LayerStackIndex = 0;
	};

}