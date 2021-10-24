#pragma once

namespace Shark {

	class ConstantBuffer : public RefCount
	{
	public:
		virtual ~ConstantBuffer() = default;

		virtual void Bind() = 0;
		virtual void UnBind() = 0;

		virtual void SetSlot(uint32_t slot) = 0;

		virtual void Set(void* data, uint32_t size) = 0;

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

		virtual void Bind() = 0;
		virtual void UnBind() = 0;

	public:
		static Ref<ConstantBufferSet> Create();
	};

}
