#pragma once

#include "Shark/Core/UUID.h"

namespace Shark {

	struct IDComponent
	{
		UUID ID = UUID::Generate();

		IDComponent() = default;
		IDComponent(const IDComponent&) = default;
	};

}
