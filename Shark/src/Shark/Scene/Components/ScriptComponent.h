#pragma once

#include "Shark/Scripting/ScriptTypes.h"
#include <string>

namespace Shark {

	struct ScriptComponent
	{
		std::string ScriptName;
		bool IsExisitingScript = false;
	};

}
