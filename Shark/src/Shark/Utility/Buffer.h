#pragma once

#include "Shark/Core/Base.h"

namespace Shark {

	class Buffer
	{
	public:
		Buffer() = default;
		~Buffer() = default;

		// Use only if Buffer is Created with Copy
		void Release() { delete m_Buffer; memset(this, 0, sizeof(Buffer)); }

		static Buffer Ref(void* data, uint32_t size)
		{
			Buffer b;
			b.m_Size = size;
			b.m_Buffer = (void*)data;
			return b;
		}

		template<typename T>
		static Buffer Ref(const T& data)
		{
			return Ref(&data, sizeof(T));
		}

		template<typename T, uint32_t Count>
		static Buffer Ref(T (&data)[Count])
		{
			return Ref(data, Count * sizeof(T));
		}

		template<typename T>
		static Buffer Ref(const std::vector<T>& data)
		{
			return Ref(data.data(), (uint32_t)(data.size() * sizeof(T)));
		}

		static Buffer Copy(void* data, uint32_t size)
		{
			Buffer b;
			b.m_Size = size;
			b.m_Buffer = ::operator new(size);
			memcpy(b.m_Buffer, data, size);
			return b;
		}

		template<typename T>
		static Buffer Copy(const T& data)
		{
			return Copy(&data, sizeof(T));
		}

		template<typename T, uint32_t Count>
		static Buffer Copy(T (&data)[Count])
		{
			return Copy(data, Count * sizeof(T));
		}


		void CopyInto(void* dest) const { memcpy(dest, m_Buffer, m_Size); }
		void MoveInto(void* dest) { CopyInto(dest); m_Buffer = nullptr; m_Size = 0; }

		void* Data() { return m_Buffer; }
		const void* Data() const { return m_Buffer; }

		uint32_t Size() const { return m_Size; }
		template<typename T>
		uint32_t Count() const { return m_Size / sizeof(T); }

		template<typename T>
		T& as() { return *(T*)m_Buffer; }
		template<typename T>
		const T& as() const { return *(T*)m_Buffer; }

		template<typename T>
		T& Index(uint32_t index) { SK_CORE_ASSERT(index < m_Size / sizeof(T)); return ((T*)m_Buffer)[index] }
		template<typename T>
		const T& Index(uint32_t index) const { SK_CORE_ASSERT(index < m_Size / sizeof(T)); return ((T*)m_Buffer)[index] }

		bool IsNull() const { return m_Buffer == nullptr; }

		bool operator==(const Buffer& rhs) { return m_Buffer == rhs.m_Buffer && m_Size == rhs.m_Size; }
		bool operator!=(const Buffer& rhs) { return !(*this == rhs); }

		operator bool() const { return !IsNull(); }

	private:
		void* m_Buffer = nullptr;
		uint32_t m_Size = 0;
	};

}
