#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/ShaderReflection.h"

#include <nvrhi/nvrhi.h>

namespace Shark {

	class ShaderCompiler;

	class Shader : public RefCount
	{
	public:
		static Ref<Shader> Create();
		static Ref<Shader> Create(Ref<ShaderCompiler> compiler);

		virtual bool Reload(bool forceCompile = false, bool disableOptimization = false) = 0;

		virtual nvrhi::ShaderHandle GetHandle(nvrhi::ShaderType stage) const = 0;

		virtual uint64_t GetHash() const = 0;
		virtual const std::string& GetName() const = 0;
		virtual const std::filesystem::path& GetFilePath() const = 0;

		virtual bool HasResource(const std::string& name) const = 0;
		virtual bool HasPushConstant() const = 0;
		virtual bool HasMember(const std::string& name) const = 0;

		virtual const ShaderReflection::Resource& GetResourceInfo(const std::string& name) const = 0;
		virtual const ShaderReflection::Resource& GetPushConstantInfo() const = 0;
		virtual const ShaderReflection::Resource& GetMembersResourceInfo(const std::string& name) const = 0;
		virtual const ShaderReflection::MemberDeclaration& GetMemberInfo(const std::string& name) const = 0;

		virtual const std::string& GetResourceName(uint32_t set, uint32_t binding) const = 0;
		virtual const ShaderReflection::Resource& GetResourceInfo(uint32_t set, uint32_t binding) const = 0;

		virtual std::pair<uint32_t, uint32_t> GetResourceBinding(const std::string& name) const = 0;
		virtual std::tuple<uint32_t, uint32_t, uint32_t> GetMemberBinding(const std::string& name) const = 0;

	public:
		virtual const ShaderReflectionData& GetReflectionData() const = 0;

	};

	class ShaderLibrary : public RefCount
	{
	public:
		using ShadersMap = std::unordered_map<std::string, Ref<Shader>>;

	public:
		ShaderLibrary() = default;
		~ShaderLibrary() { Clear(); }

		Ref<Shader> Load(const std::filesystem::path& filepath, bool forceCompile = false, bool disableOptimization = false);
		Ref<Shader> Get(const std::string& name);
		Ref<Shader> TryGet(const std::string& name);

		bool Exists(const std::string& name) { return m_ShaderMap.contains(name); }
		void Clear() { m_ShaderMap.clear(); }

		ShadersMap& GetShadersMap() { return m_ShaderMap; }
		const ShadersMap& GetShadersMap() const { return m_ShaderMap; }

		ShadersMap::const_iterator begin() const { return m_ShaderMap.cbegin(); }
		ShadersMap::const_iterator end() const { return m_ShaderMap.cend(); }
	private:
		ShadersMap m_ShaderMap;

	};

}