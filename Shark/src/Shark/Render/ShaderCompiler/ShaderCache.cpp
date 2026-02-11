#include "skpch.h"
#include "ShaderCache.h"

#include "Shark/Core/Memory.h"
#include "Shark/File/FileSystem.h"
#include "Shark/Serialization/YAML.h"
#include "Shark/Serialization/YAML/ShaderReflection.h"
#include "Shark/Utils/std.h"

#include <magic_enum.hpp>

namespace Shark {

	namespace utils {

		static uint64_t HashFileContent(const std::filesystem::path& filepath)
		{
			std::ifstream stream(filepath);

			constexpr const uint64_t bufferSize{ 1 << 10 };
			char buffer[bufferSize];

			uint64_t hash = Hash::FNVBase;
			while (stream.good())
			{
				stream.read(buffer, bufferSize);
				Hash::AppendFNV(hash, buffer, stream.gcount());
			}

			return hash;
		}

	}

	void ShaderCache::SaveRegistry()
	{
		YAML::Emitter out;

		out << YAML::BeginMap;
		out << YAML::Key << "ShaderCache" << YAML::Value;

		out << YAML::BeginSeq;

		for (const auto& [key, entry] : m_CacheRegistry)
		{
			out << YAML::BeginMap;
			out << YAML::Key << "ID" << YAML::Value << entry.Info.ShaderID;
			out << YAML::Key << "SourcePath" << YAML::Value << entry.Info.SourcePath;
			out << YAML::Key << "Hash" << YAML::Value << entry.FileHash;

			out << YAML::Key << "Stages" << YAML::Value;
			out << YAML::BeginMap;
			for (const auto& stage : entry.Stages)
				out << YAML::Key << stage.Stage << YAML::Value << stage.HashCode;
			out << YAML::EndMap;

			out << YAML::Key << "Includes" << YAML::Value;
			out << YAML::BeginSeq;
			for (const auto& include : entry.Includes)
			{
				out << YAML::BeginMap;
				out << YAML::Key << "Info.ID" << include.Info.ShaderID;
				out << YAML::Key << "Info.Path" << include.Info.SourcePath;
				out << YAML::Key << "Hash" << include.HashCode;
				out << YAML::EndMap;
			}
			out << YAML::EndSeq;

			out << YAML::EndMap;
		}

		out << YAML::EndSeq;
		out << YAML::EndMap;

		FileSystem::WriteString("Cache/Shaders/CacheRegistry.yaml", out.c_str());
	}

	void ShaderCache::LoadRegistry()
	{
		auto fileData = FileSystem::ReadString("Cache/Shaders/CacheRegistry.yaml");
		if (fileData.empty())
			return;

		auto rootNode = YAML::Load(fileData);
		if (!rootNode)
			return;

		auto cacheNode = rootNode["ShaderCache"];
		for (auto entryNode : cacheNode)
		{
			ShaderCacheEntry entry;

			bool anyFailed = false;
			anyFailed |= !DeserializeProperty(entryNode, "ID", entry.Info.ShaderID);
			anyFailed |= !DeserializeProperty(entryNode, "SourcePath", entry.Info.SourcePath);
			anyFailed |= !DeserializeProperty(entryNode, "Hash", entry.FileHash);

			if (anyFailed)
				continue;

			for (auto stage : entryNode["Stages"])
			{
				StageInfo info;
				anyFailed |= !YAML::DeserializeProperty(stage.first, info.Stage);
				anyFailed |= !YAML::DeserializeProperty(stage.second, info.HashCode);

				if (anyFailed)
					continue;

				entry.Stages.push_back(info);
			}

			m_CacheRegistry[entry.Info.ShaderID] = entry;
		}

	}

	bool ShaderCache::ShaderUpToDate(const ShaderInfo& info) const
	{
		if (!m_CacheRegistry.contains(info.ShaderID))
			return false;

		const auto& entry = m_CacheRegistry.at(info.ShaderID);

		if (entry.FileHash != utils::HashFileContent(info.SourcePath))
			return false;

		for (const auto& include : entry.Includes)
			if (include.HashCode != utils::HashFileContent(include.Info.SourcePath))
				return false;

		return true;
	}

	CacheStatus ShaderCache::GetCacheStatus(const ShaderInfo& info, const StageInfo& stageInfo) const
	{
		if (!m_CacheRegistry.contains(info.ShaderID))
			return CacheStatus::Missing;

		const std::string cacheFile = fmt::format("Cache/Shaders/spirv/{}.{}", info.ShaderID, s_ShaderTypeMappings.at(stageInfo.Stage).Extension);
		if (!FileSystem::Exists(cacheFile))
			CacheStatus::Missing;

		const auto& entry = m_CacheRegistry.at(info.ShaderID);
		return entry.FileHash == stageInfo.HashCode ? CacheStatus::OK : CacheStatus::OutOfDate;
	}

	CacheStatus ShaderCache::GetCacheStatus(const ShaderInfo& info, nvrhi::ShaderType stage, nvrhi::GraphicsAPI platform) const
	{
		const std::string spirvCacheFile = fmt::format("Cache/Shaders/spirv/{}.{}", info.ShaderID, s_ShaderTypeMappings.at(stage).Extension);
		const std::string d3d11CacheFile = fmt::format("Cache/Shaders/{}/{}.{}", magic_enum::enum_name(platform), info.ShaderID, s_ShaderTypeMappings.at(stage).Extension);

		if (!FileSystem::Exists(d3d11CacheFile))
			return CacheStatus::Missing;

		if (!FileSystem::Exists(spirvCacheFile))
			return CacheStatus::OutOfDate;

		const uint64_t spirvTime = FileSystem::GetLastWriteTime(spirvCacheFile);
		const uint64_t d3d11Time = FileSystem::GetLastWriteTime(d3d11CacheFile);

		if (d3d11Time < spirvTime)
			return CacheStatus::OutOfDate;
		return CacheStatus::OK;
	}

	bool ShaderCache::LoadStageInfo(const ShaderInfo& info, std::vector<StageInfo>& outStageInfo) const
	{
		if (!m_CacheRegistry.contains(info.ShaderID))
			return false;

		const auto& entry = m_CacheRegistry.at(info.ShaderID);
		outStageInfo = entry.Stages;
		return true;
	}

	bool ShaderCache::LoadSpirv(const ShaderInfo& info, nvrhi::ShaderType stage, std::vector<uint32_t>& outBinary) const
	{
		const std::string cacheFile = fmt::format("Cache/Shaders/spirv/{}.{}", info.ShaderID, s_ShaderTypeMappings.at(stage).Extension);

		ScopedBuffer binary = FileSystem::ReadBinary(cacheFile);
		if (!binary)
			return false;

		Memory::Write(outBinary, binary);
		return !outBinary.empty();
	}

	bool ShaderCache::LoadBinary(const ShaderInfo& info, nvrhi::ShaderType stage, nvrhi::GraphicsAPI platform, Buffer& outBinary) const
	{
		const std::string cacheFile = fmt::format("Cache/Shaders/{}/{}.{}", magic_enum::enum_name(platform), info.ShaderID, s_ShaderTypeMappings.at(stage).Extension);

		outBinary = FileSystem::ReadBinary(cacheFile);
		return outBinary;
	}

	bool ShaderCache::LoadReflection(const ShaderInfo& info, ShaderReflection& outReflection, std::vector<std::string>& outRequestedBindingSets, LayoutShareMode& outShareMode) const
	{
		const std::string cacheFile = fmt::format("Cache/Shaders/Reflection/{}.yaml", info.ShaderID);
		const std::string fileData = FileSystem::ReadString(cacheFile);
		if (fileData.empty())
			return false;

		YAML::Node rootNode = YAML::Load(fileData);
		if (!rootNode)
			return false;

		YAML::Node reflectionNode = rootNode["ShaderReflection"];

		bool anyFailed = false;
		anyFailed |= !DeserializeProperty(reflectionNode, "BindingLayouts", outReflection.BindingLayouts);
		anyFailed |= !DeserializeProperty(reflectionNode, "PushConstant", outReflection.PushConstant);
		anyFailed |= !DeserializeProperty(reflectionNode, "RequestedBindingSets", outRequestedBindingSets);
		anyFailed |= !DeserializeProperty(reflectionNode, "LayoutShareMode", outShareMode);
		return !anyFailed;
	}

	void ShaderCache::SaveShaderInfo(const ShaderInfo& info, std::span<const StageInfo> stages, std::span<const std::filesystem::path> includes)
	{
		auto& entry = m_CacheRegistry[info.ShaderID];
		entry.Info = info;
		entry.FileHash = utils::HashFileContent(info.SourcePath);
		entry.Stages = { stages.begin(), stages.end() };

		entry.Includes = includes | std::views::transform([](const auto& path)
		{
			return ShaderInclude{
				.Info = {
					.ShaderID = Hash::GenerateFNV(path.generic_string()),
					.SourcePath = path
				},
				.HashCode = utils::HashFileContent(path)
			};
		}) | std::ranges::to<std::vector<ShaderInclude>>();
	}

	void ShaderCache::SaveReflection(const ShaderInfo& info, const ShaderReflection& relfection, std::span<const std::string> requestedBindingSets, LayoutShareMode layoutMode)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "ShaderReflection" << YAML::Value;
		out << YAML::BeginMap;
		out << YAML::Key << "BindingLayouts" << YAML::Value << relfection.BindingLayouts;
		out << YAML::Key << "PushConstant" << YAML::Value << relfection.PushConstant;
		out << YAML::Key << "RequestedBindingSets" << YAML::Value << requestedBindingSets;
		out << YAML::Key << "LayoutShareMode" << YAML::Value << layoutMode;
		out << YAML::EndMap;
		out << YAML::EndMap;

		const std::string cacheFile = fmt::format("Cache/Shaders/Reflection/{}.yaml", info.ShaderID);
		FileSystem::WriteString(cacheFile, out.c_str());
	}

	void ShaderCache::SaveSpirv(const ShaderInfo& info, nvrhi::ShaderType stage, std::span<const uint32_t> binary)
	{
		const std::string cacheFile = fmt::format("Cache/Shaders/spirv/{}.{}", info.ShaderID, s_ShaderTypeMappings.at(stage).Extension);

		FileSystem::WriteBinary(cacheFile, Buffer::FromArray(binary));
	}

	void ShaderCache::SaveBinary(const ShaderInfo& info, nvrhi::ShaderType stage, nvrhi::GraphicsAPI platform, const Buffer binary)
	{
		const std::string cacheFile = fmt::format("Cache/Shaders/{}/{}.{}", magic_enum::enum_name(platform), info.ShaderID, s_ShaderTypeMappings.at(stage).Extension);

		FileSystem::WriteBinary(cacheFile, binary);
	}

}
