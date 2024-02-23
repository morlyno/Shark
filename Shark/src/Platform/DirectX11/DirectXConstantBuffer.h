#pragma once

#include "Shark/Core/Buffer.h"
#include "Shark/Render/ConstantBuffer.h"

#include <d3d11.h>

namespace Shark {

	class DirectXConstantBuffer : public ConstantBuffer
	{
	public:
		DirectXConstantBuffer() = default;
		DirectXConstantBuffer(uint32_t size);
		virtual ~DirectXConstantBuffer();

		void Invalidate();
		void RT_Invalidate();

		void SetSize(uint32_t size) { m_Size = size; }

		virtual uint32_t GetSize() const override { return m_Size; }

		virtual Buffer& GetUploadBuffer() override { return m_UploadBuffer; }
		virtual Buffer GetUploadBuffer() const override { return m_UploadBuffer; }

		virtual void UploadData(Buffer data) override;
		virtual void RT_UploadData(Buffer data) override;

		virtual void Upload() override;
		virtual void RT_Upload() override;

	private:
		Buffer m_UploadBuffer;
		ID3D11Buffer* m_ConstantBuffer = nullptr;
		uint32_t m_Size = 0;

		friend class DirectXRenderer;
	};

}
