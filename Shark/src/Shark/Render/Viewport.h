#pragma once

namespace Shark {

	class Viewport
	{
	public:
		virtual ~Viewport() = default;

		virtual void Bind() = 0;
		virtual void UnBind() = 0;

		virtual uint32_t GetWidth() = 0;
		virtual uint32_t GetHeight() = 0;

		virtual void Resize(uint32_t width, uint32_t height) = 0;

		static Ref<Viewport> Create(uint32_t width, uint32_t height);
	};

}
