#pragma once

#include <Shark/Core/TimeStep.h>
#include <Shark/Scean/Entity.h>

namespace Shark {

	class NativeScript
	{
	public:
		virtual ~NativeScript() = default;

		virtual void OnCreate() {};
		virtual void OnDestroy() {};

		virtual void OnUpdate(TimeStep ts) {};
		virtual void OnEvent(Event& event) {};

	public:
		Entity m_Entity;
		Weak<Scean> m_Scean;
	};

}
