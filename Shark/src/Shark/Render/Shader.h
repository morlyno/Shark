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

		virtual VertexLayout& GetVertexLayout() = 0;

		virtual const std::filesystem::path& GetFilePath() const = 0;
		virtual const std::string& GetFileName() const = 0;

		virtual bool ReCompile() = 0;
		virtual bool RT_ReCompile() = 0;
		virtual void LogReflection() = 0;

		virtual Ref<ConstantBuffer> CreateConstantBuffer(const std::string& name) = 0;

	public:
		static Ref<Shader> Create(const std::filesystem::path& filepath);
	};

	class ShaderLibrary : public RefCount
	{
	public:
		ShaderLibrary() = default;
		~ShaderLibrary() { Clear(); }

		ShaderLibrary(const ShaderLibrary&) = delete;
		ShaderLibrary& operator=(const ShaderLibrary&) = delete;

		Ref<Shader> Load(const std::filesystem::path& filepath);
		Ref<Shader> Load(const std::filesystem::path& filepath, const std::string name);

		void Add(Ref<Shader> shader);
		void Add(Ref<Shader> shader, const std::string& name);

		Ref<Shader> Get(const std::string& name);
		Ref<Shader> TryGet(const std::string& name);

		Ref<Shader> Remove(const std::string& name);
		Ref<Shader> Remove(Ref<Shader> shader);

		bool Exists(const std::string& name);
		bool Exists(Ref<Shader> shader);

		void Clear();

		std::unordered_map<std::string, Ref<Shader>>::const_iterator begin() const { return m_Shaders.cbegin(); }
		std::unordered_map<std::string, Ref<Shader>>::const_iterator end() const { return m_Shaders.cend(); }
	private:
		std::unordered_map<std::string, Ref<Shader>> m_Shaders;
	};

}