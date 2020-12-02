#pragma once

#include "Shark/Core/Core.h"
#include "Shark/Event/Event.h"

namespace Shark {

	class Layer
	{
	public:
		Layer( const std::string& LayerName = "Layer" )
			:
			LayerName( LayerName )
		{}
		virtual ~Layer() = default;

		virtual void OnAttach() {};
		virtual void OnDetach() {};

		virtual void OnUpdate() {};
		virtual void OnEvent( Event& e ) {};

		virtual void OnImGuiRender() {};

		inline std::string GetName() const { return LayerName; }
	protected:
		std::string LayerName;
	};

}