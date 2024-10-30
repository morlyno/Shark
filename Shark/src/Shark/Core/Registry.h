#pragma once

namespace Shark {

	class Registry
	{
	public:
		bool IsEmpty() const;
		bool HasKey(std::string_view key) const;
		void RemoveKey(std::string_view key);

		const std::string& Get(std::string_view name, const std::string& defaultValue = "") const;
		float GetFloat(std::string_view name, float defaultValue = 0.0f) const;
		int GetInt(std::string_view name, int defaultValue = 0) const;

		void Set(std::string_view name, std::string_view value);
		void SetFloat(std::string_view name, float  value);
		void SetInt(std::string_view name, int value);

		auto& GetMap() { return m_Map; }
		const auto& GetMap() const { return m_Map; }

	private:
		std::map<std::string, std::string, std::less<>> m_Map;
	};

}
