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
	private:
		struct CompileOptions
		{
			bool Optimize;
			bool GenerateDebugInfo;
			bool AutoCombineImageSamplers;
		};
	public:
		DirectXShaderCompiler(const std::filesystem::path& shaderSourcePath, bool disableOptimization = false);
		DirectXShaderCompiler(const std::filesystem::path& shaderSourcePath, const DirectXShaderCompilerOptions& options);

		bool Reload(bool forceCompile);

		DirectXShaderCompilerOptions& GetOptions() { return m_Options; }

		const std::filesystem::path& GetShaderSourcePath() const { return m_ShaderSourcePath; }
		const std::unordered_map<ShaderUtils::ShaderStage::Type, std::vector<byte>>& GetShaderBinary() const { return m_ShaderBinary; }

	public:
		static Ref<Shader> Compile(const std::filesystem::path& shaderSourcePath, bool forceCompile = false, bool disableOptimization = false);

	private:
		std::unordered_map<ShaderUtils::ShaderStage::Type, std::string> PreProcess(const std::string& source);
		std::unordered_map<ShaderUtils::ShaderStage::Type, std::string> PreProcessHLSL(const std::string& source);
		std::unordered_map<ShaderUtils::ShaderStage::Type, std::string> PreProcessGLSL(const std::string& source);

		std::unordered_map<ShaderUtils::ShaderStage::Type, std::string> PreProcessHLSLSource(const std::string& source);
		std::unordered_map<ShaderUtils::ShaderStage::Type, std::string> PreProcessGLSLSource(const std::string& source);

		bool CompileOrLoadBinaries(ShaderUtils::ShaderStage::Flags changedStages, bool forceCompile);
		bool CompileOrLoadBinary(ShaderUtils::ShaderStage::Type stage, ShaderUtils::ShaderStage::Flags changedStages, bool forceCompile);

		std::string Compile(ShaderUtils::ShaderStage::Type stage, std::vector<byte>& outputBinary, std::vector<uint32_t>& outputSPIRVDebug);
		std::string CompileFromSPIRV(ShaderUtils::ShaderStage::Type stage, const std::vector<uint32_t>& spirvBinary, std::vector<byte>& outputBinary);
		std::string CompileHLSL(ShaderUtils::ShaderStage::Type stage, const std::string& hlslSourceCode, std::vector<byte>& binary) const;
		std::string CrossCompileToHLSL(ShaderUtils::ShaderStage::Type stage, const std::vector<uint32_t>& spirvBinary);

		void SerializeDirectX(ShaderUtils::ShaderStage::Type stage, const std::vector<byte>& directXData);
		void TryLoadDirectX(ShaderUtils::ShaderStage::Type stage, std::vector<byte>& directXData);
		void SerializeSPIRV(ShaderUtils::ShaderStage::Type stage, const std::vector<uint32_t>& spirvData);
		void TryLoadSPIRV(ShaderUtils::ShaderStage::Type stage, std::vector<uint32_t>& outputSPIRVData);

		void TryLoadDirectXAndSPIRV(ShaderUtils::ShaderStage::Type stage, std::vector<byte>& outputDirectXData, std::vector<uint32_t>& outputSPIRVData);

		void SerializeReflectionData();
		bool ReadReflectionData();

		void ReflectAllShaderStages(const std::unordered_map<ShaderUtils::ShaderStage::Type, std::vector<uint32_t>>& spirvData);
		void ReflectShaderStage(ShaderUtils::ShaderStage::Type stage, const std::vector<uint32_t>& spirvBinary);

	private:
		ShaderUtils::ShaderLanguage m_Language;
		ShaderUtils::ShaderStage::Flags m_Stages = ShaderUtils::ShaderStage::None;
		DirectXShaderCompilerOptions m_Options;
		std::filesystem::path m_ShaderSourcePath;
		std::unordered_map<ShaderUtils::ShaderStage::Type, std::string> m_ShaderSource;
		std::unordered_map<ShaderUtils::ShaderStage::Type, std::vector<uint32_t>> m_SPIRVData;
		std::unordered_map<ShaderUtils::ShaderStage::Type, std::vector<byte>> m_ShaderBinary;
		ShaderReflectionData m_ReflectionData;

		struct Metadata
		{
			ShaderUtils::ShaderStage::Type Stage = ShaderUtils::ShaderStage::None;
			uint64_t HashCode = 0;
			// TODO(moro): std::string HLSLVersion;
		};
		std::unordered_map<ShaderUtils::ShaderStage::Type, Metadata> m_ShaderStageMetadata;

		ShaderUtils::ShaderStage::Flags m_StagesWrittenToCache = ShaderUtils::ShaderStage::None;

		friend class DirectXShaderCache;
	};

}
