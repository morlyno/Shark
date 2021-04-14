#pragma once

#include "Shark/Scean/Scean.h"

namespace Shark {

	class SceanController
	{
	public:
		SceanController() = default;
		SceanController(const Ref<Scean>& scean);
		void SetScean(const Ref<Scean>& scean);

		void SaveState();
		void LoadState();

		void SetSavePath(const std::string& savepath) { m_SavePath = savepath; }
		const std::string GetSavePath() const { return m_SavePath; }

		bool Serialize() { return Serialize(m_SavePath); }
		bool Serialize(const std::string& filepath);
		bool Deserialize(const std::string& filepath);

		Ref<Scean> Get() const { return m_Active; }
		Ref<Scean> GetSaveState() const { return m_SaveState; }

		const Ref<Scean>& operator->() const { return m_Active; }
		const Ref<Scean>& operator*() const { return m_Active; }

		operator bool() const { return m_Active.operator bool(); }
	private:
		std::string m_SavePath;
		Ref<Scean> m_Active;
		Ref<Scean> m_SaveState;
	};

}
