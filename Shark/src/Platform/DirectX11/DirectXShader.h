#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/Shader.h"
#include "Shark/Render/ShaderReflection.h"
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
		void RT_Release();

		virtual bool Reload(bool forceCompile = false, bool disableOptimization = false) override;

		void SetHash(uint64_t hash) { m_Hash = hash; }
		virtual uint64_t GetHash() const override { return m_Hash; }
		virtual const std::filesystem::path& GetFilePath() const override { return m_FilePath; }
		virtual const std::string& GetName() const override { return m_Name; }

		const std::unordered_map<ShaderUtils::ShaderStage::Type, std::vector<byte>>& GetShaderBinaries() const { return m_ShaderBinary; }
		virtual const ShaderReflectionData& GetReflectionData() const override { return m_ReflectionData; }

		virtual bool HasResource(const std::string& name) const override;
		virtual bool HasMember(const std::string& name) const override;

		virtual const ShaderReflection::Resource& GetResourceInfo(const std::string& name) const override;
		virtual const ShaderReflection::Resource& GetMembersResourceInfo(const std::string& name) const override;
		virtual const ShaderReflection::MemberDeclaration& GetMemberInfo(const std::string& name) const override;

		virtual std::pair<uint32_t, uint32_t> GetResourceBinding(const std::string& name) const override;
		virtual std::tuple<uint32_t, uint32_t, uint32_t> GetMemberBinding(const std::string& name) const override;

	private:
		void LoadShader(const std::unordered_map<ShaderUtils::ShaderStage::Type, std::vector<byte>>& shaderBinary);
		void SetReflectionData(const ShaderReflectionData& reflectionData) { m_ReflectionData = reflectionData; }

	private:
		ID3D11PixelShader* m_PixelShader = nullptr;
		ID3D11VertexShader* m_VertexShader = nullptr;
		ID3D11ComputeShader* m_ComputeShader = nullptr;

		std::unordered_map<ShaderUtils::ShaderStage::Type, std::vector<byte>> m_ShaderBinary;
		ShaderReflectionData m_ReflectionData;
		std::filesystem::path m_FilePath;
		std::string m_Name;
		uint64_t m_Hash;

		friend class DirectXRenderer;
		friend class DirectXShaderCompiler;
	};

}