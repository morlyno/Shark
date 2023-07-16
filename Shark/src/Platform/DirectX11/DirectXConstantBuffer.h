#pragma once

#include "Shark/Core/Buffer.h"
#include "Shark/Render/ConstantBuffer.h"

#include <d3d11.h>

namespace Shark {

	class DirectXConstantBuffer : public ConstantBuffer
	{
	public:
		DirectXConstantBuffer() = default;
		DirectXConstantBuffer(uint32_t size, uint32_t slot);
		virtual ~DirectXConstantBuffer();

		void RT_Init(uint32_t size, uint32_t slot);

		virtual void SetSlot(uint32_t slot) override { m_Slot = slot; }
		virtual void Set(void* data, uint32_t size) override;

		void RT_Set(Buffer buffer);

		virtual uint32_t GetSize() const override { return m_Size; }
		virtual uint32_t GetSlot() const override { return m_Slot; }

	private:
		void RT_CreateBuffer();

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
		virtual void Set(uint32_t slot, void* data, uint32_t size) override;
		virtual void Add(Ref<ConstantBuffer> constantBuffer) override;

	public:
		Ref<ConstantBuffer> RT_Create(uint32_t size, uint32_t slot);
		void RT_Set(uint32_t slot, Buffer data);

	private:
		std::map<uint32_t, Ref<DirectXConstantBuffer>> m_CBMap;

		friend class DirectXRenderer;
	};

}
