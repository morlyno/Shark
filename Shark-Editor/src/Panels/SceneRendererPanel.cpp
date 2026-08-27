#include "SceneRendererPanel.h"

#include "Shark/Core/Application.h"
#include "Shark/Core/Window.h"

#include "Shark/Render/SceneRenderer.h"

#include "Shark/UI/UICore.h"
#include "Shark/UI/Controls.h"

namespace Shark {

	SceneRendererPanel::SceneRendererPanel()
	{
	}

	void SceneRendererPanel::OnImGuiRender(bool& show)
	{
		if (!m_Renderer)
			return;

		if (ImGui::Begin(m_PanelName, &show))
		{
			ImGui::Text("Viewport Size: %u, %u", m_Renderer->m_Specification.Width, m_Renderer->m_Specification.Height);

			auto& window = Application::Get().GetWindow();
			bool vSync = window.VSyncEnabled();
			if (ImGui::Checkbox("VSync", &vSync))
				Application::Get().GetWindow().EnableVSync(vSync);

			if (ImGui::TreeNodeEx("Statistics", UI::DefaultHeaderFlags | ImGuiTreeNodeFlags_DefaultOpen))
			{
				const auto& stats = m_Renderer->GetStatisitcs();
				ImGui::Text(fmt::format(fmt::runtime("GPU Time: {}"), stats.GPUTime));
				ImGui::Text(fmt::format(fmt::runtime("Geometry Pass: {}"), stats.GeometryPass));
				ImGui::Text(fmt::format(fmt::runtime("Skybox Pass: {}"), stats.SkyboxPass));
				ImGui::Text(fmt::format(fmt::runtime("Composite Pass: {}"), stats.CompositePass));
				ImGui::Text(fmt::format(fmt::runtime("Jump Flood Pass: {}"), stats.JumpFloodPass));
				ImGui::TreePop();
			}

			if (ImGui::TreeNodeEx("Settings", UI::DefaultHeaderFlags | ImGuiTreeNodeFlags_DefaultOpen))
			{
				UI::BeginControlsGrid();
				UI::Control("JumpFlood", m_Renderer->GetOptions().JumpFlood);
				UI::Control("Tonemap", m_Renderer->GetOptions().Tonemap);
				UI::Control("Gamma (2.2)", m_Renderer->GetOptions().GammaCorrect);
				UI::Control("Exposure", m_Renderer->GetOptions().Exposure);

				if (UI::Control("Clear Color", m_ClearColor, UI::as_color))
					m_Renderer->SetClearColor(m_ClearColor);
				UI::EndControlsGrid();

				ImGui::TreePop();
			}

			if (ImGui::TreeNodeEx("Jump Flood", UI::DefaultHeaderFlags | ImGuiTreeNodeFlags_DefaultOpen))
			{
				UI::BeginControlsGrid();
				UI::Control("Outline Color", m_Renderer->m_OutlineColor, UI::as_color);
				UI::Control("Outline Width", m_Renderer->m_OutlinePixelWidth, { .Min = 0.0f, .Max = 50.0f, .Slider = true });
				UI::Control("Steps", m_Renderer->m_JumpFloodSteps, { .Min = 1, .Max = 10, .Slider = true });
				UI::EndControlsGrid();
				ImGui::TreePop();
			}

		}
		ImGui::End();
	}

	void SceneRendererPanel::SetRenderer(Ref<SceneRenderer> renderer)
	{
		m_Renderer = renderer;
		m_ClearColor = renderer->m_ClearColor;
	}

}
