#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/Shader.h"

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
		const std::unordered_map<ShaderUtils::ShaderStage::Type, std::vector<byte>>& GetShaderBinary() const { return m_ShaderBinary; }

	public:
		static Ref<Shader> Compile(const std::filesystem::path& shaderSourcePath, bool forceCompile = false, bool disableOptimization = false);

	private:
		std::unordered_map<ShaderUtils::ShaderStage::Type, std::string> PreProcess(const std::string& source);
		std::unordered_map<ShaderUtils::ShaderStage::Type, std::string> PreProcessHLSL(const std::string& source);
		std::unordered_map<ShaderUtils::ShaderStage::Type, std::string> PreProcessGLSL(const std::string& source);

		bool CompileOrGetBinaries(ShaderUtils::ShaderStage::Flags changedModules, bool forceCompile);
		bool CompileHLSL(ShaderUtils::ShaderStage::Type stage, const std::string& hlslSourceCode, std::vector<byte>& binary) const;

	private:
		ShaderUtils::ShaderLanguage m_Language;
		DirectXShaderCompilerOptions m_Options;
		std::filesystem::path m_ShaderSourcePath;
		std::unordered_map<ShaderUtils::ShaderStage::Type, std::string> m_ShaderSource;
		std::unordered_map<ShaderUtils::ShaderStage::Type, std::vector<uint32_t>> m_SPIRVData;
		std::unordered_map<ShaderUtils::ShaderStage::Type, std::string> m_HLSLShaderSource;
		std::unordered_map<ShaderUtils::ShaderStage::Type, std::vector<byte>> m_ShaderBinary;
	};

}
