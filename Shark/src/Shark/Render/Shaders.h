#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/VertexLayout.h"

#include <DirectXMath.h>

namespace Shark {

	class Shaders : public RefCount
	{
	public:
		virtual ~Shaders() = default;

		virtual void SetBuffer(const std::string& bufferName, void* data) = 0;

		virtual VertexLayout& GetVertexLayout() = 0;

		virtual void Bind() = 0;
		virtual void UnBind() = 0;

		virtual const std::string& GetName() const = 0;

		static Ref<Shaders> Create(const std::string& filepath);
		static Ref<Shaders> Create(const std::string& vertexshaderSrc, const std::string& pixelshaderSrc);
	};

}