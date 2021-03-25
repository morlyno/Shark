#pragma once

namespace Shark {

	class RefCount
	{
		template<typename T>
		friend class Ref;
	public:
		RefCount() = default;
		RefCount(const RefCount& other)
			: m_RefCount(other.m_RefCount) {}
		const RefCount& operator=(const RefCount& other)
		{
			m_RefCount = other.m_RefCount;
		}
		RefCount(RefCount&& other) noexcept
		{
			m_RefCount = other.m_RefCount;
			other.m_RefCount = 0;
		}
		const RefCount& operator=(RefCount&& other) noexcept
		{
			m_RefCount = other.m_RefCount;
			other.m_RefCount = 0;
			return *this;
		}

	protected:
		uint32_t AddRef() { m_RefCount++; return m_RefCount; }
		uint32_t DecRef() { m_RefCount--; return m_RefCount; }
		uint32_t GetRefCount() const { return m_RefCount; }
	private:
		uint32_t m_RefCount = 0;
	};

	template<typename T>
	class WeakRef;

	template<typename T>
	class Ref
	{
	public:
		Ref() = default;
		Ref(std::nullptr_t) {}
		Ref(Ref& other)
		{
			SK_CORE_ASSERT(m_Instance == nullptr);
			m_Instance = other.m_Instance;
			if (m_Instance)
				m_Instance->AddRef();
		}
		const Ref& operator=(Ref& other)
		{
			Release();

			m_Instance = other.m_Instance;
			if (m_Instance)
				m_Instance->AddRef();
			return *this;
		}
		Ref(Ref&& other) noexcept
		{
			SK_CORE_ASSERT(m_Instance == nullptr);
			m_Instance = other.m_Instance;
			other.m_Instance = nullptr;
		}
		const Ref& operator=(Ref&& other) noexcept
		{
			Release();

			m_Instance = other.m_Instance;
			other.m_Instance = nullptr;
			return *this;
		}
		~Ref()
		{
			Release();
		}

		template<typename T2, std::enable_if_t<std::is_convertible<T2*, T*>::type::value, bool> = true>
		Ref(const Ref<T2>& other)
		{
			SK_CORE_ASSERT(m_Instance == nullptr);
			m_Instance = other.m_Instance;
			if (m_Instance)
				m_Instance->AddRef();
		}

		template<typename T2, std::enable_if_t<std::is_convertible<T2*, T*>::type::value, bool> = true>
		Ref(Ref<T2>&& other) noexcept
		{
			m_Instance = other.m_Instance;
			other.m_Instance = nullptr;
		}

		template<typename T2, std::enable_if_t<std::is_convertible<T2*, T*>::type::value, bool> = true>
		const Ref& operator=(const Ref<T2>& other)
		{
			Release();

			m_Instance = other.m_Instance;
			if (m_Instance)
				m_Instance->AddRef();
			return *this;
		}

		template<typename T2, std::enable_if_t<std::is_convertible<T2*, T*>::type::value, bool> = true>
		const Ref& operator=(Ref<T2>&& other) noexcept
		{
			Release();

			m_Instance = other.m_Instance;
			other.m_Instance = nullptr;
			return *this;
		}

		void Release()
		{
			if (m_Instance)
			{
				SK_CORE_ASSERT(m_Instance->GetRefCount() > 0, "Release was called but refcount was 0");
				if (m_Instance->DecRef() == 0)
					delete m_Instance;
				m_Instance = nullptr;
			}
		}

		WeakRef<T> CreateWeak() const { return WeakRef<T>::Create(m_Instance); }

		T* operator->() const { return m_Instance; }
		T& operator*() const { return *m_Instance; }

		operator bool() { return m_Instance != nullptr; }
		bool operator==(const Ref& rhs) { SK_CORE_ASSERT((m_Instance == rhs.m_Instance ? m_Instance->GetRefCount() == rhs.m_Instance->GetRefCount() : true)); return m_Instance == rhs.m_Instance; }
		bool operator!=(const Ref& rhs) { return !(*this == rhs); }

		template<typename... Args>
		static Ref Create(Args&&... args) { return Ref(new T(std::forward<Args>(args)...)); }
		static Ref Create(T* inst) { return Ref(inst); }
	private:
		explicit Ref(T* inst)
		{
			m_Instance = inst;
			if (m_Instance)
				m_Instance->AddRef();
		}

	public:
		T* m_Instance = nullptr;
	};

	template<typename T>
	class WeakRef
	{
	public:
		WeakRef() = default;
		WeakRef(std::nullptr_t) {}
		WeakRef(const WeakRef&) = default;
		WeakRef(WeakRef&&) = default;
		WeakRef& operator=(const WeakRef&) = default;
		WeakRef& operator=(WeakRef&&) = default;
		~WeakRef() = default;

		explicit WeakRef(const Ref<T>& ref) { m_Instance = ref.m_Instance; }
		

		T& operator*() const { return *m_Instance; }
		T* operator->() const { return m_Instance; }

		operator bool() { return m_Instance != nullptr; }
		bool operator==(const WeakRef& rhs) { return m_Instance == rhs.m_Instance; }
		bool operator!=(const WeakRef& rhs) { return !(*this == rhs); }

		static WeakRef Create(T* inst) { return WeakRef(inst); }

	private:
		explicit WeakRef(T* inst) { m_Instance = inst; }

	public:
		T* m_Instance = nullptr;
	};

}
