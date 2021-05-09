#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/VertexLayout.h"

namespace Shark {

	class Shaders : public RefCount
	{
	public:
		virtual ~Shaders() = default;

		virtual VertexLayout& GetVertexLayout() = 0;

		virtual void Bind() = 0;
		virtual void UnBind() = 0;

		static Ref<Shaders> Create(const std::string& filepath);
	};

}