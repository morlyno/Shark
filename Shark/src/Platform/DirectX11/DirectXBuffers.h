#pragma once

#include "Shark/Render/Buffers.h"
#include <d3d11.h>

namespace Shark {

	class DirectXVertexBuffer : public VertexBuffer
	{
	public:
		DirectXVertexBuffer(const VertexLayout& layout, bool dynamic);
		DirectXVertexBuffer(const VertexLayout& layout, void* data, uint32_t size, bool dynamic);
		~DirectXVertexBuffer();

		void Init(void* data, uint32_t size, bool dynamic);

		void SetData(void* data, uint32_t size) override;
		virtual void UpdateData(void* data, uint32_t size) override;

		void Bind() override;
		void UnBind() override;
	private:
		ID3D11Buffer* m_VertexBuffer = nullptr;
		bool m_Dynamic;
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