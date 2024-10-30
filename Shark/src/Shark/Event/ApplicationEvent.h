#pragma once

#include "Shark/Event/Event.h"
#include "Shark/Asset/Asset.h"

namespace Shark {

	class ApplicationClosedEvent : public EventBase<EventType::ApplicationClosed, EventCategory::Application>
	{
	};

	class AssetReloadedEvent : public EventBase<EventType::AssetReloaded, EventCategory::Application>
	{
	public:
		AssetReloadedEvent(AssetHandle asset)
			: Asset(asset) {}

		std::string ToString() const override
		{
			return fmt::format("{} {}", GetName(), Asset);
		}

	public:
		AssetHandle Asset;
	};

}