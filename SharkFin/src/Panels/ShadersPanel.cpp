#include "skfpch.h"
#include "ShadersPanel.h"

#include "Shark/Render/Renderer.h"
#include "Shark/UI/UI.h"
#include "Shark/ImGui/TextFilter.h"

namespace Shark {

	ShadersPanel::ShadersPanel(const std::string& panelName)
		: Panel(panelName)
	{
		memset(m_SearchBuffer, 0, sizeof(m_SearchBuffer));
	}

	ShadersPanel::~ShadersPanel()
	{
	}

	void ShadersPanel::OnImGuiRender(bool& shown)
	{
		if (!shown || !Renderer::GetShaderLibrary())
			return;

		Ref<ShaderLibrary> library = Renderer::GetShaderLibrary();

		if (ImGui::Begin("Shaders", &shown))
		{
			ImGui::Checkbox("Disable Optimization", &m_DisableOptimization);

			if (ImGui::Button("Reload All"))
			{
				const ShaderLibrary::ShadersMap& shaders = library->GetShadersMap();
				for (const auto& [key, shader] : shaders)
					shader->Reload(true, m_DisableOptimization);
			}

			UI::Search(UI::GenerateID(), m_SearchBuffer, std::size(m_SearchBuffer));
			UI::TextFilter filter(m_SearchBuffer);

			ImGui::Separator();

			if (ImGui::BeginTable("shaders_table", 2, ImGuiTableFlags_Resizable))
			{
				for (const auto& [key, shader] : library->GetShadersMap())
				{
					if (!filter.PassFilter(key))
						continue;

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::Text(key.c_str());

					ImGui::TableSetColumnIndex(1);
					if (ImGui::Button("Realod"))
						shader->Reload(true, m_DisableOptimization);
				}
				ImGui::EndTable();
			}
		}
		ImGui::End();

	}

}
