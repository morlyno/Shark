#include "skpch.h"
#include "ShaderUtils.h"

#include "Shark/Math/Math.h"

namespace Shark::ShaderUtils {

}

namespace Shark {

	std::string ToString(ShaderUtils::ShaderStage::Type stage)
	{
		SK_CORE_ASSERT(Math::SingleBitSet((uint16_t)stage));
		switch (stage)
		{
			case ShaderUtils::ShaderStage::None: return "None";
			case ShaderUtils::ShaderStage::Vertex: return "Vertex";
			case ShaderUtils::ShaderStage::Pixel: return "Pixel";
		}

		SK_CORE_ASSERT(false, "Unkonw ShaderStage");
		return "Unkown";
	}

}
