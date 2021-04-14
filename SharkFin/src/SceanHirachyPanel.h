#pragma once

#include <Shark/Core/Base.h>
#include <Shark/Scean/Scean.h>
#include <Shark/Scean/Entity.h>
#include <Shark/Render/Texture.h>

namespace Shark {

	class SceanHirachyPanel
	{
	public:
		SceanHirachyPanel() = default;
		SceanHirachyPanel(Ref<Scean> context);
		void SetContext(Ref<Scean> context);
		Ref<Scean> GetContext() const { return m_Context; }

		Entity GetSelectedEntity() const { return m_SelectedEntity; }
		void SetSelectedEntity(Entity selectedentity) { m_SelectedEntity = selectedentity; }

		void OnImGuiRender();

	private:
		void DrawEntityNode(Entity entity);
		void DrawEntityProperties(Entity entity);
	private:
		Ref<Scean> m_Context;
		Entity m_SelectedEntity;

		int m_SelectedProjectionIndex = -1;
		static constexpr const char* m_ProjectionItems[2] = { "Perspective", "Orthographic" };
		Ref<Texture2D> m_ImGuiNoTextureSelectedTexture = Texture2D::Create({}, 1, 1, 0x050505FF);
	};

}