#pragma once

#include "Shark/Core/Base.h"
#include "Shark/String/TokenStreamReader.h"
#include "Shark/Render/ShaderCompiler/Common.h"

#include <nvrhi/nvrhi.h>
#include <set>

namespace Shark {

	struct CompilerInstruction
	{
		enum class Type
		{
			None = 0,
			Combine,
			Bind,
			Layout
		};

		Type Instruction = Type::None;
		std::vector<std::string> Arguments;
		uint64_t SourceID;
	};

	struct IncludeData
	{
		std::filesystem::path OriginalPath;
		std::filesystem::path Filepath;
		uint64_t ShaderID = 0;
		bool IsRelative = false;
		bool IsValid = false;

		uint64_t HashCode = 0;
		uint32_t Depth = 0;

		std::string Source;
		std::vector<uint32_t> Includes;
	};

	class ShaderPreprocessor
	{
	public:
		ShaderPreprocessor();
		bool PreprocessFile(const std::filesystem::path& filepath);

		uint64_t ShaderID;
		std::string Source;
		std::set<nvrhi::ShaderType> Stages;
		std::vector<CompilerInstruction> CompilerInstructions;
		std::vector<IncludeData> Includes;

		std::vector<std::string> Errors;

	private:
		bool InsertStageDeviders(String::ITokenStreamReader& stream, std::string& source);
		bool ParseCombine(String::ITokenStreamReader& stream);
		bool ParseBind(String::ITokenStreamReader& stream);
		bool ParseLayout(String::ITokenStreamReader& stream);
		bool ParseInclude(String::ITokenStreamReader& stream);

	private:
		std::filesystem::path m_ActiveDirectory;
		uint64_t m_ActiveShaderID = 0;
		uint32_t m_ParentIncludeIndex = 0;

		uint32_t m_Depth = 0;
		std::set<uint64_t> m_IncludedHeadersCache;
		bool m_PreprocessIncludes = true;
	};

}
