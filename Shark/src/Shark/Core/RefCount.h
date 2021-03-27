#pragma once

namespace Shark {

	class RefCount
	{
	public:
		RefCount() = default;
		RefCount(const RefCount& other) { m_RefCount = other.m_RefCount; m_WeakCount = other.m_WeakCount; }
		RefCount(RefCount&& other) noexcept { m_RefCount = other.m_RefCount; m_WeakCount = other.m_WeakCount; other.m_RefCount = 0; other.m_WeakCount = 0; }
		const RefCount& operator=(const RefCount& other) { m_RefCount = other.m_RefCount; m_WeakCount = other.m_WeakCount; }
		const RefCount& operator=(RefCount&& other) noexcept { m_RefCount = other.m_RefCount; m_WeakCount = other.m_WeakCount;other.m_RefCount = 0;other.m_WeakCount = 0;return *this; }

	protected:
		uint32_t AddRef() { return ++m_RefCount; }
		uint32_t DecRef() { return --m_RefCount; }
		uint32_t GetRefCount() const { return m_RefCount; }

		uint32_t AddWeak() { return ++m_WeakCount; }
		uint32_t DecWeak() { return --m_WeakCount; }
		uint32_t GetWeakCount() const { return m_WeakCount; }
	private:
		uint32_t m_RefCount = 0;
		uint32_t m_WeakCount = 0;

		template<typename T>
		friend class Ref;
		template<typename T>
		friend class WeakRef;
	};

	template<typename T>
	class WeakRef;

	template<typename T>
	class Ref
	{
	public:
		Ref() = default;
		Ref(std::nullptr_t) {}
		Ref(Ref& other) { SK_CORE_ASSERT(m_Instance == nullptr); m_Instance = other.m_Instance; if (m_Instance) { m_Instance->AddRef(); } }
		Ref(Ref&& other) noexcept { SK_CORE_ASSERT(m_Instance == nullptr); m_Instance = other.m_Instance; other.m_Instance = nullptr; }
		const Ref& operator=(Ref& other) { Release(); m_Instance = other.m_Instance; if (m_Instance) { m_Instance->AddRef(); } return *this; }
		const Ref& operator=(Ref&& other) noexcept { Release();  m_Instance = other.m_Instance; other.m_Instance = nullptr; return *this; }
		~Ref() { Release(); }

		template<typename T2, std::enable_if_t<std::is_convertible<T2*, T*>::type::value, bool> = true>
		Ref(const Ref<T2>& other) { Release();  m_Instance = other.m_Instance; if (m_Instance) { m_Instance->AddRef(); }  }
		template<typename T2, std::enable_if_t<std::is_convertible<T2*, T*>::type::value, bool> = true>
		Ref(Ref<T2>&& other) noexcept { Release();  m_Instance = other.m_Instance; other.m_Instance = nullptr; }
		template<typename T2, std::enable_if_t<std::is_convertible<T2*, T*>::type::value, bool> = true>
		const Ref& operator=(const Ref<T2>& other) { Release();  m_Instance = other.m_Instance; if (m_Instance) { m_Instance->AddRef(); } return *this; }
		template<typename T2, std::enable_if_t<std::is_convertible<T2*, T*>::type::value, bool> = true>
		const Ref& operator=(Ref<T2>&& other) noexcept { Release(); m_Instance = other.m_Instance; other.m_Instance = nullptr; return *this; }

		void Release()
		{
			if (m_Instance)
			{
				SK_CORE_ASSERT(m_Instance->GetRefCount() != 0, "Release was called but refcount was 0");
				if (m_Instance->DecRef() == 0)
				{
					SK_IF_DEBUG(if (uint32_t wc = m_Instance->GetWeakCount(); wc > 0) SK_CORE_INFO("Insteance is going to be deleated but there are still {0} Weak Refrences", wc););
					delete m_Instance;
				}
				m_Instance = nullptr;
			}
		}

		WeakRef<T> GetWeak() const { return WeakRef<T>::Create(*this); }

		T* operator->() const { return m_Instance; }
		T& operator*() const { return *m_Instance; }

		operator bool() { return m_Instance != nullptr; }
		bool operator==(const Ref& rhs) { SK_CORE_ASSERT((m_Instance == rhs.m_Instance ? m_Instance->GetRefCount() == rhs.m_Instance->GetRefCount() : true)); return m_Instance == rhs.m_Instance; }
		bool operator!=(const Ref& rhs) { return !(*this == rhs); }

		template<typename... Args>
		static Ref Create(Args&&... args) { return Ref(new T(std::forward<Args>(args)...)); }
		static Ref Create(T* inst) { return Ref(inst); }
	private:
		explicit Ref(T* inst) { m_Instance = inst; if (m_Instance) { m_Instance->AddRef(); } }

	public:
		T* m_Instance = nullptr;

		template<typename T2>
		friend class Ref;
	};

	template<typename T>
	class WeakRef
	{
	public:
		WeakRef() = default;
		WeakRef(std::nullptr_t) {}
		WeakRef(const WeakRef& other) { SK_CORE_ASSERT(m_Instance == nullptr); m_Instance = other.m_Instance; if (m_Instance) { m_Instance->AddWeak(); } }
		WeakRef(WeakRef&& other) { SK_CORE_ASSERT(m_Instance == nullptr); m_Instance = other.m_Instance; other.m_Instance = nullptr; }
		const WeakRef& operator=(const WeakRef& other) { Release(); m_Instance = other.m_Instance; if (m_Instance) m_Instance->AddWeak(); return *this; }
		const WeakRef& operator=(WeakRef&& other) { Release(); m_Instance = other.m_Instance; other.m_Instance = nullptr; return *this; }
		~WeakRef() { Release(); }

		template<typename T2, std::enable_if_t<std::is_convertible<T2*, T*>::type::value, bool> = true>
		WeakRef(const WeakRef<T2>& other) { SK_CORE_ASSERT(m_Instance == nullptr); m_Instance = other.m_Instance; if (m_Instance) { m_Instance->AddWeak(); } }
		template<typename T2, std::enable_if_t<std::is_convertible<T2*, T*>::type::value, bool> = true>
		WeakRef(WeakRef<T2>&& other) { SK_CORE_ASSERT(m_Instance == nullptr); m_Instance = other.m_Instance; other.m_Instamnce = nullptr; }

		template<typename T2, std::enable_if_t<std::is_convertible<T2*, T*>::type::value, bool> = true>
		const WeakRef& operator=(const WeakRef<T2>& other) { m_Instance = other.m_Instance; if (m_Instance) { m_Instance->AddWeak(); } return *this; }
		template<typename T2, std::enable_if_t<std::is_convertible<T2*, T*>::type::value, bool> = true>
		const WeakRef& operator=(WeakRef<T2>&& other) { m_Instance = other.m_Instance; other.m_Instance = nullptr; return *this; }

		void Release() { if (m_Instance) { SK_CORE_ASSERT(m_Instance->GetWeakCount() != 0, "Release was called but WeakCount was 0"); m_Instance->DecWeak(); } m_Instance = nullptr; }

		T& operator*() const { return *m_Instance; }
		T* operator->() const { return m_Instance; }

		operator bool() { return m_Instance != nullptr; }
		bool operator==(const WeakRef& rhs) { SK_CORE_ASSERT((m_Instance == rhs.m_Instance ? m_Instance->GetWeakCount() == rhs.m_Instance->GetWeakCount() : true)); return m_Instance == rhs.m_Instance; }
		bool operator!=(const WeakRef& rhs) { return !(*this == rhs); }


		static WeakRef Create(T* inst) { return WeakRef(inst); }
		static WeakRef Create(const Ref<T>& ref) { return WeakRef(ref); }

	private:
		explicit WeakRef(T* inst) { m_Instance = inst; if (m_Instance) m_Instance->AddWeak(); }
		explicit WeakRef(const Ref<T>& ref) { m_Instance = ref.m_Instance; if (m_Instance) m_Instance->AddWeak(); }

	private:
		T* m_Instance = nullptr;

		template<typename T2>
		friend class WeakRef;
	};

}
