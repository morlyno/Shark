#pragma once

#include "Shark/Core/Buffer.h"
#include "Shark/Render/ConstantBuffer.h"

#include <d3d11.h>

namespace Shark {

	class DirectXConstantBuffer : public ConstantBuffer
	{
	public:
		DirectXConstantBuffer() = default;
		DirectXConstantBuffer(BufferUsage usage, uint32_t byteSize, Buffer initData);
		DirectXConstantBuffer(const ConstantBufferSpecification& specification, Buffer initData);
		virtual ~DirectXConstantBuffer();

		virtual bool IsDynamic() const override { return m_Specification.Usage == BufferUsage::Dynamic; }
		virtual uint32_t GetByteSize() const override { return m_Specification.ByteSize; }

		virtual void Upload(Buffer data) override;
		virtual void RT_Upload(Buffer data) override;

		void RT_Upload(ID3D11DeviceContext* commandBuffer, Buffer data);

	private:
		void Initialize(Buffer initData);

	private:
		ConstantBufferSpecification m_Specification;
		ID3D11Buffer* m_ConstantBuffer = nullptr;

		friend class DirectXRenderer;
	};

}
