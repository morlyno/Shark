#include "skpch.h"
#include "SceanController.h"

#include "Shark/Scean/SceanSerialization.h"
#include "Shark/Utility/PlatformUtils.h"

namespace Shark {

	SceanController::SceanController(const Ref<Scean>& scean)
		: m_Active(scean), m_SaveState(Ref<Scean>::Create())
	{
		SaveState();
	}

	void SceanController::SetScean(const Ref<Scean>& scean)
	{
		m_Active = scean;
		SaveState();
	}

	void SceanController::SaveState()
	{
		*m_SaveState = *m_Active;
	}

	void SceanController::LoadState()
	{
		*m_Active = *m_SaveState;
	}

	bool SceanController::Serialize(const std::string& filepath)
	{
		SetSavePath(filepath);
		SceanSerializer serializer(m_Active);
		return serializer.Serialize(filepath);
	}

	bool SceanController::Deserialize(const std::string& filepath)
	{
		m_Active = Ref<Scean>::Create();
		SceanSerializer serializer(m_Active);
		bool succeeded = serializer.Deserialize(filepath);
		SaveState();
		return succeeded;
	}

}