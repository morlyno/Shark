#pragma once

#include "Shark/Core/UUID.h"

#include <string>

namespace Shark {

	struct ScriptComponent
	{
		std::string ScriptName;
		bool ScriptModuleFound = false;
		bool HasRuntime = false;
	};

}
