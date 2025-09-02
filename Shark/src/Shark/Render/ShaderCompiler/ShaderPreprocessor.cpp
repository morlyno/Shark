#include "skpch.h"
#include "ShaderPreprocessor.h"
#include "Shark/Utils/String.h"

namespace Shark {

	namespace utils {

		static nvrhi::ShaderType GetShaderStage(const std::string& stage)
		{
			if (String::Compare(stage, "vertex", String::Case::Ignore)) return nvrhi::ShaderType::Vertex;
			if (String::Compare(stage, "vert", String::Case::Ignore)) return nvrhi::ShaderType::Vertex;

			if (String::Compare(stage, "pixel", String::Case::Ignore)) return nvrhi::ShaderType::Pixel;
			if (String::Compare(stage, "frag", String::Case::Ignore)) return nvrhi::ShaderType::Pixel;
			if (String::Compare(stage, "fragment", String::Case::Ignore)) return nvrhi::ShaderType::Pixel;

			if (String::Compare(stage, "compute", String::Case::Ignore)) return nvrhi::ShaderType::Compute;

			SK_CORE_ASSERT(false, "Unkown Shader Stage");
			return nvrhi::ShaderType::None;
		}

	}

	PreProcessorResult HLSLPreprocessor::Preprocess(const std::string& source)
	{
		PreProcessorResult result;
		const std::string_view ShaderStageToken = "#pragma stage";

		size_t offset = 0;
		while (offset != std::string::npos)
		{
			const size_t moduleBegin = source.find(ShaderStageToken, offset);
			if (moduleBegin == std::string::npos)
			{
				SK_CORE_ERROR_TAG("Renderer", "Failed to find Shader Stage!");
				return {};
			}

			const size_t moduleEnd = source.find(ShaderStageToken, offset + moduleBegin + ShaderStageToken.length());
			std::string moduleSource = source.substr(moduleBegin, moduleEnd == std::string::npos ? std::string::npos : moduleEnd - moduleBegin);

			const size_t stageBegin = moduleSource.find(ShaderStageToken);
			if (stageBegin == std::string::npos)
			{
				SK_CORE_ERROR_TAG("Renderer", "Failed to find Shader Stage!");
				return {};
			}

			const size_t stageArgBegin = moduleSource.find_first_not_of(": ", stageBegin + ShaderStageToken.length());
			const size_t stageArgEnd = moduleSource.find_first_of(" \n\r", stageArgBegin);

			std::string stageString = moduleSource.substr(stageArgBegin, stageArgEnd - stageArgBegin);
			nvrhi::ShaderType stage = utils::GetShaderStage(stageString);
			offset = moduleEnd;

			for (size_t versionBegin = moduleSource.find("#version"); versionBegin != std::string::npos; versionBegin = moduleSource.find("#version"))
			{
				const size_t versionEnd = moduleSource.find_first_of("\n\r", versionBegin);
				moduleSource.erase(versionBegin, versionEnd - versionBegin);
				SK_CORE_WARN_TAG("Renderer", "The #version Preprocessor directive is depricated for shader written in HLSL");
			}

			SK_CORE_ASSERT(stage != ShaderUtils::ShaderStage::None);
			SK_CORE_ASSERT(!moduleSource.empty());
			result[stage] = moduleSource;
		}

		return result;
	}

	PreProcessorResult GLSLPreprocssor::Preprocess(const std::string& source)
	{
		PreProcessorResult result;

		const std::string_view VersionToken = "#version";
		const std::string_view ShaderStageToken = "#pragma stage";

		size_t offset = 0;

		while (offset != std::string::npos)
		{
			const size_t moduleBegin = source.find(VersionToken, offset);
			if (moduleBegin == std::string::npos)
			{
				SK_CORE_ERROR_TAG("Renderer", "Failed to find Shader Version!");
				return {};
			}

			const size_t moduleEnd = source.find(VersionToken, offset + VersionToken.length());
			std::string moduleSource = source.substr(moduleBegin, moduleEnd == std::string::npos ? std::string::npos : moduleEnd - moduleBegin);

			const size_t stageBegin = moduleSource.find(ShaderStageToken);
			if (stageBegin == std::string::npos)
			{
				SK_CORE_ERROR_TAG("Renderer", "Failed to find Shader Stage!");
				return {};
			}

			const size_t stageArgBegin = moduleSource.find_first_not_of(": ", stageBegin + ShaderStageToken.length());
			const size_t stageArgEnd = moduleSource.find_first_of(" \n\r", stageArgBegin);
			std::string stageString = moduleSource.substr(stageArgBegin, stageArgEnd - stageArgBegin);
			nvrhi::ShaderType stage = utils::GetShaderStage(stageString);
			offset = moduleEnd;

			SK_CORE_ASSERT(stage != ShaderUtils::ShaderStage::None);
			SK_CORE_ASSERT(!moduleSource.empty());
			result[stage] = moduleSource;
		}

		return result;
	}

}
