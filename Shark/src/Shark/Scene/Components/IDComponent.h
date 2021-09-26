#pragma once

#include "Shark/Core/UUID.h"

namespace Shark {

	struct IDComponent
	{
		UUID ID = UUID::Create();

		IDComponent() = default;
		IDComponent(const IDComponent&) = default;
	};

}
