#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Scene/Components/CoreComponents.h"

#include "Shark/UI/TextFilter.h"

#include "Panel.h"

namespace Shark {
	class ProjectConfig;
	class Event;
	class KeyPressedEvent;

	class Scene;
	class Entity;
}

struct ImGuiMultiSelectIO;

namespace Shark {

	class SceneHierarchyPanel : public Panel
	{
	public:
		SceneHierarchyPanel(Ref<Scene> scene = nullptr, bool isWindow = true);

		virtual void OnImGuiRender(bool& shown) override;
		virtual void OnEvent(Event& event) override;

		virtual void OnProjectChanged(const Ref<ProjectConfig>& projectConfig) override;
		virtual void SetContext(const Ref<Scene>& scene) override;
		Ref<Scene> GetContext() const { return m_Context; }

		template<typename Func> // void(Entity)
		void RegisterSelectionChangedCallback(const Func& func) { m_SelectionChangedCallback = func; }

		template<typename TFunc>
		void RegisterEntityCreatedCallback(const TFunc& callback) { m_EntityCreatedCallback = callback; }
		template<typename TFunc>
		void RegisterEntityDestroyedCallback(const TFunc& callback) { m_EntityDestoyedCallback = callback; }

		template<typename TFunc> // void(Entity)
		void RegisterSnapToEditorCameraCallback(const TFunc& callback) { m_SnapToEditorCameraCallback = callback; }

		static const char* GetStaticID() { return "SceneHierarchyPanel"; }
		virtual const char* GetPanelID() const override { return GetStaticID(); }

	private:
		bool OnKeyPressedEvent(KeyPressedEvent& event);
		void HandleSelectionRequests(ImGuiMultiSelectIO* selectionIO, bool isBegin);

		void DrawEntityNode(Entity entity, uint32_t& index, const UI::TextFilter& searchFilter);
	public:
		void DrawEntityProperties(const std::vector<Entity>& entities);
	private:
		void DrawCreateEntityMenu(Entity parent);

		bool SearchTagRecursive(Entity entity, const UI::TextFilter& filter, uint32_t maxSearchDepth, uint32_t currentDepth = 0);

		template<typename Comp, typename UIFunction>
		void DrawComponet(Entity entity, const char* lable, UIFunction func);

		template<typename Comp, typename UIFunction>
		void DrawComponetMultiSelect(const std::vector<Entity>& entities, const char* lable, UIFunction func);

	private:
		bool m_IsWindow = true;
		Ref<Scene> m_Context;

		bool m_TransformInWorldSpace = false;
		bool m_HierarchyFocused = false;
		bool m_PropertiesFocused = false;

		std::function<void(Entity)> m_SelectionChangedCallback;
		std::function<void(Entity)> m_SnapToEditorCameraCallback;
		std::function<void(Entity)> m_EntityCreatedCallback;
		std::function<void(Entity)> m_EntityDestoyedCallback;

		struct ComponentBinding
		{
			std::string_view Name;
			void(*AddComponent)(Entity);
			bool(*HasComponent)(Entity);
		};
		std::vector<ComponentBinding> m_Components;

		char m_SearchComponentBuffer[260]{};
		UI::TextFilter m_ComponentFilter;

		bool m_ScriptFound = false;

		// #TODO use Scene to store copies
		bool m_HasTransformCopy;
		TransformComponent m_TransformCopy;

		bool m_DeleteSelected = false;
		bool m_DeleteChildren = true;

		bool m_ActivateSerach = false;
		UI::TextFilter m_SearchFilter;
		UI::TextFilter m_AnimationSearchFilter;

		struct RangeSelectRequest
		{
			uint32_t First;
			uint32_t Last;
			bool Select;
			bool ApplyRequest = false;
		} m_RangeSelectRequest;
	};

}