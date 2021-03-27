#pragma once

namespace Shark {

	template<typename T>
	class Scope
	{
	public:
		Scope() = default;
		Scope(std::nullptr_t) {}

		Scope(const Scope&) = delete;
		const Scope& operator=(const Scope&) = delete;

		Scope(Scope&& other) { m_Instance = other.m_Instance; other.m_Instance = nullptr; }
		const Scope& operator=(Scope&& other) { m_Instance = other.m_Instance; other.m_Instance = nullptr; return *this; }

		template<typename T2, std::enable_if_t<std::is_convertible<T2*, T*>::type::value, bool> = true>
		Scope(Scope<T2>&& other) { m_Instance = other.m_Instance;other.m_Instance = nullptr; }
		template<typename T2, std::enable_if_t<std::is_convertible<T2*, T*>::type::value, bool> = true>
		Scope& operator=(Scope<T2>&& other) { m_Instance = other.m_Instance; other.m_Instance = nullptr; return *this; }

		void Release() { delete m_Instance; }

		T& operator*() const { return *m_Instance; }
		T* operator->() const { return m_Instance; }

		operator bool() { return m_Instance != nullptr; }
		bool operator==(const Scope& rhs) { return m_Instance == rhs.m_Instance; }
		bool operator!=(const Scope& rhs) { return !(*this == rhs); }

		template<typename... Args>
		constexpr static Scope Create(Args&&... args) { return Scope(new T(std::forward<Args>(args)...)); }

	private:
		explicit Scope(T* inst) { m_Instance = inst; }

	private:
		T* m_Instance = nullptr;

		template<typename T2>
		friend class Scope;
	};

}
