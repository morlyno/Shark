#include "skpch.h"
#include "Environment.h"

#include "Shark/Render/Texture.h"

namespace Shark {

	Environment::Environment(Ref<TextureCube> radianceMap, Ref<TextureCube> irradianceMap)
		: m_RadianceMap(radianceMap), m_IrradianceMap(irradianceMap)
	{
	}

}
