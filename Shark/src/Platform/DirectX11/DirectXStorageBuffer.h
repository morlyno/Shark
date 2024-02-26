#pragma once

#include "Shark/Render/StorageBuffer.h"

#include "Platform/DirectX11/DirectXAPI.h"

namespace Shark {

	class DirectXStorageBuffer : public StorageBuffer
	{
	public:
		DirectXStorageBuffer(uint32_t structSize, uint32_t count);
		~DirectXStorageBuffer();

		virtual void Release() override;
		virtual void Invalidate() override;

		virtual uint32_t& GetStructSize() override { return m_StructSize; }
		virtual uint32_t& GetCount() override { return m_Count; }

		virtual void Upload(Buffer buffer) override;
		void RT_Upload(Buffer buffer);

	private:
		ID3D11Buffer* m_Buffer = nullptr;
		ID3D11ShaderResourceView* m_View = nullptr;

		uint32_t m_StructSize = 0;
		uint32_t m_Count = 0;

		friend class DirectXRenderer;
	};

}
