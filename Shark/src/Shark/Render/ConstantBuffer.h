#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Buffer.h"
#include "Shark/Render/RenderCommandBuffer.h"

namespace Shark {

	class ConstantBuffer : public RefCount
	{
	public:
		virtual ~ConstantBuffer() = default;

		virtual void SetSlot(uint32_t slot) = 0;

		virtual void Set(void* data, uint32_t size) = 0;

		void UploadData(Buffer data) { Set(data.Data, data.Size); }

		virtual uint32_t GetSize() const = 0;
		virtual uint32_t GetSlot() const = 0;

	public:
		static Ref<ConstantBuffer> Create(uint32_t size, uint32_t slot);
	};

	// TODO: create directx version
	class ConstantBufferSet : public RefCount
	{
	public:
		virtual ~ConstantBufferSet() = default;

		virtual Ref<ConstantBuffer> Create(uint32_t size, uint32_t slot) = 0;
		virtual Ref<ConstantBuffer> Get(uint32_t slot) const = 0;
		virtual void Set(uint32_t slot, void* data, uint32_t size) = 0;
		virtual void Add(Ref<ConstantBuffer> constantBuffer) = 0;

	public:
		static Ref<ConstantBufferSet> Create();
	};

}
