#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Buffer.h"
#include "Shark/Core/Hash.h"
#include "Shark/Render/ShaderReflection.h"
#include "Shark/Render/ShaderCompiler/Common.h"

#include <nvrhi/nvrhi.h>

namespace Shark {

	class ShaderCompiler;

	enum class ShaderCacheState
	{
		UpToDate, OutOfDate, Missing
	};

	struct ShaderCacheKey
	{
		uint64_t ShaderID;
		auto operator<=>(const ShaderCacheKey& other) const = default;
	};

	struct ShaderCacheEntry
	{
		std::vector<std::pair<nvrhi::ShaderType, uint64_t>> Hashes;

		std::filesystem::path SourcePath;
	};

	class ShaderCache
	{
	public:
		ShaderCache() = default;

		void SaveRegistry();
		void LoadRegistry();

		ShaderCacheEntry& GetEntry(const ShaderInfo& info);

		ShaderCacheState GetCacheState(const ShaderSourceInfo& info);
		bool LoadBinary(const ShaderSourceInfo& info, std::vector<uint32_t>& outBinary, Buffer* outD3D11Binary = nullptr);
		bool LoadSPIRV(const ShaderSourceInfo& info, std::vector<uint32_t>& outBinary);
		bool LoadD3D11(const ShaderSourceInfo& info, Buffer& outBinary);
		bool LoadReflection(uint64_t shaderID, ShaderReflection& reflectionData, std::vector<std::string>& requestedBindingSets, LayoutShareMode& outShareMode);

		void CacheStage(const ShaderSourceInfo& info, std::span<const uint32_t> spirvBinary, const Buffer d3d11Binary = {});
		void CacheReflection(uint64_t shaderID, const ShaderReflection& reflectionData, std::span<const std::string> requestedBindingSets, LayoutShareMode layoutMode);

		ShaderCacheState GetD3D11CacheSyncState(const ShaderSourceInfo& info);
	private:
		uint64_t GetHash(const ShaderSourceInfo& info) const;
		void SetHash(ShaderCacheEntry& entry, nvrhi::ShaderType stage, uint64_t hashCode);
	private:
		std::map<ShaderCacheKey, ShaderCacheEntry> m_CacheRegistry;
	};

}

namespace std {

	template<>
	struct hash<Shark::ShaderCacheKey>
	{
		uint64_t operator()(const Shark::ShaderCacheKey& key)
		{
			uint64_t seed = Shark::Hash::FNVBase;
			Shark::Hash::AppendFNV(seed, key.ShaderID);
			return seed;
		}
	};

}
