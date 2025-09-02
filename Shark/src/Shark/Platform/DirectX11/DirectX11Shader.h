#pragma once

#include "Shark/Render/Shader.h"
#include "Shark/Render/ShaderCompiler/Common.h"

namespace Shark {

	class DirectX11Shader : public Shader
	{
	public:
		DirectX11Shader();
		DirectX11Shader(Ref<ShaderCompiler> compiler);
		~DirectX11Shader();

		virtual bool Reload(bool forceCompile = false, bool disableOptimization = false) override;

		virtual nvrhi::ShaderHandle GetHandle(nvrhi::ShaderType stage) const override;

		virtual uint64_t GetHash() const override { return m_Info.ShaderID; }
		virtual const std::string& GetName() const override { return m_Name; }
		virtual const std::filesystem::path& GetFilePath() const override;

		virtual bool HasResource(const std::string& name) const override;
		virtual bool HasPushConstant() const override;
		virtual bool HasMember(const std::string& name) const override;

		virtual const ShaderReflection::Resource& GetResourceInfo(const std::string& name) const override;
		virtual const ShaderReflection::Resource& GetPushConstantInfo() const override;
		virtual const ShaderReflection::Resource& GetMembersResourceInfo(const std::string& name) const override;
		virtual const ShaderReflection::MemberDeclaration& GetMemberInfo(const std::string& name) const override;

		virtual const std::string& GetResourceName(uint32_t set, uint32_t binding) const override;
		virtual const ShaderReflection::Resource& GetResourceInfo(uint32_t set, uint32_t binding) const override;

		virtual std::pair<uint32_t, uint32_t> GetResourceBinding(const std::string& name) const override;
		virtual std::tuple<uint32_t, uint32_t, uint32_t> GetMemberBinding(const std::string& name) const override;

		virtual const ShaderReflectionData& GetReflectionData() const override { return m_ReflectionData; }

	private:
		void InitializeFromCompiler();

	private:
		Ref<ShaderCompiler> m_Compiler;

		ShaderInfo m_Info;
		std::string m_Name;

		ShaderReflectionData m_ReflectionData;
		std::map<nvrhi::ShaderType, nvrhi::ShaderHandle> m_ShaderHandles;
	};

}
