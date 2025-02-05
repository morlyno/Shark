#pragma once

#include "Shark/Core/Base.h"
#include "Layer.h"

namespace Shark {

	template<typename TReturn, typename... TArgs>
	using LayerMethod = TReturn (Layer::*)(TArgs...);

	class LayerStack
	{
	public:
		LayerStack() = default;
		~LayerStack();

		void Clear();

		void Invoke(LayerMethod<void> function);
		void Invoke(LayerMethod<void, TimeStep> function, TimeStep timeStep);

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* layer);
		void PopLayer(Layer* layer);
		void PopOverlay(Layer* layer);

	public:
		auto begin() { return m_Layers.begin(); }
		auto end() { return m_Layers.end(); }
		auto rbegin() { return m_Layers.rbegin(); }
		auto rend() { return m_Layers.rend(); }
	private:
		std::vector<Layer*> m_Layers;
		uint32_t m_LayerStackIndex = 0;
	};

}