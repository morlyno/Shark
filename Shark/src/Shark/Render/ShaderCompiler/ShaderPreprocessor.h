#pragma once

#include "Shark/Core/Base.h"

#include <nvrhi/nvrhi.h>

namespace Shark {

	using PreProcessorResult = std::map<nvrhi::ShaderType, std::string>;

	class HLSLPreprocessor
	{
	public:
		bool Preprocess(const std::string& source);

		PreProcessorResult PreProcessedResult;
		std::vector<std::pair<std::string, std::string>> CombinedImageSamplers;

	private:
		bool SplitStages(const std::string& source);
		void ProcessCombineInstructions(nvrhi::ShaderType stage, const std::string& code);

	};

	class GLSLPreprocssor
	{
	public:
		PreProcessorResult Preprocess(const std::string& source);
	};

}
