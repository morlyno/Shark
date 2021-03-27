#include "skpch.h"
#include "DirectXViewport.h"

namespace Shark {

	DirectXViewport::DirectXViewport(uint32_t width, uint32_t height, APIContext apicontext)
		: m_ApiContext(apicontext), m_Width(width), m_Height(height)
	{
	}

	DirectXViewport::~DirectXViewport()
	{
	}

	void DirectXViewport::Bind()
	{
		D3D11_VIEWPORT vp;
		vp.TopLeftX = 0u;
		vp.TopLeftY = 0u;
		vp.Width = (float)m_Width;
		vp.Height = (float)m_Height;
		vp.MinDepth = 0u;
		vp.MaxDepth = 1u;
		m_ApiContext.Context->RSSetViewports(1, &vp);
	}

	void DirectXViewport::UnBind()
	{
		m_ApiContext.Context->RSSetViewports(0, nullptr);
	}

	void DirectXViewport::Resize(uint32_t width, uint32_t height)
	{
		m_Width = width;
		m_Height = height;

		Bind();
	}

}
