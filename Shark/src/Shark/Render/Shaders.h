#pragma once

#include "Shark/Core/Core.h"

namespace Shark {

	class Shaders
	{
	public:
		virtual ~Shaders() = default;

		virtual void SetInputs( class VertexLayout& layout ) = 0;

		virtual void Bind() = 0;
		virtual void UnBind() = 0;

		static std::shared_ptr<Shaders> Create( const std::string& vertexshaderSrc,const std::string& pixelshaderSrc );
		static std::shared_ptr<Shaders> Create( VertexLayout& layout,const std::string& vertexshaderSrc,const std::string& pixelshaderSrc );
	};

}