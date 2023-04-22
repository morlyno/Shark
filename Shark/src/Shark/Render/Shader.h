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

		virtual bool Reload(bool forceCompile = false) = 0;

		virtual const std::string& GetName() const = 0;
		virtual const std::filesystem::path& GetFilePath() const = 0;
	public:
		static Ref<Shader> Create();
	};

	class ShaderLibrary : public RefCount
	{
	public:
		ShaderLibrary() = default;
		~ShaderLibrary() { Clear(); }

		Ref<Shader> Load(const std::filesystem::path& filepath, bool forceCompile = false, bool disableOptimization = false);
		Ref<Shader> Get(const std::string& name) { return m_ShaderMap.at(name); }

		bool Exists(const std::string& name) { return m_ShaderMap.find(name) != m_ShaderMap.end(); }
		void Clear() { m_ShaderMap.clear(); }

		std::unordered_map<std::string, Ref<Shader>>::const_iterator begin() const { return m_ShaderMap.cbegin(); }
		std::unordered_map<std::string, Ref<Shader>>::const_iterator end() const { return m_ShaderMap.cend(); }
	private:
		std::unordered_map<std::string, Ref<Shader>> m_ShaderMap;

	};

}