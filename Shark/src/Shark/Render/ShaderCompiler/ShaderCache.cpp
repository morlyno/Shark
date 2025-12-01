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
			out << YAML::Key << "HashCodes" << YAML::Value;
			out << YAML::BeginMap;
			for (const auto& code : entry.Hashes)
				out << YAML::Key << code.first << YAML::Value << code.second;
			out << YAML::EndMap;
			out << YAML::Key << "SourcePath" << YAML::Value << entry.SourcePath;
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
			DeserializeProperty(entryNode, "SourcePath", entry.SourcePath);

			for (auto codeNode : entryNode["HashCodes"])
			{
				auto key = codeNode.first.as<nvrhi::ShaderType>();
				auto hash = codeNode.second.as<uint64_t>();
				entry.Hashes.emplace_back(key, hash);
			}

			m_CacheRegistry[key] = entry;
		}

	}

	ShaderCacheEntry& ShaderCache::GetEntry(const ShaderInfo& info)
	{
		return m_CacheRegistry[{ info.ShaderID }];
	}

	ShaderCacheState ShaderCache::GetCacheState(const ShaderSourceInfo& info)
	{
		if (!m_CacheRegistry.contains({ info.ShaderID }) || !FileSystem::Exists(utils::GetSPIRVCacheFile(info)))
			return ShaderCacheState::Missing;

		uint64_t hashCode = GetHash(info);
		if (hashCode == info.HashCode)
		{
#if SK_WITH_DX11 && false
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
		return true;
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

	bool ShaderCache::LoadReflection(uint64_t shaderID, ShaderReflection& outReflectionData, std::vector<std::string>& outRequestedBindingSets, LayoutShareMode& outShareMode)
	{
		const std::string cacheFile = fmt::format("Cache/Shaders/Reflection/{}.yaml", shaderID);
		std::string fileData = FileSystem::ReadString(cacheFile);
		if (fileData.empty())
			return false;

		YAML::Node rootNode = YAML::Load(fileData);
		if (!rootNode)
			return false;

		YAML::Node reflectionNode = rootNode["ShaderReflection"];

		bool anyFailed = false;
		anyFailed |= !DeserializeProperty(reflectionNode, "BindingLayouts", outReflectionData.BindingLayouts);
		anyFailed |= !DeserializeProperty(reflectionNode, "PushConstant", outReflectionData.PushConstant);
		anyFailed |= !DeserializeProperty(reflectionNode, "RequestedBindingSets", outRequestedBindingSets);
		anyFailed |= !DeserializeProperty(reflectionNode, "LayoutShareMode", outShareMode);
		return !anyFailed;
	}

	void ShaderCache::CacheStage(const ShaderSourceInfo& info, std::span<const uint32_t> spirvBinary, const Buffer d3d11Binary)
	{
		const std::string cacheFile = utils::GetSPIRVCacheFile(info);
		FileSystem::WriteBinary(cacheFile, Buffer::FromArray(spirvBinary));
		SetHash(m_CacheRegistry[{ info.ShaderID }], info.Stage, info.HashCode);

#if SK_WITH_DX11
		if (d3d11Binary)
		{
			const std::string cacheFile = utils::GetD3D11CacheFile(info);
			FileSystem::WriteBinary(cacheFile, d3d11Binary);
		}
#endif

		SaveRegistry();
	}

	void ShaderCache::CacheReflection(uint64_t shaderID, const ShaderReflection& reflectionData, std::span<const std::string> requestedBindingSets, LayoutShareMode layoutMode)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "ShaderReflection" << YAML::Value;
		out << YAML::BeginMap;
		out << YAML::Key << "BindingLayouts" << YAML::Value << reflectionData.BindingLayouts;
		out << YAML::Key << "PushConstant" << YAML::Value << reflectionData.PushConstant;
		out << YAML::Key << "RequestedBindingSets" << YAML::Value << requestedBindingSets;
		out << YAML::Key << "LayoutShareMode" << YAML::Value << layoutMode;
		out << YAML::EndMap;
		out << YAML::EndMap;

		const std::string cacheFile = fmt::format("Cache/Shaders/Reflection/{}.yaml", shaderID);
		FileSystem::WriteString(cacheFile, out.c_str());
	}

	uint64_t ShaderCache::GetHash(const ShaderSourceInfo& info) const
	{
		if (m_CacheRegistry.contains({ info.ShaderID }))
		{
			const auto& entry = m_CacheRegistry.at({ info.ShaderID });
			const auto hash = std::ranges::find(entry.Hashes, info.Stage, &std::pair<nvrhi::ShaderType, uint64_t>::first);
			if (hash != entry.Hashes.end())
				return hash->second;
		}
		return 0;
	}

	void ShaderCache::SetHash(ShaderCacheEntry& entry, nvrhi::ShaderType stage, uint64_t hashCode)
	{
		auto hash = std::ranges::find(entry.Hashes, stage, &std::pair<nvrhi::ShaderType, uint64_t>::first);
		if (hash != entry.Hashes.end())
		{
			hash->second = hashCode;
			return;
		}

		entry.Hashes.emplace_back(stage, hashCode);
	}

#if SK_WITH_DX11

	ShaderCacheState ShaderCache::GetD3D11CacheSyncState(const ShaderSourceInfo& info)
	{
		const std::string spirvCacheFile = utils::GetSPIRVCacheFile(info);
		const std::string d3d11CacheFile = utils::GetD3D11CacheFile(info);

		if (!FileSystem::Exists(d3d11CacheFile))
			return ShaderCacheState::Missing;

		ShaderCacheState spirvState = GetCacheState(info);
		if (spirvState == ShaderCacheState::Missing)
			return ShaderCacheState::OutOfDate;

		const uint64_t spirvTime = FileSystem::GetLastWriteTime(spirvCacheFile);
		const uint64_t d3d11Time = FileSystem::GetLastWriteTime(d3d11CacheFile);

		if (d3d11Time < spirvTime)
			return ShaderCacheState::OutOfDate;
		return ShaderCacheState::UpToDate;
	}

#else

	ShaderCacheState ShaderCache::GetD3D11CacheSyncState(const ShaderSourceInfo& info)
	{
		return ShaderCacheState::UpToDate;
	}

#endif

}
