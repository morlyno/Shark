#pragma once

#include "Shark/Render/Buffers.h"
#include <d3d11.h>

namespace Shark {

	class DirectXVertexBuffer : public VertexBuffer
	{
	public:
		DirectXVertexBuffer(const VertexLayout& layout);
		DirectXVertexBuffer(const VertexLayout& layout, void* data, uint32_t size);
		~DirectXVertexBuffer();

		void Init(void* data, uint32_t size);

		void SetData(void* data, uint32_t count) override;

		void Bind() override;
		void UnBind() override;
	private:
		ID3D11Buffer* m_VertexBuffer = nullptr;
	};


	class DirectXIndexBuffer : public IndexBuffer
	{
	public:
		DirectXIndexBuffer(uint32_t* indices, uint32_t count);
		~DirectXIndexBuffer();

		void Init(uint32_t* indices, uint32_t count);

		void Bind() override;
		void UnBind() override;
	private:
		ID3D11Buffer* m_IndexBuffer = nullptr;
	};

}