#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/UUID.h"

#include <vector>

namespace Shark {

	struct RelationshipComponent
	{
		UUID Parent = UUID::Invalid;
		std::vector<UUID> Children;
	};

}
