#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/Shader.h"
#include "Shark/Render/ShaderReflection.h"

#include "Platform/DirectX11/ShaderUtils.h"

#include <filesystem>

namespace Shark {

	struct DirectXShaderCompilerOptions
	{
		bool DisableOptimization = false;
		bool AutoCombineImageSamplers = false;
	};

	class DirectXShaderCompiler : public RefCount
	{
	public:
		DirectXShaderCompiler(const std::filesystem::path& shaderSourcePath, bool disableOptimization = false);
		DirectXShaderCompiler(const std::filesystem::path& shaderSourcePath, const DirectXShaderCompilerOptions& options);

		bool Reload(bool forceCompile);

		DirectXShaderCompilerOptions& GetOptions() { return m_Options; }

		const std::filesystem::path& GetShaderSourcePath() const { return m_ShaderSourcePath; }
		const std::unordered_map<ShaderUtils::ShaderStage, std::vector<byte>>& GetShaderBinary() const { return m_ShaderBinary; }
		const ShaderReflectionData& GetRefectionData() const { return m_ReflectionData; }
		uint64_t GetHash() const { return m_Hash; }

	public:
		static Ref<Shader> Compile(const std::filesystem::path& shaderSourcePath, bool forceCompile = false, bool disableOptimization = false);

	private:
		std::unordered_map<ShaderUtils::ShaderStage, std::string> PreProcess(const std::string& source);
		std::unordered_map<ShaderUtils::ShaderStage, std::string> PreProcessHLSL(const std::string& source);
		std::unordered_map<ShaderUtils::ShaderStage, std::string> PreProcessGLSL(const std::string& source);

		std::unordered_map<ShaderUtils::ShaderStage, std::string> PreProcessHLSLSource(const std::string& source);
		std::unordered_map<ShaderUtils::ShaderStage, std::string> PreProcessGLSLSource(const std::string& source);

		std::string GetMetadata(const std::string& source);

		bool CompileOrLoadBinaries(ShaderUtils::ShaderStage changedStages, bool forceCompile);
		bool CompileOrLoadBinary(ShaderUtils::ShaderStage stage, ShaderUtils::ShaderStage changedStages, bool forceCompile);

		std::string Compile(ShaderUtils::ShaderStage stage, std::vector<byte>& outputBinary, std::vector<uint32_t>& outputSPIRVDebug);
		std::string CompileHLSLToSPIRV(ShaderUtils::ShaderStage stage, std::vector<uint32_t>& outputSPIRVDebug);
		std::string CompileGLSLToSPIRV(ShaderUtils::ShaderStage stage, std::vector<uint32_t>& outputSPIRVDebug);
		std::string CompileFromSPIRV(ShaderUtils::ShaderStage stage, const std::vector<uint32_t>& spirvBinary, std::vector<byte>& outputBinary);
		std::string CompileHLSL(ShaderUtils::ShaderStage stage, const std::string& hlslSourceCode, std::vector<byte>& binary) const;
		std::string CrossCompileToHLSL(ShaderUtils::ShaderStage stage, const std::vector<uint32_t>& spirvBinary);

		void SerializeDirectX(ShaderUtils::ShaderStage stage, const std::vector<byte>& directXData);
		void TryLoadDirectX(ShaderUtils::ShaderStage stage, std::vector<byte>& directXData);
		void SerializeSPIRV(ShaderUtils::ShaderStage stage, const std::vector<uint32_t>& spirvData);
		void TryLoadSPIRV(ShaderUtils::ShaderStage stage, std::vector<uint32_t>& outputSPIRVData);

		void TryLoadDirectXAndSPIRV(ShaderUtils::ShaderStage stage, std::vector<byte>& outputDirectXData, std::vector<uint32_t>& outputSPIRVData);

		void SerializeReflectionData();
		bool ReadReflectionData();

		void ReflectAllShaderStages(const std::unordered_map<ShaderUtils::ShaderStage, std::vector<uint32_t>>& spirvData);
		void ReflectShaderStage(ShaderUtils::ShaderStage stage, const std::vector<uint32_t>& spirvBinary);

	private:
		ShaderUtils::ShaderLanguage m_Language;
		ShaderUtils::ShaderStage m_Stages = ShaderUtils::ShaderStage::None;
		DirectXShaderCompilerOptions m_Options;
		std::filesystem::path m_ShaderSourcePath;
		uint64_t m_Hash = 0;

		std::unordered_map<ShaderUtils::ShaderStage, std::string> m_ShaderSource;
		std::unordered_map<ShaderUtils::ShaderStage, std::vector<uint32_t>> m_SPIRVData;
		std::unordered_map<ShaderUtils::ShaderStage, std::vector<byte>> m_ShaderBinary;
		ShaderReflectionData m_ReflectionData;

		ShaderUtils::ShaderStage m_CompiledStages = ShaderUtils::ShaderStage::None;

		struct Metadata
		{
			ShaderUtils::ShaderStage Stage = ShaderUtils::ShaderStage::None;
			uint64_t HashCode = 0;
			std::filesystem::path CacheFile;
		};
		std::unordered_map<ShaderUtils::ShaderStage, Metadata> m_ShaderStageMetadata;

		ShaderUtils::ShaderStage m_StagesWrittenToCache = ShaderUtils::ShaderStage::None;

		friend class DirectXShaderCache;
	};

}
