#pragma once

#include "Shark/Render/ConstantBuffer.h"

#include <d3d11.h>

namespace Shark {

	class DirectXConstantBuffer : public ConstantBuffer
	{
	public:
		DirectXConstantBuffer(uint32_t size, uint32_t slot);
		virtual ~DirectXConstantBuffer();

		virtual void Bind() override;
		virtual void UnBind() override;

		virtual void SetSlot(uint32_t slot) override { m_Slot = slot; }

		virtual void Set(void* data) override;

	private:
		ID3D11Buffer* m_ConstBuffer = nullptr;
		uint32_t m_Size = 0;
		uint32_t m_Slot = 0;

	};

}
