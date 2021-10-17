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

	class ConstantBufferSet : public RefCount
	{
	public:
		Ref<ConstantBuffer> Create(uint32_t size, uint32_t slot);
		Ref<ConstantBuffer> Get(uint32_t index);
		uint32_t BufferCount() const { return (uint32_t)m_ConstantBuffers.size(); }

		void Bind();
		void UnBind();

		static Ref<ConstantBufferSet> Create();
	private:
		std::vector<Ref<ConstantBuffer>> m_ConstantBuffers;
	};

}
