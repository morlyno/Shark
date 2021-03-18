#pragma once

#include <Shark/Core/TimeStep.h>
#include <Shark/Scean/Entity.h>

namespace Shark {

	class NativeScript
	{
	public:
		virtual ~NativeScript() = default;

		virtual void OnCreate() = 0;
		virtual void OnDestroy() = 0;

		virtual void OnUpdate(TimeStep ts) = 0;

		void SetEntity(Entity entity) { m_Entity = entity; }

	protected:
		Entity m_Entity;
	};

}
