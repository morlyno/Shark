#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/VertexLayout.h"
#include "Shark/Render/ConstantBuffer.h"

namespace Shark {

	class Shaders : public RefCount
	{
	public:
		virtual ~Shaders() = default;

		virtual VertexLayout& GetVertexLayout() = 0;

		virtual const std::filesystem::path& GetFilePath() const = 0;
		virtual const std::string& GetFileName() const = 0;

		virtual bool ReCompile() = 0;

		virtual Ref<ConstantBuffer> CreateConstantBuffer(const std::string& name) = 0;

		virtual void Bind() = 0;
		virtual void UnBind() = 0;

		static Ref<Shaders> Create(const std::filesystem::path& filepath);
	};

	class ShaderLibrary
	{
	public:
		ShaderLibrary() = default;
		~ShaderLibrary() { Clear(); }

		ShaderLibrary(const ShaderLibrary&) = delete;
		ShaderLibrary& operator=(const ShaderLibrary&) = delete;

		Ref<Shaders> Load(const std::filesystem::path& filepath);
		Ref<Shaders> Load(const std::filesystem::path& filepath, const std::string name);

		void Add(Ref<Shaders> shader);
		void Add(Ref<Shaders> shader, const std::string& name);

		Ref<Shaders> Get(const std::string& name);
		Ref<Shaders> TryGet(const std::string& name);

		Ref<Shaders> Remove(const std::string& name);
		Ref<Shaders> Remove(Ref<Shaders> shader);

		bool Exists(const std::string& name);
		bool Exists(Ref<Shaders> shader);

		void Clear();

		std::unordered_map<std::string, Ref<Shaders>>::const_iterator begin() const { return m_Shaders.cbegin(); }
		std::unordered_map<std::string, Ref<Shaders>>::const_iterator end() const { return m_Shaders.cend(); }
	private:
		std::unordered_map<std::string, Ref<Shaders>> m_Shaders;
	};

}