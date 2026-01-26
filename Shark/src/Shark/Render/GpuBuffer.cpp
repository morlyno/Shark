#include "skpch.h"
#include "GpuBuffer.h"

#include "Shark/Core/Application.h"
#include "Shark/Render/Renderer.h"
#include "Shark/Debug/Profiler.h"

namespace Shark {

	GpuBuffer::GpuBuffer(const nvrhi::BufferDesc& desc)
		: m_Desc(desc), m_ByteSize(desc.byteSize)
	{
		InvalidateFromState(RT_State{ .ByteSize = m_ByteSize });

		m_LocalStorage.Allocate(m_ByteSize);
		Allocator::SetMemoryDescription(m_LocalStorage.Data, "LocalStorage");
	}

	GpuBuffer::~GpuBuffer()
	{
		m_LocalStorage.Release();
	}

	void GpuBuffer::Upload(const Buffer data)
	{
		m_LocalStorage.Write(data);

		Ref instance = this;
		Renderer::Submit([instance, storage = m_LocalStorage]()
		{
			instance->RT_Upload(storage);
		});
	}

	void GpuBuffer::RT_Upload(const Buffer data)
	{
		SK_PROFILE_FUNCTION();

		auto deviceManager = Renderer::GetDeviceManager();
		deviceManager->ExecuteCommand([handle = m_BufferHandle, data](nvrhi::ICommandList* cmd)
		{
			cmd->writeBuffer(handle, data.As<const void>(), data.Size);
		});
	}

	void GpuBuffer::InvalidateFromState(const RT_State& state)
	{
		m_Desc.setByteSize(state.ByteSize);

		auto device = Application::Get().GetDeviceManager()->GetDevice();
		m_BufferHandle = device->createBuffer(m_Desc);
	}

	void GpuBuffer::ResizeBuffer(uint64_t newSize)
	{
		if (m_ByteSize == newSize)
			return;

		m_ByteSize = newSize;

		Buffer temp = std::exchange(m_LocalStorage, {});
		m_LocalStorage.Allocate(m_ByteSize);
		Allocator::SetMemoryDescription(m_LocalStorage.Data, "LocalStorage");

		Ref instance = this;
		Renderer::Submit([instance, state = RT_State{ .ByteSize = m_ByteSize }, temp]() mutable
		{
			instance->InvalidateFromState(state);
			temp.Release();
		});
	}

}
