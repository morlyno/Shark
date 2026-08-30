#include "skpch.h"
#include "GpuBuffer.h"

#include "Shark/Render/DeviceManager.h"
#include "Shark/Render/Renderer.h"

#include "Shark/Debug/Profiler.h"

namespace Shark {

	GpuBuffer::GpuBuffer(const nvrhi::BufferDesc& desc)
		: m_Desc(desc), m_ByteSize(desc.byteSize)
	{
		InvalidateFromState(RT_State{ .ByteSize = m_ByteSize });

	}

	GpuBuffer::~GpuBuffer()
	{
	}

	void GpuBuffer::Upload(BufferHandle data)
	{
		if (!data)
			return;

		Ref instance = this;
		Renderer::Submit([instance, storage = data.Store()]()
		{
			instance->RT_Upload(storage.AsBuffer());
		});
	}

	void GpuBuffer::RT_Upload(const Buffer data)
	{
		SK_PROFILE_FUNCTION();

		if (!data)
			return;

		auto deviceManager = Renderer::GetDeviceManager();
		deviceManager->ExecuteCommand([handle = m_BufferHandle, data](nvrhi::ICommandList* cmd)
		{
			cmd->writeBuffer(handle, data.As<const void>(), data.Size);
		});
	}

	void GpuBuffer::InvalidateFromState(const RT_State& state)
	{
		m_Desc.setByteSize(state.ByteSize);

		auto device = Renderer::GetGraphicsDevice();
		m_BufferHandle = device->createBuffer(m_Desc);
	}

	void GpuBuffer::ResizeBuffer(uint64_t newSize)
	{
		if (m_ByteSize == newSize)
			return;

		m_ByteSize = newSize;

		Ref instance = this;
		Renderer::Submit([instance, state = RT_State{ .ByteSize = m_ByteSize }]() mutable
		{
			instance->InvalidateFromState(state);
		});
	}

}
