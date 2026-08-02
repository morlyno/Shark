#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/UUID.h"
#include "NodeGraph/Factory.h"

namespace Shark::NodeGraph::Editor {

	class AnimationFactory : public CoreFactory
	{
	private:
		using Base = CoreFactory;

	public:
		AnimationFactory();

		virtual bool InitializePin(Pin& outPin, int pinType) const override;
		virtual bool InitializePin(Pin& outPin, const std::type_info& type) const override;
		virtual bool InitializePin(Pin& outPin, const choc::value::Type& type) const override;
		virtual std::optional<int> GetPinTypeOverride(std::string_view node, std::string_view pin) const override;
	};

}