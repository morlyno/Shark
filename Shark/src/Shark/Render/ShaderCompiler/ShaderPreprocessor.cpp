#include "skpch.h"
#include "ShaderPreprocessor.h"
#include "Shark/Utils/String.h"
#include <regex>

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

	bool HLSLPreprocessor::Preprocess(const std::string& source)
	{
		if (!SplitStages(source))
			return false;

		for (const auto& [stage, code] : PreProcessedResult)
		{
			ProcessCombineInstructions(stage, code);
		}
	}

	bool HLSLPreprocessor::SplitStages(const std::string& source)
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
				return false;
			}

			const size_t moduleEnd = source.find(ShaderStageToken, offset + moduleBegin + ShaderStageToken.length());
			std::string moduleSource = source.substr(moduleBegin, moduleEnd == std::string::npos ? std::string::npos : moduleEnd - moduleBegin);

			const size_t stageBegin = moduleSource.find(ShaderStageToken);
			if (stageBegin == std::string::npos)
			{
				SK_CORE_ERROR_TAG("Renderer", "Failed to find Shader Stage!");
				return false;
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

			SK_CORE_ASSERT(stage != nvrhi::ShaderType::None);
			SK_CORE_ASSERT(!moduleSource.empty());
			result[stage] = moduleSource;
		}

		PreProcessedResult = std::move(result);
		return true;
	}

	static std::vector<std::string> TokenizeCode(const std::string& code)
	{
		static const std::regex wordRegex(R"(\w+|[:()\n\r#])");

		std::sregex_iterator rit(code.begin(), code.end(), wordRegex);
		std::sregex_iterator end;

		std::vector<std::string> result;
		for (; rit != end; rit++)
			result.push_back(rit->str());

		return result;
	}

	class Tokens
	{
		std::vector<std::string>& m_Tokens;
		size_t m_CurrentToken = 0;

	public:
		Tokens(std::vector<std::string>& tokens)
			: m_Tokens(tokens)
		{}

		size_t Count() const { return m_Tokens.size(); }
		bool Valid() const { return m_CurrentToken < m_Tokens.size(); }

		bool Next(uint32_t count = 1)
		{
			m_CurrentToken += count;
			return Valid();
		}

		bool Seek(std::string_view token)
		{
			while (Valid() && !Compare(token, 0))
				Next();

			return Valid();
		}

		std::string_view Peek(size_t offset) const
		{
			if ((m_CurrentToken + offset) >= m_Tokens.size())
				return {};
			return m_Tokens[m_CurrentToken + offset];
		}

		bool Compare(std::string_view value, size_t offset) const
		{
			return Peek(offset) == value;
		}

	};

	void HLSLPreprocessor::ProcessCombineInstructions(nvrhi::ShaderType stage, const std::string& code)
	{
		auto tempTokens = TokenizeCode(code);
		Tokens tokens(tempTokens);

		while (tokens.Seek("#") && tokens.Next())
		{
			if (!tokens.Compare("pragma", 0) || !tokens.Next())
				continue;

			if (tokens.Compare("combine", 0) && tokens.Compare("\n", 4))
			{
				auto first = tokens.Peek(2);
				auto second = tokens.Peek(3);
				CombinedImageSamplers.emplace_back(first, second);
				tokens.Next(4);
			}
		}
	}




	PreProcessorResult GLSLPreprocssor::Preprocess(const std::string& source)
	{
		//SK_NOT_IMPLEMENTED();
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

			SK_CORE_ASSERT(stage != nvrhi::ShaderType::None);
			SK_CORE_ASSERT(!moduleSource.empty());
			result[stage] = moduleSource;
		}

		return result;
	}

}
