#pragma once

namespace Shark {

	class RefCount
	{
	public:
		RefCount() = default;
		RefCount(const RefCount& other)
			: m_RefCount(other.m_RefCount)
		{}

		RefCount(RefCount&& other)
			: m_RefCount(other.m_RefCount)
		{
			other.m_RefCount = 0;
		}

		const RefCount& operator=(const RefCount& other)
		{
			m_RefCount = other.m_RefCount;
			return *this;
		}

		const RefCount& operator=(RefCount&& other)
		{
			m_RefCount = other.m_RefCount;
			other.m_RefCount = 0;
			return *this;
		}
		virtual ~RefCount() = default;

		uint32_t GetRefCount() const
		{
			return m_RefCount;
		}

	protected:
		uint32_t AddRef()
		{
			return ++m_RefCount;
		}

		uint32_t DecRef()
		{
			return --m_RefCount;
		}

	private:
		uint32_t m_RefCount = 0;

		template<typename T>
		friend class Ref;
		template<typename T>
		friend class Weak;
	};

	template<typename T>
	class Weak;

	template<typename T>
	class Ref
	{
	public:
		Ref() = default;
		Ref(std::nullptr_t) {};
		Ref(const Ref& other)
			: m_Instance(other.m_Instance)
		{
			if (m_Instance)
				m_Instance->AddRef();
		}

		Ref(Ref&& other)
			: m_Instance(other.m_Instance)
		{
			other.m_Instance = nullptr;
		}

		const Ref& operator=(const Ref& other)
		{
			Release();
			m_Instance = other.m_Instance;
			if (m_Instance)
				m_Instance->AddRef();
		return *this;
		}

		const Ref& operator=(Ref&& other)
		{
			Release();
			m_Instance = other.m_Instance;
			other.m_Instance = nullptr;
			return *this;
		}

		const Ref& operator=(std::nullptr_t)
		{
			Release();
			return *this;
		}

		~Ref()
		{
			Release();
		}

		Ref(T* inst)
		{
			m_Instance = inst;
			if (m_Instance)
				m_Instance->AddRef();
		}

		explicit Ref(const Weak<T>& weak)
			: m_Instance(weak.m_Instance)
		{
			m_Instance->AddRef();
		}

		template<typename T2, std::enable_if_t<std::is_convertible_v<T2*, T*>, int> = 0>
		Ref(const Ref<T2>& other)
		{
			Release();
			m_Instance = other.m_Instance;
			if (m_Instance)
				m_Instance->AddRef(); 
		}

		template<typename T2, std::enable_if_t<std::is_convertible_v<T2*, T*>, int> = 0>
		Ref(Ref<T2>&& other)
		{
			Release();
			m_Instance = other.m_Instance;
			other.m_Instance = nullptr;
		}

		template<typename T2, std::enable_if_t<std::is_convertible_v<T2*, T*>, int> = 0>
		const Ref& operator=(const Ref<T2>& other)
		{
			Release();
			m_Instance = other.m_Instance;
			if (m_Instance)
				m_Instance->AddRef();
			return *this;
		}

		template<typename T2, std::enable_if_t<std::is_convertible_v<T2*, T*>, int> = 0>
		const Ref& operator=(Ref<T2>&& other)
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
				SK_CORE_ASSERT(m_Instance->GetRefCount() != 0, "Release was called but refcount was 0");
				if (m_Instance->DecRef() == 0)
					delete m_Instance;
				m_Instance = nullptr;
			}
		}

		T* operator->() const
		{
			return m_Instance;
		}

		T& operator*() const
		{
			return *m_Instance;
		}

		operator bool() const
		{
			return m_Instance != nullptr;
		}

		bool operator==(T* ptr) const
		{
			return m_Instance == ptr;
		}

		bool operator!=(T* ptr) const
		{
			return m_Instance != ptr;
		}

		template<typename T2>
		bool operator==(const Ref<T2>& rhs) const
		{
			return m_Instance == rhs.m_Instance;
		}

		template<typename T2>
		bool operator!=(const Ref<T2>& rhs) const
		{
			return !(*this == rhs);
		}

		T* Raw() const
		{
			return m_Instance;
		}

		template<typename T2>
		Ref<T2> As() const
		{
			SK_CORE_ASSERT(m_Instance ? dynamic_cast<T2*>(m_Instance) : true);
			return static_cast<T2*>(m_Instance);
		}

		template<typename... Args>
		static Ref Create(Args&&... args)
		{
			return new T(std::forward<Args>(args)...);
		}

	private:
		T* m_Instance = nullptr;

		template<typename> friend class Ref;
		template<typename> friend class Weak;
	};


	template<typename T>
	class Weak
	{
	public:
		Weak() = default;
		Weak(std::nullptr_t) {}
		Weak(const Weak& other)
			: m_Instance(other.m_Instance)
		{
			m_Instance = other.m_Instance;
		}

		Weak(Weak&& other)
			: m_Instance(other.m_Instance)
		{
			m_Instance = other.m_Instance;
			other.m_Instance = nullptr;
		}

		const Weak& operator=(const Weak& other)
		{
			Release();
			m_Instance = other.m_Instance;
			return *this;
		}

		const Weak& operator=(Weak&& other)
		{
			Release();
			m_Instance = other.m_Instance;
			other.m_Instance = nullptr;
			return *this;
		}

		const Weak& operator=(std::nullptr_t)
		{
			Release();
			return *this;
		}

		~Weak()
		{
			Release();
		}

		Weak(T* inst)
			: m_Instance(inst)
		{}

		Weak(const Ref<T>& ref)
			: m_Instance(ref.m_Instance)
		{}

		template<typename T2, std::enable_if_t<std::is_convertible<T2*, T*>::type::value, bool> = true>
		Weak(const Weak<T2>& other)
			: m_Instance(other.m_Instance)
		{}

		template<typename T2, std::enable_if_t<std::is_convertible<T2*, T*>::type::value, bool> = true>
		Weak(Weak<T2>&& other)
		: m_Instance(other.m_Instance)
		{
			other.m_Instance = nullptr;
		}

		template<typename T2, std::enable_if_t<std::is_convertible<T2*, T*>::type::value, bool> = true>
		const Weak& operator=(const Weak<T2>& other)
		{
			m_Instance = other.m_Instance;
			return *this;
		}

		template<typename T2, std::enable_if_t<std::is_convertible<T2*, T*>::type::value, bool> = true>
		const Weak& operator=(Weak<T2>&& other)
		{
			m_Instance = other.m_Instance;
			other.m_Instance = nullptr;
			return *this;
		}

		void Release()
		{
			m_Instance = nullptr;
		}

		T& operator*() const
		{
			return *m_Instance;
		}

		T* operator->() const
		{
			return m_Instance;
		}

		operator bool() const
		{
			return m_Instance != nullptr;
		}

		bool operator==(const Weak& rhs) const
		{
			return m_Instance == rhs.m_Instance;
		}

		bool operator!=(const Weak& rhs) const
		{
			return !(*this == rhs);
		}

		T* Raw() const
		{
			return m_Instance;
		}

		template<typename T2>
		Weak<T2> As() const
		{
			SK_CORE_ASSERT(dynamic_cast<T2*>(m_Instance));
			return static_cast<T2*>(m_Instance);
		}

	private:
		T* m_Instance = nullptr;

		template<typename> friend class Weak;
		template<typename> friend class Ref;
	};

}

namespace std {

	template<typename T>
	struct hash<Shark::Ref<T>>
	{
		auto operator()(const Shark::Ref<T>& val) const
		{
			return hash<void*>()((void*)val.Raw());
		}
	};

	template<typename T>
	struct hash<Shark::Weak<T>>
	{
		auto operator()(const Shark::Weak<T>& val) const
		{
			return hash<void*>()((void*)val.Raw());
		}
	};

}
