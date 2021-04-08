#pragma once

#include "Shark/Render/RendererAPI.h"
#include <d3d11.h>

namespace Shark {

	class DirectXRendererAPI : public RendererAPI
	{
	public:
		virtual void Init() override;
		virtual void ShutDown() override;

		virtual void DrawIndexed(uint32_t count, uint32_t indexoffset, uint32_t vertexoffset) override;
		virtual void Flush() override;

		ID3D11Device* GetDevice() const { return m_Device; }
		ID3D11DeviceContext* GetContext() const { return m_Context; }
		IDXGIFactory* GetFactory() const { return m_Factory; }
	private:
		IDXGIFactory* m_Factory = nullptr;
		ID3D11Device* m_Device = nullptr;
		ID3D11DeviceContext* m_Context = nullptr;
	};

}