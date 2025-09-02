#pragma once

#include "Shark/Core/Base.h"
#include "Shark/File/FileSystem.h"

#include <shaderc/shaderc.hpp>

namespace Shark {

	class FileIncluder : public shaderc::CompileOptions::IncluderInterface
	{
	public:
		FileIncluder(const std::filesystem::path& baseDirectory = "Resources/Shaders")
			: m_BaseDirectory(std::filesystem::absolute(baseDirectory))
		{
		}

		virtual shaderc_include_result* GetInclude(const char* requested_source, shaderc_include_type type, const char* requesting_source, size_t include_depth) override
		{
			std::string sourcePath = (m_BaseDirectory / requested_source).string();
			SK_CORE_ASSERT(FileSystem::Exists(sourcePath));
			std::string fileContent = FileSystem::ReadString(sourcePath);

			if (fileContent.empty())
			{
				IncludeResult& includeResult = m_IncludeResults[sourcePath];
				includeResult.SourcePath = sourcePath;
				includeResult.Content = fileContent;

				includeResult.Result.source_name = nullptr;
				includeResult.Result.source_name_length = 0;
				includeResult.Result.content = "File not found";
				includeResult.Result.content_length = strlen(includeResult.Result.content);
				return &includeResult.Result;
			}

			IncludeResult& includeResult = m_IncludeResults[sourcePath];
			includeResult.SourcePath = sourcePath;
			includeResult.Content = fileContent;

			includeResult.Result.source_name = includeResult.SourcePath.c_str();
			includeResult.Result.source_name_length = includeResult.SourcePath.length();
			includeResult.Result.content = includeResult.Content.c_str();
			includeResult.Result.content_length = includeResult.Content.length();

			return &includeResult.Result;
		}


		virtual void ReleaseInclude(shaderc_include_result* data) override
		{
		}

	private:
		std::filesystem::path m_BaseDirectory;

		struct IncludeResult
		{
			std::string SourcePath;
			std::string Content;
			shaderc_include_result Result;
		};

		std::unordered_map<std::string, IncludeResult> m_IncludeResults;
	};

}
