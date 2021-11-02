#pragma once

#include <Shark/Core/TimeStep.h>
#include <Shark/Scene/Entity.h>

namespace Shark {

	class NativeScript
	{
	public:
		virtual ~NativeScript() = default;

		virtual void OnCreate() {}
		virtual void OnDestroy() {}

		virtual void OnUpdate(TimeStep ts) {}
		virtual void OnEvent(Event& event) {}

	public:
		Entity m_Entity;
		Weak<Scene> m_Scene;
	};

}
