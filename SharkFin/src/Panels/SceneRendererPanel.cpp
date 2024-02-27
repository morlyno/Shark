#include "skfpch.h"
#include "SceneRendererPanel.h"

#include "Shark/Core/Application.h"
#include "Shark/UI/UI.h"

namespace Shark {

	SceneRendererPanel::SceneRendererPanel(const std::string& panelName)
		: Panel(panelName)
	{
		m_VSync = Application::Get().GetWindow().VSyncEnabled();
	}

	void SceneRendererPanel::OnImGuiRender(bool& shown)
	{
		if (!shown || !m_Renderer)
			return;

		if (ImGui::Begin(m_PanelName.c_str()))
		{
			ImGui::Text("Viewport Size: %u, %u", m_Renderer->m_Specification.Width, m_Renderer->m_Specification.Height);
			if (ImGui::Checkbox("VSync", &m_VSync))
				Application::Get().GetWindow().EnableVSync(m_VSync);

			if (ImGui::TreeNodeEx("Statistics", UI::DefaultHeaderFlags | ImGuiTreeNodeFlags_DefaultOpen))
			{
				const auto& stats = m_Renderer->GetStatisitcs();
				UI::TextF("GPU Time: {}", stats.GPUTime);
				UI::TextF("Geometry Pass: {}", stats.GeometryPass);
				UI::TextF("Skybox Pass: {}", stats.SkyboxPass);

				if (ImGui::TreeNodeEx("Pipeline Statistics", UI::DefaultThinHeaderFlags | ImGuiTreeNodeFlags_DefaultOpen))
				{
					const auto& pipelineStats = m_Renderer->m_PipelineStatistics;
					ImGui::Text("Input Assembler Vertices: %llu", pipelineStats.InputAssemblerVertices);
					ImGui::Text("Input Assembler Primitives: %llu", pipelineStats.InputAssemblerPrimitives);
					ImGui::Text("Vertex Shader Invocations: %llu", pipelineStats.VertexShaderInvocations);
					ImGui::Text("Pixel Shader Invocations: %llu", pipelineStats.PixelShaderInvocations);
					ImGui::Text("Compute Shader Invocations: %llu", pipelineStats.ComputeShaderInvocations);
					ImGui::Text("Rasterizer Incovations: %llu", pipelineStats.RasterizerInvocations);
					ImGui::Text("Rasterizer Primitives: %llu", pipelineStats.RasterizerPrimitives);
					ImGui::TreePop();
				}

				ImGui::TreePop();
			}

			if (ImGui::TreeNodeEx("Statistics", UI::DefaultHeaderFlags | ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Checkbox("Skybox Pass", &m_Renderer->GetOptions().SkyboxPass);
				ImGui::TreePop();
			}
		}
		ImGui::End();
	}

}
