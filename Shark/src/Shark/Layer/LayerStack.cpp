#include "skpch.h"
#include "Layer.h"
#include "LayerStack.h"

namespace Shark {

	LayerStack::~LayerStack()
	{
		for ( auto l : Layers )
			if ( l )
				delete l;
	}

	void LayerStack::AddLayer( Layer* layer )
	{
		Layers.emplace( Layers.begin() + LayerStackIndex,layer );
		++LayerStackIndex;
	}

	void LayerStack::AddOverlay( Layer* layer )
	{
		Layers.emplace_back( layer );
	}

	void LayerStack::RemoveLayer( Layer* layer )
	{
		auto it = std::find( Layers.begin(),Layers.end(),layer );
		if ( it != Layers.end() )
		{
			Layers.erase( it );
			--LayerStackIndex;
		}
	}

	void LayerStack::RemoveOverlay( Layer* layer )
	{
		auto it = std::find( Layers.begin(),Layers.end(),layer );
		if ( it != Layers.end() )
		{
			Layers.erase( it );
		}
	}

}