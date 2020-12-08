#include "skpch.h"
#include "Layer.h"
#include "LayerStack.h"

namespace Shark {

	LayerStack::~LayerStack()
	{
		for ( auto l : Layers )
			delete l;
	}

	void LayerStack::PushLayer( Layer* layer )
	{
		Layers.emplace( Layers.begin() + LayerStackIndex,layer );
		++LayerStackIndex;
		layer->OnAttach();
	}

	void LayerStack::PushOverlay( Layer* layer )
	{
		Layers.emplace_back( layer );
		layer->OnAttach();
	}

	void LayerStack::PopLayer( Layer* layer )
	{
		auto it = std::find( Layers.begin(),Layers.end(),layer );
		if ( it != Layers.end() )
		{
			Layers.erase( it );
			--LayerStackIndex;
			layer->OnDetach();
		}
	}

	void LayerStack::PopOverlay( Layer* layer )
	{
		auto it = std::find( Layers.begin(),Layers.end(),layer );
		if ( it != Layers.end() )
		{
			Layers.erase( it );
			layer->OnDetach();
		}
	}

}