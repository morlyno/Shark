#pragma once

namespace Shark {

	class RefCount
	{
	public:
		RefCount() = default;
		RefCount(const RefCount& other) = delete;
		RefCount(RefCount&& other) = delete;
		const RefCount& operator=(const RefCount& other) = delete;
		const RefCount& operator=(RefCount&& other) = delete;
		virtual ~RefCount() = default;

		uint32_t GetRefCount() const
		{
			return m_RefCount;
		}

	private:
		uint32_t AddRef()
		{
			return ++m_RefCount;
		}

		uint32_t DecRef()
		{
			return --m_RefCount;
		}

		std::weak_ptr<uint8_t> GetWeakCheckData()
		{
			return m_WeakCheckData;
		}

		void PreDelete()
		{
			SK_CORE_VERIFY(m_RefCount == 0);
			m_WeakCheckData.reset();
		}

	private:
		std::atomic<uint32_t> m_RefCount = 0;
		std::shared_ptr<uint8_t> m_WeakCheckData = std::make_shared<uint8_t>();

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
		{
			ReleaseAndAssign(other);
		}

		Ref(Ref&& other)
		{
			ReleaseAndMove(std::move(other));
		}

		const Ref& operator=(const Ref& other)
		{
			ReleaseAndAssign(other);
			return *this;
		}

		const Ref& operator=(Ref&& other)
		{
			ReleaseAndMove(std::move(other));
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
			Release();
			m_Instance = inst;
			if (m_Instance)
				m_Instance->AddRef();
		}

		template<typename T2, std::enable_if_t<std::is_convertible_v<T2*, T*>, int> = 0>
		Ref(const Ref<T2>& other)
		{
			ReleaseAndAssign(other);
		}

		template<typename T2, std::enable_if_t<std::is_convertible_v<T2*, T*>, int> = 0>
		Ref(Ref<T2>&& other)
		{
			ReleaseAndMove(std::move(other));
		}

		template<typename T2, std::enable_if_t<std::is_convertible_v<T2*, T*>, int> = 0>
		const Ref& operator=(const Ref<T2>& other)
		{
			ReleaseAndAssign(other);
			return *this;
		}

		template<typename T2, std::enable_if_t<std::is_convertible_v<T2*, T*>, int> = 0>
		const Ref& operator=(Ref<T2>&& other)
		{
			ReleaseAndMove(std::move(other));
			return *this;
		}

		void Release()
		{
			if (m_Instance)
			{
				SK_CORE_VERIFY(m_Instance->GetRefCount() != 0, "Release was called but refcount was 0");
				if (m_Instance->DecRef() == 0)
				{
					m_Instance->PreDelete();
					skdelete m_Instance;
				}
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

		T* Raw()
		{
			return m_Instance;
		}

		const T* Raw() const
		{
			return m_Instance;
		}

		Ref Clone() const
		{
			return Create(*m_Instance);
		}

		template<typename T2>
		Ref<T2> As() const
		{
			SK_CORE_VERIFY(m_Instance ? dynamic_cast<T2*>(m_Instance) : true);
			return (T2*)m_Instance;
		}

		template<typename... Args>
		static Ref Create(Args&&... args)
		{
			return new(typeid(T).name()) T(std::forward<Args>(args)...);
		}

	private:
		template<typename T2>
		void ReleaseAndAssign(const Ref<T2>& other)
		{
			Release();
			m_Instance = other.m_Instance;
			if (m_Instance)
				m_Instance->AddRef();
		}

		template<typename T2>
		void ReleaseAndMove(Ref<T2>&& other)
		{
			Release();
			m_Instance = other.m_Instance;
			other.m_Instance = nullptr;
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

		Weak(Ref<T> ref)
		{
			Assign(ref.Raw());
		}

		Weak(T* inst)
		{
			Assign(inst);
		}

		const Weak& operator=(Ref<T> ref)
		{
			Assign(ref.Raw());
			return *this;
		}

		const Weak& operator=(T* inst)
		{
			Assign(inst);
			return *this;
		}

		Ref<T> GetRef() const
		{
			SK_CORE_VERIFY(!Expired());
			return m_Instance;
		}

		Ref<T> TryGetRef() const
		{
			if (Expired())
				return nullptr;
			return m_Instance;
		}

		bool Expired() const
		{
			return m_WeakControlData.expired();
		}

		operator bool() const
		{
			return !Expired() && m_Instance;
		}

		T* Raw() const
		{
			return m_Instance;
		}

		template<typename TTo>
		Weak<TTo> As()
		{
			SK_CORE_VERIFY(m_Instance ? dynamic_cast<TTo*>(m_Instance) : true);
			return (TTo*)m_Instance;
		}

	private:
		void Assign(T* inst)
		{
			m_Instance = inst;
			if (inst)
				m_WeakControlData = inst->GetWeakCheckData();
		}

	private:
		T* m_Instance = nullptr;
		std::weak_ptr<uint8_t> m_WeakControlData;

		template<typename> friend class Weak;
		template<typename> friend class Ref;
	};

}
