#include "skfpch.h"
#include "EditorSettings.h"

namespace Shark {

	static EditorSettings* s_Instance = nullptr;

	void EditorSettings::Init()
	{
		SK_CORE_VERIFY(!s_Instance);
		s_Instance = sknew EditorSettings();
	}

	void EditorSettings::Shutdown()
	{
		skdelete s_Instance;
		s_Instance = nullptr;
	}

	EditorSettings& EditorSettings::Get()
	{
		return *s_Instance;
	}

}

