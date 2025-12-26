#pragma once

#include <Shark/Render/ShaderCompiler/Common.h>
#include <span>

namespace Shark {

	struct ShaderInclude
	{
		ShaderInfo Info;
		uint64_t HashCode;
	};

	struct ShaderCacheEntry
	{
		ShaderInfo Info;
		uint64_t FileHash = 0;
		std::vector<StageInfo> Stages;
		std::vector<ShaderInclude> Includes;
	};

	enum class CacheStatus
	{
		OK, OutOfDate, Missing
	};

	class ShaderCache
	{
	public:
		void SaveRegistry();
		void LoadRegistry();

		bool ShaderUpToDate(const ShaderInfo& info) const;
		CacheStatus GetCacheStatus(const ShaderInfo& info, const StageInfo& stageInfo) const;
		CacheStatus GetCacheStatus(const ShaderInfo& info, nvrhi::ShaderType stage, nvrhi::GraphicsAPI platform) const;

		bool LoadStageInfo(const ShaderInfo& info, std::vector<StageInfo>& outStageInfo) const;
		bool LoadSpirv(const ShaderInfo& info, nvrhi::ShaderType stage, std::vector<uint32_t>& outBinary) const;
		bool LoadBinary(const ShaderInfo& info, nvrhi::ShaderType stage, nvrhi::GraphicsAPI platform, Buffer& outBinary) const;
		bool LoadReflection(const ShaderInfo& info, ShaderReflection& outReflection, std::vector<std::string>& outRequestedBindingSets, LayoutShareMode& outShareMode) const;

		void SaveShaderInfo(const ShaderInfo& info, std::span<const StageInfo> stages, std::span<const std::filesystem::path> includes);
		void SaveReflection(const ShaderInfo& info, const ShaderReflection& relfection, std::span<const std::string> requestedBindingSets, LayoutShareMode layoutMode);
		void SaveSpirv(const ShaderInfo& info, nvrhi::ShaderType stage, std::span<const uint32_t> binary);
		void SaveBinary(const ShaderInfo& info, nvrhi::ShaderType stage, nvrhi::GraphicsAPI platform, const Buffer binary);

	private:
		std::unordered_map<uint64_t, ShaderCacheEntry> m_CacheRegistry;
	};

}
