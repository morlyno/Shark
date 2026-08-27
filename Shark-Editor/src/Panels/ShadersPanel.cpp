#include "ShadersPanel.h"

#include "Shark/Render/Renderer.h"
#include "Shark/Render/Shader.h"

#include "Shark/UI/UICore.h"
#include "Shark/UI/Widgets.h"

namespace Shark {

	ShadersPanel::ShadersPanel()
	{
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
				{
					SK_NOT_IMPLEMENTED();
					// #Renderer #Disabled reload shader
					//shader->Reload(true, m_DisableOptimization);
				}
			}

			UI::Widgets::Search(m_Filter);

			ImGui::Separator();

			if (ImGui::BeginTable("shaders_table", 2, ImGuiTableFlags_Resizable))
			{
				for (const auto& [key, shader] : library->GetShadersMap())
				{
					if (!m_Filter.PassesFilter(key))
						continue;

					UI::ScopedID id(key);

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::Text(key.c_str());

					ImGui::TableSetColumnIndex(1);
					if (ImGui::Button("Reload"))
					{
						SK_NOT_IMPLEMENTED();
						// #Renderer #Disabled reload shader
						//shader->Reload(true, m_DisableOptimization);
					}
				}
				ImGui::EndTable();
			}
		}
		ImGui::End();

	}

}
