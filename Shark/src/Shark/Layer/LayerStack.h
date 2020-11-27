#pragma once

#include "Shark/Core/Core.h"
#include "Layer.h"

namespace Shark {

	class SHARK_API LayerStack
	{
		using Iterator = std::vector<Layer*>::iterator;
	public:
		LayerStack() = default;
		~LayerStack();

		void AddLayer( Layer* layer );
		void AddOverlay( Layer* layer );
		void RemoveLayer( Layer* layer );
		void RemoveOverlay( Layer* layer );

		Iterator begin() { return Layers.begin(); }
		Iterator end() { return Layers.end(); }
	private:
		std::vector<Layer*> Layers;
		unsigned int LayerStackIndex = 0;
	};

}