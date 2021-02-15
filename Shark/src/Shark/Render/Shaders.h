#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/Buffers.h"

#include <DirectXMath.h>

namespace Shark {

	class Shaders
	{
	public:
		virtual ~Shaders() = default;

		virtual void SetBuffer(const std::string& bufferName, void* data, uint32_t size) = 0;

		virtual VertexLayout& GetVertexLayout() = 0;

		virtual void Bind() = 0;
		virtual void UnBind() = 0;

		virtual const std::string& GetName() = 0;

		static Ref<Shaders> Create(const std::string& filepath);
		static Ref<Shaders> Create(const std::string& vertexshaderSrc, const std::string& pixelshaderSrc);
	};

	class ShaderLib
	{
	public:
		void Add(const std::string& name, Ref<Shaders> shaders);
		void Add(Ref<Shaders> shaders);

		Ref<Shaders> Load(const std::string& name, const std::string& filepath);
		Ref<Shaders> Load(const std::string& filepath);

		Ref<Shaders> Get(const std::string& name);

		bool Exits(const std::string& name);
	private:
		std::unordered_map<std::string, Ref<Shaders>> s_Shaders;
	};

}