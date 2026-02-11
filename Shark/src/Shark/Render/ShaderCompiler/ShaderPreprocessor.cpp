#include "skpch.h"
#include "Shark/Render/ShaderCompiler/ShaderPreprocessor.h"

#include "Shark/File/FileSystem.h"
#include "Shark/String/RegexStream.h"
#include "Shark/Utils/String.h"

namespace Shark {

	namespace utils {

		static nvrhi::ShaderType GetShaderStage(std::string_view stage)
		{
			if (String::Compare(stage, "vertex", String::Case::Ignore)) return nvrhi::ShaderType::Vertex;
			if (String::Compare(stage, "vert", String::Case::Ignore)) return nvrhi::ShaderType::Vertex;

			if (String::Compare(stage, "pixel", String::Case::Ignore)) return nvrhi::ShaderType::Pixel;
			if (String::Compare(stage, "frag", String::Case::Ignore)) return nvrhi::ShaderType::Pixel;
			if (String::Compare(stage, "fragment", String::Case::Ignore)) return nvrhi::ShaderType::Pixel;

			if (String::Compare(stage, "compute", String::Case::Ignore)) return nvrhi::ShaderType::Compute;

			SK_CORE_ASSERT(false, "Unknown Shader Stage");
			return nvrhi::ShaderType::None;
		}

		static std::string_view ShaderStageToMakro(nvrhi::ShaderType stage)
		{
			switch (stage)
			{
				case nvrhi::ShaderType::Vertex: return "__VERTEX_STAGE__";
				case nvrhi::ShaderType::Pixel: return "__PIXEL_STAGE__";
				case nvrhi::ShaderType::Compute: return "__COMPUTE_STAGE__";
			}

			SK_CORE_ASSERT(false, "Unknown ShaderType");
			return "__UNKNOWN_STAGE__";
		}

	}

	ShaderPreprocessor::ShaderPreprocessor()
	{
	}

	bool ShaderPreprocessor::PreprocessFile(const std::filesystem::path& filepath)
	{
		ScopedTimer timer(fmt::format("PreprocessFile '{}'", filepath));

		/////////////////////////////////////////////////
		//// Backup and assign context
		const auto backupDirectory = m_ActiveDirectory;
		const uint64_t backupShaderID = m_ActiveShaderID;
		m_ActiveDirectory = filepath.parent_path();
		m_ActiveShaderID = Hash::GenerateFNV(filepath.generic_string());

		/////////////////////////////////////////////////
		//// Load and tokenize source
		std::string source = FileSystem::ReadString(filepath);

		static const auto s_Regex = std::regex(R"(\w+|[:()\n#\/<>".])");
		String::RegexStreamReader tokens(s_Regex, source);

		/////////////////////////////////////////////////
		//// Separate shader stages
		InsertStageDeviders(tokens, source);

		/////////////////////////////////////////////////
		//// Parse compiler instructions and includes

		tokens.SetStreamPosition(String::StreamPosition::Start);

		while (tokens.SeekPast("#"))
		{
			std::string_view token;
			tokens.Read(token);

			if (token == "include")
			{
				ParseInclude(tokens);
				continue;
			}

			if (token != "pragma")
				continue;

			tokens.ReadCurrent(token);
			if (token == "combine")
			{
				ParseCombine(tokens);
			}
			else if (token == "bind")
			{
				ParseBind(tokens);
			}
			else if (token == "layout")
			{
				ParseLayout(tokens);
			}
		}

		/////////////////////////////////////////////////
		//// Build preprocessed shader source
		Source = std::move(source);
		ShaderID = m_ActiveShaderID;

		m_ActiveDirectory = backupDirectory;
		m_ActiveShaderID = backupShaderID;
		return true;
	}

	bool ShaderPreprocessor::InsertStageDeviders(String::ITokenStreamReader& stream, std::string& source)
	{
		std::vector<std::tuple<size_t, size_t, nvrhi::ShaderType>> deviders;

		bool insideStage = false;
		while (stream.Seek("#", "pragma", "stage"))
		{
			// #pragma stage : <stage> => #if <stage_macro> | #elif <stage_macro>
			const size_t start = stream.GetSourcePosition();

			stream.SeekUntil(":", "\n");
			if (!stream.IsStreamGood())
				return false;

			stream.Advance();

			auto stage = utils::GetShaderStage(stream.Read());
			Stages.emplace(stage);
			stream.Seek("\n");

			const size_t end = stream.GetSourcePosition();

			deviders.emplace_back(start, end, stage);
		}

		if (!deviders.empty())
		{
			source.insert(stream.GetSourcePosition(), "\n#endif");

			for (auto i = deviders.rbegin(); i != deviders.rend(); i++)
			{
				const auto& [start, end, stage] = *i;
				const bool isFirst = i == std::prev(deviders.rend());

				source.erase(start, end - start);
				source.insert(start, fmt::format("#{} {}", isFirst ? "if" : "elif", utils::ShaderStageToMakro(stage)));
			}
		}

		stream.SetSource(source, false);
		return true;
	}

	bool ShaderPreprocessor::ParseCombine(String::ITokenStreamReader& stream)
	{
		//
		// #pragma combine : <texture>, <sampler>
		//

		if (stream.Read() != "combine" || stream.Read() != ":")
		{
			SK_CORE_VERIFY(false);
			return false;
		}

		auto& instruction = CompilerInstructions.emplace_back();
		instruction.SourceID = m_ActiveShaderID;
		instruction.Instruction = CompilerInstruction::Type::Combine;
		instruction.Arguments = {
			std::string(stream.Read()),
			std::string(stream.Read())
		};

		return true;
	}

	bool ShaderPreprocessor::ParseBind(String::ITokenStreamReader& stream)
	{
		// #pragma bind : <identifier>, <space>

		if (stream.Read() != "bind" || stream.Read() != ":")
		{
			SK_CORE_VERIFY(false);
			return false;
		}

		auto& pragma = CompilerInstructions.emplace_back();
		pragma.SourceID = m_ActiveShaderID;
		pragma.Instruction = CompilerInstruction::Type::Bind;
		pragma.Arguments = {
			std::string{ stream.Read() },
		};

		return true;
	}

	bool ShaderPreprocessor::ParseLayout(String::ITokenStreamReader& stream)
	{
		// #pragma layout : <sharemode>

		if (stream.Read() != "layout" || stream.Read() != ":")
		{
			SK_CORE_VERIFY(false);
			return false;
		}

		auto& pragma = CompilerInstructions.emplace_back();
		pragma.SourceID = m_ActiveShaderID;
		pragma.Instruction = CompilerInstruction::Type::Layout;
		pragma.Arguments = {
			std::string{ stream.Read() }
		};

		return true;
	}

	bool ShaderPreprocessor::ParseInclude(String::ITokenStreamReader& stream)
	{
		using namespace String::RegexLiterals;

		stream.SeekPast(R"([<"])"_r);
		const auto pathBegin = stream.GetStreamPosition();

		stream.Seek(R"([>"])"_r);
		const auto pathEnd = stream.GetStreamPosition();

		std::string result;
		stream.SetStreamPosition(pathBegin);
		while (pathEnd != stream.GetStreamPosition())
			result.append(stream.Read());

		std::filesystem::path includePath = result;

		static const std::filesystem::path s_BasePath = "Resources/Shaders";
		bool isRelative = false;
		bool exists = false;

		auto absolutePath = m_ActiveDirectory / includePath;
		if (FileSystem::Exists(absolutePath))
		{
			isRelative = true;
			exists = true;
		}

		if (!exists)
		{
			absolutePath = s_BasePath / includePath;
			if (FileSystem::Exists(absolutePath))
			{
				isRelative = false;
				exists = true;
			}
		}

		if (!exists)
		{
			Errors.emplace_back(fmt::format("Include file '{}' doesn't exists!\nTested:\n - {}\n - {}", includePath, (m_ActiveDirectory / includePath).generic_string(), (s_BasePath / includePath).generic_string()));
			return false;
		}

		absolutePath = FileSystem::Relative(std::filesystem::canonical(absolutePath)).generic_wstring();
		const uint64_t includeShaderID = Hash::GenerateFNV(absolutePath.string());

		if (m_IncludedHeadersCache.contains(includeShaderID))
		{
			return true;
		}

		const uint32_t includeIndex = Includes.size();

		{
			auto& includeData = Includes.emplace_back();
			includeData.OriginalPath = includePath;
			includeData.Filepath = absolutePath;
			includeData.IsRelative = isRelative;
			includeData.ShaderID = includeShaderID;
			includeData.Depth = m_Depth + 1;

			m_IncludedHeadersCache.emplace(includeShaderID);
		}

		if (m_Depth > 0)
		{
			auto& parentInclude = Includes[m_ParentIncludeIndex];
			parentInclude.Includes.push_back(includeIndex);
		}

		const uint32_t backupParentIndex = std::exchange(m_ParentIncludeIndex, includeIndex);
		m_Depth++;
		if (PreprocessFile(absolutePath))
		{
			auto& includeData = Includes[includeIndex];
			includeData.IsValid = true;
			includeData.Source = std::move(Source);
			includeData.HashCode = Hash::GenerateFNV(includeData.HashCode);
		}
		m_Depth--;
		m_ParentIncludeIndex = backupParentIndex;

		return Includes[includeIndex].IsValid;
	}

}
