#pragma once

namespace Shark {

	template<typename T>
	class Scope
	{
	public:
		Scope() = default;
		Scope(std::nullptr_t) {}
		Scope(Scope&& other) { m_Instance = other.m_Instance; other.m_Instance = nullptr; }
		const Scope& operator=(Scope&& other) { Release(); m_Instance = other.m_Instance; other.m_Instance = nullptr; return *this; }
		const Scope& operator=(std::nullptr_t) { Release(); return *this; }

		Scope(const Scope&) = delete;
		const Scope& operator=(const Scope&) = delete;

		~Scope() { Release(); }

		explicit Scope(T* inst) { m_Instance = inst; }

		template<typename T2, std::enable_if_t<std::is_convertible<T2*, T*>::type::value, int> = 0>
		Scope(Scope<T2>&& other) { Release(); m_Instance = other.m_Instance;other.m_Instance = nullptr; }
		template<typename T2, std::enable_if_t<std::is_convertible<T2*, T*>::type::value, int> = 0>
		const Scope& operator=(Scope<T2>&& other) { Release(); m_Instance = other.m_Instance; other.m_Instance = nullptr; return *this; }

		void Release() { skdelete m_Instance; m_Instance = nullptr; }

		T& operator*() const { return *m_Instance; }
		T* operator->() const { return m_Instance; }

		operator bool() const { return m_Instance != nullptr; }
		bool operator==(const Scope& rhs) const { return m_Instance == rhs.m_Instance; }
		bool operator!=(const Scope& rhs) const { return !(*this == rhs); }

		T* Raw() { return m_Instance; }
		const T* Raw() const { return m_Instance; }

		template<typename... Args>
		static Scope Create(Args&&... args) { return std::move(Scope(new(typeid(T).name()) T(std::forward<Args>(args)...))); }

	private:
		T* m_Instance = nullptr;

		template<typename T2> friend class Scope;

		template<typename T1, typename T2> friend Scope<T1> StaticCast(Scope<T2>& scope);
		template<typename T1, typename T2> friend Scope<T1> DynamicCast(Scope<T2>& scope);
	};

}
