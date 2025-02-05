#pragma once

#include "EditorLayer.h"

namespace Shark {

	class EditorApplication : public Application
	{
	public:
		EditorApplication(const ApplicationSpecification& specification, std::string_view startupProject);
		virtual ~EditorApplication();

		virtual void OnInitialize() override;
		virtual void OnShutdown() override;

	private:
		virtual ImGuiLayer& GetImGuiLayer() { return *m_ImGuiLayer; }
		virtual const ImGuiLayer& GetImGuiLayer() const { return *m_ImGuiLayer; }

	private:
		std::filesystem::path m_StartupProject{};

		// Not owned by this class
		EditorLayer* m_EditorLayer = nullptr;
		ImGuiLayer* m_ImGuiLayer = nullptr;
	};

}
