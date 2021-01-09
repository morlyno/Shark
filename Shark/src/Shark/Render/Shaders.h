#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/Buffers.h"

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

		virtual void UploudData(const std::string& bufferName, ShaderType type, void* data, uint32_t size) = 0;

		virtual VertexLayout& GetVertexLayout() = 0;

		virtual void Bind() = 0;
		virtual void UnBind() = 0;

		static Ref<Shaders> Create(const std::string& vertexshaderSrc, const std::string& pixelshaderSrc);
	};

}