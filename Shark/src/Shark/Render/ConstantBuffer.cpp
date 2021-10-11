#include "skpch.h"
#include "ConstantBuffer.h"

#include "Shark/Render/RendererAPI.h"
#include "Platform/DirectX11/DirectXConstantBuffer.h"

namespace Shark {

	Ref<ConstantBuffer> ConstantBuffer::Create(uint32_t size, uint32_t slot)
	{
		switch (RendererAPI::GetAPI())
		{
			case RendererAPI::API::None: SK_CORE_ASSERT(false, "No RendererAPI Specified"); return nullptr;
			case RendererAPI::API::DirectX11: return Ref<DirectXConstantBuffer>::Create(size, slot);
		}
		SK_CORE_ASSERT(false, "Unkown RendererAPI");
		return nullptr;
	}

	Ref<ConstantBuffer> ConstantBufferSet::Create(uint32_t size, uint32_t slot)
	{
		return m_ConstantBuffers.emplace_back(ConstantBuffer::Create(size, slot));
	}

	Ref<ConstantBuffer> ConstantBufferSet::Get(uint32_t index)
	{
		return m_ConstantBuffers[index];
	}

	void ConstantBufferSet::Bind()
	{
		for (auto cb : m_ConstantBuffers)
			cb->Bind();
	}

	void ConstantBufferSet::UnBind()
	{
		for (auto cb : m_ConstantBuffers)
			cb->UnBind();
	}

	Ref<ConstantBufferSet> ConstantBufferSet::Create()
	{
		return Ref<ConstantBufferSet>::Create();
	}

}