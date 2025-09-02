#include "skpch.h"
#include "ShaderCache.h"

#include "Shark/File/FileSystem.h"
#include "Shark/Serialization/YAML.h"
#include "Shark/Serialization/YAML/ShaderReflection.h"

namespace Shark {

	namespace utils {

		static std::string GetSPIRVCacheFile(const ShaderSourceInfo& info)
		{
			return fmt::format("Cache/Shaders/SPIRV/{}{}", info.ShaderID, s_ShaderTypeMappings.at(info.Stage).Extension);
		}

		static std::string GetD3D11CacheFile(const ShaderSourceInfo& info)
		{
			return fmt::format("Cache/Shaders/D3D11/{}{}", info.ShaderID, s_ShaderTypeMappings.at(info.Stage).Extension);
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
			out << YAML::Key << "ID" << YAML::Value << key.ShaderID;
			out << YAML::Key << "Stage" << YAML::Value << key.Stage;
			out << YAML::Key << "HashCode" << YAML::Value << entry.HashCode;
			out << YAML::Key << "SourcePath" << YAML::Value << entry.SourcePath;
			out << YAML::Key << "Option.ForceCompile" << YAML::Value << entry.ForceCompile;
			out << YAML::Key << "Option.GenerateDebugInfo" << YAML::Value << entry.GenerateDebugInfo;
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

		YAML::Node rootNode = YAML::Load(fileData);
		if (!rootNode)
			return;

		YAML::Node cacheNode = rootNode["ShaderCache"];

		for (auto entryNode : cacheNode)
		{
			ShaderCacheKey key;
			ShaderCacheEntry entry;

			DeserializeProperty(entryNode, "ID", key.ShaderID);
			DeserializeProperty(entryNode, "Stage", key.Stage);
			DeserializeProperty(entryNode, "HashCode", entry.HashCode);
			DeserializeProperty(entryNode, "SourcePath", entry.SourcePath);
			DeserializeProperty(entryNode, "Option.ForceCompile", entry.ForceCompile, ShaderCacheOption::Ignore);
			DeserializeProperty(entryNode, "Option.GenerateDebugInfo", entry.ForceCompile, ShaderCacheOption::Ignore);

			m_CacheRegistry[key] = entry;
		}

	}

	void ShaderCache::UpdateOptions(const ShaderSourceInfo& info, CompilerOptions& options)
	{
		if (!m_CacheRegistry.contains({ info.ShaderID, info.Stage }))
			return;

		auto& entry = m_CacheRegistry.at({ info.ShaderID, info.Stage });
		if (entry.ForceCompile != ShaderCacheOption::Ignore)
			options.Force = entry.ForceCompile == ShaderCacheOption::True;

		if (entry.GenerateDebugInfo != ShaderCacheOption::Ignore)
			options.GenerateDebugInfo = entry.GenerateDebugInfo == ShaderCacheOption::True;
	}

	ShaderCacheEntry& ShaderCache::GetEntry(const ShaderSourceInfo& info)
	{
		return m_CacheRegistry[{ info.ShaderID, info.Stage }];
	}

	ShaderCacheState ShaderCache::GetCacheState(const ShaderSourceInfo& info)
	{
		if (!m_CacheRegistry.contains({ info.ShaderID, info.Stage }) || !FileSystem::Exists(utils::GetSPIRVCacheFile(info)))
			return ShaderCacheState::Missing;

		auto& entry = m_CacheRegistry.at({ info.ShaderID, info.Stage });
		if (entry.HashCode == info.HashCode)
		{
#if SK_WITH_DX11
			auto syncState = GetD3D11CacheSyncState(info);
			if (syncState != ShaderCacheState::UpToDate)
				return ShaderCacheState::OutOfDate;
#endif
			return ShaderCacheState::UpToDate;
		}
		return ShaderCacheState::OutOfDate;
	}

	bool ShaderCache::LoadBinary(const ShaderSourceInfo& info, std::vector<uint32_t>& outBinary, Buffer* outD3D11Binary)
	{
		if (!LoadSPIRV(info, outBinary))
			return false;

		if (outD3D11Binary)
			return LoadD3D11(info, *outD3D11Binary);

		return true;
	}

	bool ShaderCache::LoadSPIRV(const ShaderSourceInfo& info, std::vector<uint32_t>& outBinary)
	{
		auto state = GetCacheState(info);
		if (state == ShaderCacheState::Missing)
			return false;

		const std::string cacheFile = utils::GetSPIRVCacheFile(info);

		Buffer binary = FileSystem::ReadBinary(cacheFile);
		if (!binary)
			return false;

		binary.CopyTo(outBinary);
		binary.Release();
	}

	bool ShaderCache::LoadD3D11(const ShaderSourceInfo& info, Buffer& outBinary)
	{
#if SK_WITH_DX11
		const std::string cacheFile = utils::GetD3D11CacheFile(info);

		if (!FileSystem::Exists(cacheFile))
			return false;

		outBinary = FileSystem::ReadBinary(cacheFile);
		return outBinary;
#else
		return true;
#endif
	}

	bool ShaderCache::LoadReflection(uint64_t shaderID, ShaderReflectionData& reflectionData)
	{
		const std::string cacheFile = fmt::format("Cache/Shaders/Reflection/{}.yaml", shaderID);
		std::string fileData = FileSystem::ReadString(cacheFile);
		if (fileData.empty())
			return false;

		YAML::Node rootNode = YAML::Load(fileData);
		if (!rootNode)
			return false;

		YAML::Node reflectionNode = rootNode["ShaderReflection"];

		DeserializeProperty(reflectionNode, "Resources", reflectionData.Resources);
		DeserializeProperty(reflectionNode, "Members", reflectionData.Members);

		if (reflectionNode["PushConstant"])
		{
			DeserializeProperty(reflectionNode, "PushConstant", reflectionData.PushConstant);
			DeserializeProperty(reflectionNode, "PushConstantMembers", reflectionData.PushConstantMembers);
		}

		DeserializeProperty(reflectionNode, "NameCache", reflectionData.NameCache);
		DeserializeProperty(reflectionNode, "MemberNameCache", reflectionData.MemberNameCache);

		return true;
	}

	void ShaderCache::CacheStage(const ShaderSourceInfo& info, std::span<const uint32_t> spirvBinary, const Buffer d3d11Binary)
	{
		const std::string cacheFile = utils::GetSPIRVCacheFile(info);
		FileSystem::WriteBinary(cacheFile, Buffer::FromArray(spirvBinary));
		m_CacheRegistry[{ info.ShaderID, info.Stage }].HashCode = info.HashCode;

#if SK_WITH_DX11
		if (d3d11Binary)
		{
			const std::string cacheFile = utils::GetD3D11CacheFile(info);
			FileSystem::WriteBinary(cacheFile, d3d11Binary);
		}
#endif
	}

	void ShaderCache::CacheReflection(uint64_t shaderID, const ShaderReflectionData& reflectionData)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "ShaderReflection" << YAML::Value;
		out << YAML::BeginMap;
		out << YAML::Key << "Resources" << YAML::Value << reflectionData.Resources;
		out << YAML::Key << "Members" << YAML::Value << reflectionData.Members;

		if (reflectionData.HasPushConstant)
		{
			out << YAML::Key << "PushConstant" << YAML::Value << reflectionData.PushConstant;
			out << YAML::Key << "PushConstantMembers" << YAML::Value << reflectionData.PushConstantMembers;
		}

		out << YAML::Key << "NameCache" << YAML::Value << reflectionData.NameCache;
		out << YAML::Key << "MemberNameCache" << YAML::Value << reflectionData.MemberNameCache;

		out << YAML::EndMap;
		out << YAML::EndMap;

		const std::string cacheFile = fmt::format("Cache/Shaders/Reflection/{}.yaml", shaderID);
		FileSystem::WriteString(cacheFile, out.c_str());
	}

#if SK_WITH_DX11

	ShaderCacheState ShaderCache::GetD3D11CacheSyncState(const ShaderSourceInfo& info)
	{
		const std::string spirvCacheFile = utils::GetSPIRVCacheFile(info);
		const std::string d3d11CacheFile = utils::GetD3D11CacheFile(info);

		if (!FileSystem::Exists(d3d11CacheFile))
			return ShaderCacheState::Missing;

		const uint64_t spirvTime = FileSystem::GetLastWriteTime(spirvCacheFile);
		const uint64_t d3d11Time = FileSystem::GetLastWriteTime(d3d11CacheFile);

		if (spirvTime == d3d11Time)
			return ShaderCacheState::UpToDate;
		return ShaderCacheState::OutOfDate;
	}

#else

	ShaderCacheState ShaderCache::GetD3D11CacheSyncState(const ShaderSourceInfo& info)
	{
		return ShaderCacheState::UpToDate;
	}

#endif

}
