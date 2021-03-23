#pragma once

#include "Shark/Render/Viewport.h"
#include "Platform/DirectX11/DirectXRendererAPI.h"

namespace Shark {

	class DirectXViewport : public Viewport
	{
	public:
		DirectXViewport(uint32_t width, uint32_t height, APIContext apicontext);
		virtual ~DirectXViewport();

		virtual void Bind() override;
		virtual void UnBind() override;

		virtual uint32_t GetWidth() override { return m_Width; }
		virtual uint32_t GetHeight() override { return m_Height; }

		virtual void Resize(uint32_t width, uint32_t height) override;

	private:
		APIContext m_ApiContext;

		uint32_t m_Width;
		uint32_t m_Height;
	};

}
