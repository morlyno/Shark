#pragma once

#include "Shark/Core/UUID.h"

#include <string>

namespace Shark {

	struct ScriptComponent
	{
		std::string ScriptName;
		// Can remove Handle (same as entity uuid)
		UUID Handle;
		bool ScriptModuleFound = false;
	};

}
