#pragma once

namespace Shark {

	template<typename T>
	class Scope
	{
	public:
		Scope() = default;
		Scope(std::nullptr_t) {}
		Scope(Scope&& other) { m_Instance = other.m_Instance; other.m_Instance = nullptr; }
		const Scope& operator=(Scope&& other) { m_Instance = other.m_Instance; other.m_Instance = nullptr; return *this; }

		Scope(const Scope&) = delete;
		const Scope& operator=(const Scope&) = delete;

		~Scope() { Release(); }

		explicit Scope(T* inst) { m_Instance = inst; }

		template<typename T2, std::enable_if_t<std::is_convertible<T2*, T*>::type::value, bool> = true>
		Scope(Scope<T2>&& other) { m_Instance = other.m_Instance;other.m_Instance = nullptr; }
		template<typename T2, std::enable_if_t<std::is_convertible<T2*, T*>::type::value, bool> = true>
		const Scope& operator=(Scope<T2>&& other) { m_Instance = other.m_Instance; other.m_Instance = nullptr; return *this; }

		void Release() { delete m_Instance; }

		T& operator*() const { return *m_Instance; }
		T* operator->() const { return m_Instance; }

		operator bool() { return m_Instance != nullptr; }
		bool operator==(const Scope& rhs) { return m_Instance == rhs.m_Instance; }
		bool operator!=(const Scope& rhs) { return !(*this == rhs); }

		template<typename... Args>
		constexpr static Scope Create(Args&&... args) { return T::Create(std::forward<Args>(args)...); }
		template<typename... Args>
		constexpr static Scope Allocate(Args&&... args) { return Scope(new T(std::forward<Args>(args)...)); }

	private:
		T* m_Instance = nullptr;

		template<typename T2> friend class Scope;

		template<typename T1, typename T2> friend Scope<T1> StaticCast(Scope<T2>& scope);
		template<typename T1, typename T2> friend Scope<T1> DynamicCast(Scope<T2>& scope);
	};

	template<typename T1, typename T2>
	Scope<T1> StaticCast(Scope<T2>& scope)
	{
		T1* ptr = static_cast<T1*>(scope.m_Instance);
		scope.m_Instance = nullptr;
		return std::move(Scope(temp));
	}

	template<typename T1, typename T2>
	Scope<T1> DynamicCast(Scope<T2>& scope)
	{
		T1* ptr = dynamic_cast<T1*>(scope.m_Instance);
		if (ptr)
		{
			scope.m_Instance = nullptr;
			return std::move(Scope(ptr));
		}
		return nullptr;
	}

}
