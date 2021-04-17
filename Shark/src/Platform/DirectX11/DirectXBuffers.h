#pragma once

#include "Shark/Render/Buffers.h"

#include "Platform/DirectX11/DirectXRendererAPI.h"

#include <d3d11.h>

namespace Shark {

	class DirectXVertexBuffer : public VertexBuffer
	{
	public:
		DirectXVertexBuffer(const VertexLayout& layout, const Buffer& data, bool dynamic);
		virtual ~DirectXVertexBuffer();

		virtual void SetData(const Buffer& data) override;

		virtual uint32_t GetSize() const { return m_Size; }

		virtual void Bind() override;
		virtual void UnBind() override;
	private:
		void CreateBuffer(const Buffer& data);
	private:
		Weak<DirectXRendererAPI> m_DXApi;

		ID3D11Buffer* m_VertexBuffer = nullptr;
		uint32_t m_Size = 0;
		bool m_Dynamic;
	};


	class DirectXIndexBuffer : public IndexBuffer
	{
	public:
		DirectXIndexBuffer(const Buffer& data, bool dynamic);
		virtual ~DirectXIndexBuffer();

		virtual void SetData(const Buffer& data) override;

		virtual uint32_t GetSize() const { return m_Size; }

		virtual void Bind() override;
		virtual void UnBind() override;
	private:
		void CreateBuffer(const Buffer& data);
	private:
		Weak<DirectXRendererAPI> m_DXApi;

		ID3D11Buffer* m_IndexBuffer = nullptr;
		uint32_t m_Size = 0;
		bool m_Dynamic;
	};

}