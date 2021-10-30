#pragma once

#include "Shark/Render/ConstantBuffer.h"

#include <d3d11.h>

namespace Shark {

	class DirectXConstantBuffer : public ConstantBuffer
	{
	public:
		DirectXConstantBuffer(uint32_t size, uint32_t slot);
		virtual ~DirectXConstantBuffer();

		void Bind(ID3D11DeviceContext* ctx);
		void UnBind(ID3D11DeviceContext* ctx);

		virtual void SetSlot(uint32_t slot) override { m_Slot = slot; }

		virtual void Set(void* data, uint32_t size) override;

	private:
		ID3D11Buffer* m_ConstBuffer = nullptr;
		uint32_t m_Size = 0;
		uint32_t m_Slot = 0;

		friend class DirectXRenderer;
	};

	class DirectXConstantBufferSet : public ConstantBufferSet
	{
	public:
		virtual Ref<ConstantBuffer> Create(uint32_t size, uint32_t slot) override;
		virtual Ref<ConstantBuffer> Get(uint32_t slot) const override;

	private:
		std::unordered_map<uint32_t, Ref<DirectXConstantBuffer>> m_CBMap;

		friend class DirectXRenderer;
	};

}
