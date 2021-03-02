#pragma once

#include <Shark/Core/Base.h>
#include <Shark/Scean/Scean.h>
#include <Shark/Scean/Entity.h>

namespace Shark {

	class SceanHirachyPanel
	{
	public:
		SceanHirachyPanel() = default;
		SceanHirachyPanel(Ref<Scean> context);
		void SetContext(Ref<Scean> context);
		Ref<Scean> GetContext() const { return m_Context; }

		void OnImGuiRender();

	private:
		void DrawEntityNode(Entity entity);
		void DrawEntityProperties(Entity entity);
	private:
		Ref<Scean> m_Context;
		Entity m_SelectedEntity;

		int m_SelectedProjectionIndex = -1;
		static constexpr const char* m_ProjectionItems[2] = { "Perspective", "Orthographic" };
	};

}