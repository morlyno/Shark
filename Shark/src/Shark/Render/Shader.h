#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/ShaderReflection.h"
#include "Shark/Render/ShaderCompiler/ShaderCompiler.h"

#include <nvrhi/nvrhi.h>
#include <utility>

namespace Shark {

	class Shader : public RefCount
	{
	public:
		static Ref<Shader> Create() { return Ref<Shader>::Create(); }
		static Ref<Shader> Create(Ref<ShaderCompiler> compiler) { return Ref<Shader>::Create(compiler); }

		bool Reload(bool forceCompile = false, bool disableOptimization = false);

		nvrhi::ShaderHandle GetHandle(nvrhi::ShaderType stage) const;

		uint64_t GetHash() const { return m_Info.ShaderID; }
		const std::string& GetName() const { return m_Name; }
		const std::filesystem::path& GetFilePath() const { return m_Info.SourcePath; }

		bool HasLayout(uint32_t set) const { return m_LayoutMapping[set] != -1; }
		nvrhi::IBindingLayout* GetBindingLayout(uint32_t set) const { return m_BindingLayouts[m_LayoutMapping[set]]; }
		const nvrhi::BindingLayoutVector& GetBindingLayouts() const { return m_BindingLayouts; }
		const ShaderReflection& GetReflectionData() const { return m_ReflectionData; }

		int MapSet(uint32_t set) { return m_LayoutMapping[set]; }

		const auto& GetRequestedBindingSets() const { return m_RequestedBindingSets; }
		LayoutShareMode GetLayoutMode() const { return m_LayoutMode; }

	public:
		Shader();
		Shader(Ref<ShaderCompiler> compiler);
		~Shader();

	private:
		void InitializeFromCompiler();
		void CreateBindingLayout();

	private:
		// #Renderer #TODO remove this
		Ref<ShaderCompiler> m_Compiler;

		ShaderInfo m_Info;
		std::string m_Name;

		ShaderReflection m_ReflectionData;
		std::map<nvrhi::ShaderType, nvrhi::ShaderHandle> m_ShaderHandles;

		std::array<int, nvrhi::c_MaxBindingLayouts> m_LayoutMapping;
		nvrhi::BindingLayoutVector m_BindingLayouts;

		std::map<uint32_t, nvrhi::BindingSetHandle> m_RequestedBindingSets;
		LayoutShareMode m_LayoutMode;
	};

	struct DefaultCompilerOptions
	{
		bool ForceCompile = false;
		bool Optimize = false;
		bool GenerateDebugInfo = false;
	};

	class ShaderLibrary : public RefCount
	{
	private:
		struct LoadArgs
		{
			std::optional<bool> ForceCompile;
			std::optional<bool> Optimize;
			std::optional<bool> GenerateDebugInfo;
		};

	public:
		using ShadersMap = std::unordered_map<std::string, Ref<Shader>>;

	public:
		ShaderLibrary() = default;
		~ShaderLibrary() { Clear(); }

		void SetCompilerOptions(const DefaultCompilerOptions& defaultOptions) { m_DefaultOptions = defaultOptions; }

		Ref<Shader> Load(const std::filesystem::path& filepath, const LoadArgs& options = {});
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
		DefaultCompilerOptions m_DefaultOptions;
	};

}