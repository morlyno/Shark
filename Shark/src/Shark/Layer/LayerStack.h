#pragma once

#include "Shark/Core/Core.h"
#include "Layer.h"

namespace Shark {

	class LayerStack
	{
		using Iterator = std::vector<Layer*>::iterator;
	public:
		LayerStack() = default;
		~LayerStack();

		void PushLayer( Layer* layer );
		void PushOverlay( Layer* layer );
		void PopLayer( Layer* layer );
		void PopOverlay( Layer* layer );

		Iterator begin() { return Layers.begin(); }
		Iterator end() { return Layers.end(); }
	private:
		std::vector<Layer*> Layers;
		unsigned int LayerStackIndex = 0;
	};

}