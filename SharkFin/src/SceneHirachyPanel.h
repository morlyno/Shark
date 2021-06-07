#pragma once

#include <Shark/Core/Base.h>
#include <Shark/Scene/Scene.h>
#include <Shark/Scene/Entity.h>
#include <Shark/Render/Texture.h>
#include <Shark/Event/Event.h>
#include <Shark/Event/ApplicationEvent.h>
#include <Shark/Render/Material.h>

namespace Shark {

	class SceneHirachyPanel
	{
	private:
		struct MaterialEditData
		{
			bool Active = false;
			bool Finished = false;
			bool Changed = false;
			Ref<Material> Material = nullptr;
			Ref<Shaders> MaterialShader = nullptr;
			Entity Entity = {};
			bool OpenWindow = false;
		};
	public:
		SceneHirachyPanel() = default;
		SceneHirachyPanel(const Ref<Scene>& context);
		void SetContext(const Ref<Scene>& context);
		const Ref<Scene>& GetContext() const { return m_Context; }

		void ShowPanel(bool show) { m_ShowPanel = show; }
		bool IsShowen() const { return m_ShowPanel; }

		Entity GetSelectedEntity() const { return m_SelectedEntity; }

		void SetScenePlaying(bool playing) { m_ScenePlaying = playing; }

		void OnImGuiRender();

		void OnEvent(Event& event);

	private:
		void DrawEntityNode(Entity entity);
		void DrawEntityProperties(Entity entity);
		void DrawMaterial(Entity entity);

		bool OnSelectionChanged(SelectionChangedEvent& event);
	private:
		bool m_ShowPanel = true;

		Ref<Scene> m_Context;
		Entity m_SelectedEntity;

		bool m_ScenePlaying = false;

		int m_SelectedProjectionIndex = -1;
		static constexpr const char* m_ProjectionItems[2] = { "Perspective", "Orthographic" };

		MaterialEditData m_EditData;
	};

}