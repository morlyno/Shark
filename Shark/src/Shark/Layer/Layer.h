#pragma once

#include "Shark/Core.h"
#include "Shark/Event/Event.h"

namespace Shark {

	class SHARK_API Layer
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

		virtual void OnRender() {};

		inline std::string GetName() const { return LayerName; }
	protected:
		std::string LayerName;
	};

}