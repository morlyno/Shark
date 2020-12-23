#pragma once

#include "Shark/Core/Core.h"

#include <DirectXMath.h>

namespace Shark {

	enum class ShaderType
	{
		VertexShader, PixelShader
	};

	class Shaders
	{
	public:
		virtual ~Shaders() = default;

		virtual void SetInputs( class VertexLayout& layout ) = 0;

		virtual void SetSceanData(ShaderType target, uint32_t slot, void* data, uint32_t size) = 0;

		virtual void Bind() = 0;
		virtual void UnBind() = 0;

		static std::shared_ptr<Shaders> Create( const std::string& vertexshaderSrc,const std::string& pixelshaderSrc );
		static std::shared_ptr<Shaders> Create( VertexLayout& layout,const std::string& vertexshaderSrc,const std::string& pixelshaderSrc );
	};

}