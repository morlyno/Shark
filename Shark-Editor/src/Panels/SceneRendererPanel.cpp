#include "SceneRendererPanel.h"

#include "Shark/Core/Application.h"
#include "Shark/UI/UICore.h"
#include "Shark/UI/Controls.h"

namespace Shark {

	SceneRendererPanel::SceneRendererPanel(const std::string& panelName)
		: Panel(panelName)
	{
	}

	void SceneRendererPanel::OnImGuiRender(bool& show)
	{
		if (!m_Renderer)
			return;

		if (ImGui::Begin(m_PanelName.c_str(), &show))
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

				if (ImGui::TreeNodeEx("Pipeline Statistics", UI::DefaultThinHeaderFlags | ImGuiTreeNodeFlags_DefaultOpen))
				{
					const auto& pipelineStats = m_Renderer->m_PipelineStatistics;
					ImGui::Text("Input Assembler Vertices: %llu", pipelineStats.InputAssemblerVertices);
					ImGui::Text("Input Assembler Primitives: %llu", pipelineStats.InputAssemblerPrimitives);
					ImGui::Text("Vertex Shader Invocations: %llu", pipelineStats.VertexShaderInvocations);
					ImGui::Text("Pixel Shader Invocations: %llu", pipelineStats.PixelShaderInvocations);
					ImGui::Text("Compute Shader Invocations: %llu", pipelineStats.ComputeShaderInvocations);
					ImGui::Text("Rasterizer Invocations: %llu", pipelineStats.RasterizerInvocations);
					ImGui::Text("Rasterizer Primitives: %llu", pipelineStats.RasterizerPrimitives);
					ImGui::TreePop();
				}

				ImGui::TreePop();
			}

			if (ImGui::TreeNodeEx("Settings", UI::DefaultHeaderFlags | ImGuiTreeNodeFlags_DefaultOpen))
			{
				UI::BeginControlsGrid();
				UI::Control("JumpFlood", m_Renderer->GetOptions().JumpFlood);
				UI::Control("Tonemap", m_Renderer->GetOptions().Tonemap);
				UI::Control("Gamma (2.2)", m_Renderer->GetOptions().GammaCorrect);
				UI::Control("Exposure", m_Renderer->GetOptions().Exposure);

				if (UI::ControlColor("Clear Color", m_ClearColor))
					m_Renderer->SetClearColor(m_ClearColor);
				UI::EndControlsGrid();

				ImGui::TreePop();
			}

			if (ImGui::TreeNodeEx("Jump Flood", UI::DefaultHeaderFlags | ImGuiTreeNodeFlags_DefaultOpen))
			{
				UI::BeginControlsGrid();
				UI::ControlColor("Outline Color", m_Renderer->m_OutlineColor);
				UI::ControlSlider("Outline Width", m_Renderer->m_OutlinePixelWidth, 0.0f, 50.0f);
				UI::ControlSlider("Steps", m_Renderer->m_JumpFloodSteps, 1, 10);
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
