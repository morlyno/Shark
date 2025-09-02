#pragma once

#include "Shark/Core/Base.h"
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
		nvrhi::ShaderType Stage;

		auto operator<=>(const ShaderCacheKey& other) const = default;
	};

	enum class ShaderCacheOption : uint8_t
	{
		Ignore, True, False
	};

	struct ShaderCacheEntry
	{
		uint64_t HashCode = 0;
		std::filesystem::path SourcePath;
		ShaderCacheOption ForceCompile = ShaderCacheOption::Ignore;
		ShaderCacheOption GenerateDebugInfo = ShaderCacheOption::Ignore;
	};

	class ShaderCache
	{
	public:
		ShaderCache() = default;

		void SaveRegistry();
		void LoadRegistry();

		void UpdateOptions(const ShaderSourceInfo& info, CompilerOptions& options);
		ShaderCacheEntry& GetEntry(const ShaderSourceInfo& info);

		ShaderCacheState GetCacheState(const ShaderSourceInfo& info);
		bool LoadBinary(const ShaderSourceInfo& info, std::vector<uint32_t>& outBinary, Buffer* outD3D11Binary = nullptr);
		bool LoadSPIRV(const ShaderSourceInfo& info, std::vector<uint32_t>& outBinary);
		bool LoadD3D11(const ShaderSourceInfo& info, Buffer& outBinary);
		bool LoadReflection(uint64_t shaderID, ShaderReflectionData& reflectionData);

		void CacheStage(const ShaderSourceInfo& info, std::span<const uint32_t> spirvBinary, const Buffer d3d11Binary = {});
		void CacheReflection(uint64_t shaderID, const ShaderReflectionData& reflectionData);

		ShaderCacheState GetD3D11CacheSyncState(const ShaderSourceInfo& info);
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
			Shark::Hash::AppendFNV(seed, (uint64_t)key.Stage);
			return seed;
		}
	};

}
