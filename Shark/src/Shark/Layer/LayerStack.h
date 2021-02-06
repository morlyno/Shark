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
		std::vector<Layer*>::reverse_iterator rbegin() { return Layers.rbegin(); }
		std::vector<Layer*>::reverse_iterator rend() { return Layers.rend(); }
	private:
		std::vector<Layer*> Layers;
		uint32_t LayerStackIndex = 0;
	};

}