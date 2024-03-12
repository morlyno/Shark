#include "skfpch.h"
#include "TextureEditorPanel.h"

#include "Shark/Core/Application.h"
#include "Shark/Scene/Components.h"
#include "Shark/Render/Renderer.h"
#include "Shark/Input/Input.h"

#include "Shark/UI/UI.h"
#include "Shark/Math/Math.h"

#include "Shark/Debug/Profiler.h"

namespace Shark {

	TextureEditorPanel::TextureEditorPanel(const std::string& panelName, const AssetMetaData& metadata)
		: EditorPanel(panelName)
	{
		SetAsset(metadata);
	}

	TextureEditorPanel::~TextureEditorPanel()
	{
	}

	void TextureEditorPanel::SetAsset(const AssetMetaData& metadata)
	{
		if (metadata.Type != AssetType::Texture)
			return;

		m_IsSharkTexture = metadata.FilePath.extension() == ".sktex";
		m_TextureHandle = metadata.Handle;
		m_SetupWindows = true;
		m_Views.clear();

		Ref<Texture2D> texture = AssetManager::GetAsset<Texture2D>(metadata.Handle);
		AssetHandle handle = AssetManager::CreateMemoryAsset<Texture2D>(texture->GetSpecification());
		m_EditTexture = AssetManager::GetAsset<Texture2D>(handle);
		Renderer::CopyImage(Renderer::GetCommandBuffer(), texture->GetImage(), m_EditTexture->GetImage());

		CreateImageViews();

		const TextureSpecification& specification = texture->GetSpecification();
		m_ImageFormat = ToString(specification.Format);
		m_GenerateMips = specification.GenerateMips;
		m_FilterMode = specification.Filter;
		m_WrapMode = specification.Wrap;
		m_MaxAnisotropy = specification.MaxAnisotropy;
	}

	void TextureEditorPanel::DockWindow(ImGuiID dockspace)
	{
		m_DockWindowID = dockspace;
		m_DockWindow = true;
	}

	void TextureEditorPanel::OnImGuiRender(bool& shown, bool& destroy)
	{
		SK_PROFILE_FUNCTION();

		if (!shown || !m_TextureHandle)
			return;

		if (!m_Active)
		{
			shown = false;
			destroy = true;
			return;
		}

		if (m_DockWindow)
		{
			ImGui::SetNextWindowDockID(m_DockWindowID, ImGuiCond_Always);
			m_DockWindow = false;
		}

		if (ImGui::Begin(m_PanelName.c_str(), &m_Active))
		{
			if (ImGui::BeginTable("TextureEditor", 2, ImGuiTableFlags_Resizable))
			{
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);

				ImVec2 imageSize;
				const ImVec2 maxSize = ImGui::GetContentRegionAvail();
				if (maxSize.x < maxSize.y)
				{
					imageSize.x = maxSize.x;
					imageSize.y = maxSize.x * m_EditTexture->GetAspectRatio();
				}
				else
				{
					imageSize.x = maxSize.y * m_EditTexture->GetVerticalAspectRatio();
					imageSize.y = maxSize.y;
				}

				UI::Image(m_Views[m_MipIndex], imageSize);

				ImGui::TableSetColumnIndex(1);
				UI_DrawSettings();

				ImGui::EndTable();
			}
		}
		ImGui::End();

	}

	void TextureEditorPanel::UI_DrawSettings()
	{
		SK_PROFILE_FUNCTION();

		const auto& capabilities = Renderer::GetCapabilities();

		if (!m_IsSharkTexture)
			ImGui::Text("Editor is disabled because the texture is not a Shark Texture (.sktex)");

		UI::ScopedDisabled disabled(!m_IsSharkTexture);

		bool changed = false;

		UI::BeginControlsGrid();

		UI::Property("Format", m_ImageFormat);
		UI::Control("Generate Mipmap", m_GenerateMips);

		{
			UI::ScopedDisabled disabled(!m_GenerateMips);
			UI::ControlCustom("Mip", [this]()
			{
				ImGui::SetNextItemWidth(-1.0);
				UI::SliderScalar("#mip", ImGuiDataType_U32, m_MipIndex, 0, m_Views.size() - 1);
			});
		}

		changed |= UI::ControlCombo("Filter", (uint16_t&)m_FilterMode, s_FilterItems, std::size(s_FilterItems));
		changed |= UI::ControlCombo("Wrap", (uint16_t&)m_WrapMode, s_WrapItems, std::size(s_WrapItems));
		changed |= UI::Control("Max Anisotropy", m_MaxAnisotropy, 0.05f, 0, capabilities.MaxAnisotropy);

		UI::EndControls();

		ImGui::Separator();

		if (changed)
		{
			TextureSpecification& specification = m_EditTexture->GetSpecification();
			specification.Filter = m_FilterMode;
			specification.Wrap = m_WrapMode;
			specification.MaxAnisotropy = m_MaxAnisotropy;
			m_EditTexture->RT_Invalidate();

			Ref<Texture2D> sourceTexture = AssetManager::GetAsset<Texture2D>(m_TextureHandle);
			Renderer::RT_CopyImage(Renderer::GetCommandBuffer(), sourceTexture->GetImage(), m_EditTexture->GetImage());
			CreateImageViews();
		}

		if (m_GenerateMips != m_EditTexture->GetSpecification().GenerateMips)
		{
			m_EditTexture->GetSpecification().GenerateMips = m_GenerateMips;
			m_EditTexture->RT_Invalidate();
			CreateImageViews();
		}

		if (ImGui::Button("Reset"))
		{
			Ref<Texture2D> sourceTexture = AssetManager::GetAsset<Texture2D>(m_TextureHandle);
			const TextureSpecification& sourceSpec = sourceTexture->GetSpecification();

			if (m_GenerateMips != sourceSpec.GenerateMips || m_FilterMode != sourceSpec.Filter || m_WrapMode != sourceSpec.Wrap || m_MaxAnisotropy != sourceSpec.MaxAnisotropy)
			{
				m_GenerateMips = sourceSpec.GenerateMips;
				m_FilterMode = sourceSpec.Filter;
				m_WrapMode = sourceSpec.Wrap;
				m_MaxAnisotropy = sourceSpec.MaxAnisotropy;

				TextureSpecification& textureSpecification = m_EditTexture->GetSpecification();
				textureSpecification.GenerateMips = m_GenerateMips;
				textureSpecification.Filter = m_FilterMode;
				textureSpecification.Wrap = m_WrapMode;
				textureSpecification.MaxAnisotropy = m_MaxAnisotropy;
				m_EditTexture->RT_Invalidate();
				CreateImageViews();
			}
		}

		ImGui::SameLine();

		if (ImGui::Button("Save"))
		{
			Ref<Texture2D> texture = AssetManager::GetAsset<Texture2D>(m_TextureHandle);
			TextureSpecification& specification = texture->GetSpecification();
			specification.GenerateMips = m_GenerateMips;
			specification.Filter = m_FilterMode;
			specification.Wrap = m_WrapMode;
			specification.MaxAnisotropy = m_MaxAnisotropy;
			texture->Invalidate();
			Renderer::CopyImage(Renderer::GetCommandBuffer(), m_EditTexture->GetImage(), texture->GetImage());
			Project::GetActiveEditorAssetManager()->SaveAsset(m_TextureHandle);
			Project::GetActiveEditorAssetManager()->ReloadAsset(m_TextureHandle);
		}
	}

	void TextureEditorPanel::CreateImageViews()
	{
		m_MipIndex = 0;
		m_Views.clear();
		Ref<Image2D> image = m_EditTexture->GetImage();
		for (uint32_t i = 0; i < image->GetSpecification().MipLevels - 1; i++)
		{
			Ref<ImageView> view = ImageView::Create(image, i);
			m_Views.push_back(view);
		}
	}

}
