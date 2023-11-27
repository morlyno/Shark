#pragma once

#include "Shark/Render/Buffers.h"

#include <d3d11.h>

namespace Shark {

	class DirectXVertexBuffer : public VertexBuffer
	{
	public:
		DirectXVertexBuffer(uint64_t size, bool dynamic, Buffer vertexData);
		virtual ~DirectXVertexBuffer();

		virtual void Release() override;
		virtual void RT_Release() override;

		virtual void Resize(uint64_t size) override;
		virtual void Resize(Buffer vertexData) override;
		virtual void SetData(Buffer vertexData, bool allowResize) override;
		
		virtual void OpenWritableBuffer() override;
		virtual void CloseWritableBuffer() override;
		virtual void Write(Buffer vertexData, uint64_t offset) override;

		virtual bool RT_OpenBuffer() override;
		virtual void RT_CloseBuffer() override;
		virtual Buffer RT_GetBuffer() override { return m_WritableBuffer; }

		virtual uint64_t GetBufferSize() const override { return m_Size; }

	private:
		void ReCreateBuffer(uint64_t size, bool dynamic, Buffer vertexData);

		void RT_SetData(Buffer vertexData, bool allowResize);
		void RT_ReCreateBuffer(uint64_t size, bool dynamic, Buffer vertexData);


	private:
		ID3D11Buffer* m_VertexBuffer = nullptr;
		uint64_t m_Size = 0;
		bool m_Dynamic;

		bool m_Mapped = false;
		Buffer m_WritableBuffer;

		friend class DirectXRenderer;
	};


	class DirectXIndexBuffer : public IndexBuffer
	{
	public:
		DirectXIndexBuffer(uint32_t count, bool dynmaic, Buffer indexData);
		virtual ~DirectXIndexBuffer();

		virtual void Release() override;
		virtual void RT_Release() override;

		virtual void Resize(uint32_t count) override;
		virtual void Resize(Buffer indexData) override;
		virtual void SetData(Buffer indexData, bool allowResize = false) override;

		virtual void RT_Resize(uint32_t count, Buffer indexData) override;

		virtual Buffer GetWritableBuffer() override;
		virtual void CloseWritableBuffer() override;

		virtual uint32_t GetCount() const override { return m_Count; }

	private:
		void ReCreateBuffer(uint32_t count, bool dynamic, Buffer indexData);

		void RT_SetData(Buffer indexData, bool allowResize = false);
		void RT_ReCreateBuffer(uint32_t count, bool dynamic, Buffer indexData);

	private:
		ID3D11Buffer* m_IndexBuffer = nullptr;
		uint32_t m_Count;
		bool m_Dynamic;

		bool m_Mapped = false;

		friend class DirectXRenderer;
	};

}