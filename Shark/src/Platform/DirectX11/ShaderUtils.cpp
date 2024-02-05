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
			case ShaderUtils::ShaderStage::Compute: return "Compute";
		}

		SK_CORE_ASSERT(false, "Unkonw ShaderStage");
		return "Unkown";
	}

	ShaderUtils::ShaderStage::Type StringToShaderStage(const std::string& shaderStage)
	{
		if (shaderStage == "None") return ShaderUtils::ShaderStage::None;
		if (shaderStage == "Vertex") return ShaderUtils::ShaderStage::Vertex;
		if (shaderStage == "Pixel") return ShaderUtils::ShaderStage::Pixel;
		if (shaderStage == "Compute") return ShaderUtils::ShaderStage::Compute;

		SK_CORE_ASSERT(false, "Unkown ShaderStage");
		return ShaderUtils::ShaderStage::None;
	}

}
