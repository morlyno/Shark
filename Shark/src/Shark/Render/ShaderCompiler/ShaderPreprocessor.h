#pragma once

#include "Shark/Core/Base.h"

#include <nvrhi/nvrhi.h>

namespace Shark {

	using PreProcessorResult = std::map<nvrhi::ShaderType, std::string>;

	class HLSLPreprocessor
	{
	public:
		PreProcessorResult Preprocess(const std::string& source);
	};

	class GLSLPreprocssor
	{
	public:
		PreProcessorResult Preprocess(const std::string& source);
	};

}
