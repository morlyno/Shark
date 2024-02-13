#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/VertexLayout.h"
#include "Shark/Render/ConstantBuffer.h"
#include "Shark/Render/RenderCommandBuffer.h"

namespace Shark {

	class Shader : public RefCount
	{
	public:
		virtual ~Shader() = default;

		virtual bool Reload(bool forceCompile = false, bool disableOptimization = false) = 0;

		virtual const std::string& GetName() const = 0;
		virtual const std::filesystem::path& GetFilePath() const = 0;
	public:
		static Ref<Shader> Create();
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