#pragma once

namespace Shark {

	template<typename T>
	class Scope
	{
	public:
		using value_type = T;

	public:
		Scope() = default;
		Scope(std::nullptr_t) {}
		Scope(Scope&& other) { m_Instance = other.m_Instance; other.m_Instance = nullptr; }
		const Scope& operator=(Scope&& other) { Release(); m_Instance = other.m_Instance; other.m_Instance = nullptr; return *this; }
		const Scope& operator=(std::nullptr_t) { Release(); return *this; }

		Scope(const Scope&) = delete;
		const Scope& operator=(const Scope&) = delete;

		~Scope() { Release(); }

		Scope(T* inst) { m_Instance = inst; }

		template<typename T2, std::enable_if_t<std::is_convertible<T2*, T*>::type::value, int> = 0>
		Scope(Scope<T2>&& other) { Release(); m_Instance = other.m_Instance;other.m_Instance = nullptr; }
		template<typename T2, std::enable_if_t<std::is_convertible<T2*, T*>::type::value, int> = 0>
		const Scope& operator=(Scope<T2>&& other) { Release(); m_Instance = other.m_Instance; other.m_Instance = nullptr; return *this; }

		void Release() { skdelete m_Instance; m_Instance = nullptr; }

		T& operator*() const { return *m_Instance; }
		T* operator->() const { return m_Instance; }

		operator bool() const { return m_Instance != nullptr; }

		T* Raw() { return m_Instance; }
		const T* Raw() const { return m_Instance; }

		T* Detach() { return std::exchange(m_Instance, nullptr); }

		template<typename TAs> TAs* ViewAs() const { return static_cast<TAs*>(m_Instance); }

		template<typename... Args>
		static Scope Create(Args&&... args) { return std::move(Scope(new(typeid(T).name()) T(std::forward<Args>(args)...))); }

	private:
		T* m_Instance = nullptr;

		template<typename T2> friend class Scope;

		template<typename Left, typename Right>
		friend bool operator==(const Scope<Left>& left, const Scope<Right>& right);
		template<typename T>
		friend bool operator==(const Scope<T>& left, std::nullptr_t);
	};

	template<typename Left, typename Right>
	bool operator==(const Scope<Left>& left, const Scope<Right>& right)
	{
		return left.m_Instance == right.m_Instance;
	}

	template<typename Left, typename Right>
	bool operator!=(const Scope<Left>& left, const Scope<Right>& right)
	{
		return !(left == right);
	}

	template<typename T>
	bool operator==(const Scope<T>& left, std::nullptr_t)
	{
		return left.m_Instance == nullptr;
	}

	template<typename T>
	bool operator==(std::nullptr_t, const Scope<T>& right)
	{
		return right == nullptr;
	}

}
