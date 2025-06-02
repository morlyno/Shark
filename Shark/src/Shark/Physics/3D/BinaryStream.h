#pragma once

#include "Shark/Core/Base.h"
#include "Shark/File/Serialization/BufferStream.h"

#include <Jolt/Jolt.h>
#include "Jolt/Core/StreamIn.h"
#include "Jolt/Core/StreamOut.h"

namespace Shark {

	class JoltBinaryStreamWriter : public JPH::StreamOut
	{
	public:
		JoltBinaryStreamWriter();
		~JoltBinaryStreamWriter();
		virtual void WriteBytes(const void* inData, size_t inNumBytes) override;
		virtual bool IsFailed() const override;

		Buffer GetFinalBuffer() { return Buffer::Copy(m_Buffer.Data, m_Stream.GetStreamPosition()); }
	private:
		Buffer m_Buffer;
		BufferStreamWriter m_Stream;
	};

	class JoltBinaryStreamReader : public JPH::StreamIn
	{
	public:
		JoltBinaryStreamReader(const Buffer buffer);
		virtual void ReadBytes(void* outData, size_t inNumBytes) override;
		virtual bool IsEOF() const override;
		virtual bool IsFailed() const override;
	private:
		BufferStreamReader m_Stream;
	};

}
