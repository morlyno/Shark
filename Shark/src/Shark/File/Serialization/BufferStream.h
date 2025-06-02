#pragma once

#include "Shark/Core/Buffer.h"
#include "Shark/File/Serialization/StreamReader.h"
#include "Shark/File/Serialization/StreamWriter.h"

namespace Shark {

	class BufferStreamReader : public StreamReader
	{
	public:
		BufferStreamReader(const Buffer buffer);
		BufferStreamReader(const BufferStreamReader&) = delete;
		~BufferStreamReader();

		virtual bool IsStreamGood() const final { return m_Good; }
		virtual uint64_t GetStreamPosition() final { return m_Position; }
		virtual void SetStreamPosition(uint64_t position) final { m_Position = position; }
		virtual bool ReadData(void* destination, uint64_t size) final;

	private:
		const Buffer m_Buffer;
		uint64_t m_Position = 0;
		bool m_Good = true;
	};

	class BufferStreamWriter : public StreamWriter
	{
	public:
		BufferStreamWriter(Buffer& buffer);
		BufferStreamWriter(const BufferStreamWriter&) = delete;
		~BufferStreamWriter();

		virtual bool IsStreamGood() const final { return m_Good; }
		virtual uint64_t GetStreamPosition() final { return m_Position; }
		virtual void SetStreamPosition(uint64_t position) final { m_Position = position; }
		virtual bool WriteData(const void* data, uint64_t size) final;

	private:
		Buffer& m_Buffer;
		uint64_t m_Position = 0;
		bool m_Good = true;
	};

}
