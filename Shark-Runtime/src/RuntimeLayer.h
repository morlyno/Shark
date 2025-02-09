#pragma once

#include "Shark/Core/Project.h"
#include "Shark/Layer/Layer.h"
#include "Shark/Scene/Scene.h"
#include "Shark/Render/SceneRenderer.h"

#include "Shark/Event/WindowEvent.h"
#include "Shark/Event/KeyEvent.h"

namespace Shark {

	class RuntimeLayer : public Layer
	{
	public:
		RuntimeLayer(const std::filesystem::path& projectFile);
		~RuntimeLayer();

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnUpdate(TimeStep ts) override;
		virtual void OnEvent(Event& event) override;

	private:
		bool OnWindowResizedEvent(WindowResizeEvent& event);
		bool OnKeyPressedEvent(KeyPressedEvent& event);

		Ref<ProjectConfig> LoadProject(const std::filesystem::path& projectFile);
	private:
		std::filesystem::path m_ProjectFile;

		Ref<Scene> m_Scene;
		Ref<SceneRenderer> m_Renderer;
		Ref<RenderCommandBuffer> m_CommandBuffer;
		Ref<RenderPass> m_PresentPass;
	};

}
