#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/Shader.h"
#include "Platform/DirectX11/ShaderUtils.h"

#include <d3d11.h>
#include <d3d11shader.h>

namespace Shark {

	class DirectXShader : public Shader
	{
	public:
		DirectXShader();
		virtual ~DirectXShader();
		void Release();

		virtual bool Reload(bool forceCompile = false, bool disableOptimization = false) override;

		virtual const std::filesystem::path& GetFilePath() const override { return m_FilePath; }
		virtual const std::string& GetName() const override { return m_Name; }

		const std::unordered_map<ShaderUtils::ShaderStage::Type, std::vector<byte>>& GetShaderBinaries() const { return m_ShaderBinary; }
		const ShaderReflectionData& GetReflectionData() const { return m_RefelctionData; }

	private:
		void LoadShader(const std::unordered_map<ShaderUtils::ShaderStage::Type, std::vector<byte>>& shaderBinary);
		void SetReflectionData(const ShaderReflectionData& reflectionData) { m_RefelctionData = reflectionData; }

	private:
		ID3D11PixelShader* m_PixelShader = nullptr;
		ID3D11VertexShader* m_VertexShader = nullptr;
		
		std::unordered_map<ShaderUtils::ShaderStage::Type, std::vector<byte>> m_ShaderBinary;
		ShaderReflectionData m_RefelctionData;
		std::filesystem::path m_FilePath;
		std::string m_Name;

		friend class DirectXRenderer;
		friend class DirectXShaderCompiler;
	};

}