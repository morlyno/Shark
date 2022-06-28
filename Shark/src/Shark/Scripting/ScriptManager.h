#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/TimeStep.h"

#include "Shark/Scene/Entity.h"

#include <mono/metadata/object.h>

namespace Shark {

	using GCHandle = uint32_t;

	class Script
	{
	public:
		void OnCreate() const;
		void OnDestroy() const;
		void OnUpdate(TimeStep ts) const;
		void OnPhysicsUpdate(TimeStep fixedTimeStep) const;
		void OnUIRender() const;
		void OnCollishionBegin(UUID uuid, bool isScript) const;
		void OnCollishionEnd(UUID uuid, bool isScript) const;

		MonoObject* GetObject() const { return mono_gchandle_get_target(m_GCHandle); }

	public:
		GCHandle m_GCHandle = 0;
		MonoMethod* m_OnCreate = nullptr;
		MonoMethod* m_OnDestroy = nullptr;
		MonoMethod* m_OnUpdate = nullptr;
		MonoMethod* m_OnPhyicsUpdate = nullptr;
		MonoMethod* m_OnUIRender = nullptr;
		MonoMethod* m_OnCollishionBegin = nullptr;
		MonoMethod* m_OnCollishionEnd = nullptr;

		friend class ScriptManager;
	};

	class ScriptManager
	{
	public:
		static bool Instantiate(Entity entity, bool callOnCreate = false);
		static void Destroy(Entity entity, bool callOnDestroy = false);
		static void Cleanup();

		static bool Contains(UUID handle);
		static Script& GetScript(UUID handle);

		static const std::unordered_map<UUID, Script>& GetScripts();

	};

}
