#include "TextureEditorPanel.h"

#include "Shark/Event/ApplicationEvent.h"
#include "Shark/Asset/AssetManager.h"
#include "Shark/Scene/Components.h"
#include "Shark/Render/Renderer.h"

#include "Shark/UI/UICore.h"
#include "Shark/UI/Controls.h"

#include "Shark/Debug/Profiler.h"

namespace Shark {

	TextureEditorPanel::TextureEditorPanel(const std::string& panelName, const AssetMetaData& metadata)
		: EditorPanel(panelName)
	{
		m_CommandBuffer = RenderCommandBuffer::Create(fmt::format("TextureEditorPanel - {}", metadata.FilePath));

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
		m_PerMipView.clear();

		m_Texture = AssetManager::GetAsset<Texture2D>(metadata.Handle);

		TextureSpecification backupSpec = m_Texture->GetSpecification();
		backupSpec.DebugName = "TextureEditor-Backup";
		m_BackupTexture = Texture2D::Create(backupSpec);

		m_CommandBuffer->Begin();
		Renderer::CopyImage(m_CommandBuffer, m_Texture->GetImage(), m_BackupTexture->GetImage());
		m_CommandBuffer->End();
		m_CommandBuffer->Execute();
		
		CreateImageViews();
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
			m_Focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

			if (ImGui::BeginTable("TextureEditor", 2, ImGuiTableFlags_Resizable))
			{
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);

				ImVec2 imageSize;
				const ImVec2 maxSize = ImGui::GetContentRegionAvail();
				if (maxSize.x < maxSize.y)
				{
					imageSize.x = maxSize.x;
					imageSize.y = maxSize.x * m_Texture->GetAspectRatio();
				}
				else
				{
					imageSize.x = maxSize.y * m_Texture->GetVerticalAspectRatio();
					imageSize.y = maxSize.y;
				}

				UI::Image(m_PerMipView[m_MipIndex], imageSize);

				ImGui::TableSetColumnIndex(1);
				UI_DrawSettings();

				ImGui::EndTable();
			}
		}
		ImGui::End();

	}

	void TextureEditorPanel::OnEvent(Event& event)
	{
		EventDispacher dispacher(event);
		dispacher.DispachEvent<AssetReloadedEvent>([this](AssetReloadedEvent& e)
		{
			if (e.Asset == m_TextureHandle)
			{
				if (m_Focused)
					ImGui::ClearActiveID();

				SetAsset(Project::GetEditorAssetManager()->GetMetadata(m_TextureHandle));
			}
			return false;
		});
	}

	void TextureEditorPanel::UI_DrawSettings()
	{
		SK_PROFILE_FUNCTION();

		const auto& capabilities = Renderer::GetCapabilities();

		if (!m_IsSharkTexture)
		{
			ImGui::Text("Texture is not a Shark Texture (.sktex)\nChanges aren't saved");
		}

		const auto nativeTextureTooltip = [this]()
		{
			if (!m_IsSharkTexture && ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
				ImGui::SetTooltip("Changes can't be saved.\nTry converting this to a Shark Texture (*.sktex)");
		};

		bool changed = false;

		UI::BeginControlsGrid();

		TextureSpecification& spec = m_Texture->GetSpecification();
		changed |= UI::ControlCombo("Format", spec.Format);

		nativeTextureTooltip();
		bool genMipsChanged = UI::Control("Generate Mipmap", spec.GenerateMips);
		nativeTextureTooltip();

		{
			UI::ScopedDisabled disabled(!spec.GenerateMips);
			UI::ControlSlider("Mip", m_MipIndex, 0, (uint32_t)m_PerMipView.size() - 1);
			nativeTextureTooltip();
		}

		changed |= UI::ControlCombo("Filter", spec.Filter);
		nativeTextureTooltip();
		changed |= UI::ControlCombo("Wrap", spec.Wrap);
		nativeTextureTooltip();
		changed |= UI::Control("Max Anisotropy", spec.MaxAnisotropy, 0.05f, 0, capabilities.MaxAnisotropy);
		nativeTextureTooltip();

		UI::EndControlsGrid();

		ImGui::Separator();

		ImGui::Separator();

		if (changed)
		{
			m_Texture->RT_Invalidate();
			
			m_CommandBuffer->Begin();
			Renderer::CopyMip(m_CommandBuffer, m_BackupTexture->GetImage(), 0, m_Texture->GetImage(), 0);
			Renderer::GenerateMips(m_CommandBuffer, m_Texture->GetImage());
			m_CommandBuffer->End();
			m_CommandBuffer->Execute();

			CreateImageViews();
		}

		ImGui::BeginVertical("##buttonAlignVertical", ImGui::GetContentRegionAvail());
		ImGui::Spring();
		UI::Fonts::Push("Medium");
		ImGui::BeginHorizontal("##buttonAlignHorizontal", { ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight() });
		ImGui::Spring();

		if (ImGui::Button("Reset"))
		{
			auto& specification = m_Texture->GetSpecification();
			const auto& backupSpec = m_BackupTexture->GetSpecification();
			
			if (backupSpec.Format != specification.Format || backupSpec.GenerateMips != specification.GenerateMips ||
				backupSpec.Filter != specification.Filter || backupSpec.Wrap != specification.Wrap || backupSpec.MaxAnisotropy != specification.MaxAnisotropy)
			{
				specification = backupSpec;
				m_Texture->RT_Invalidate();
				
				m_CommandBuffer->Begin();
				Renderer::CopyImage(m_CommandBuffer, m_BackupTexture->GetImage(), m_Texture->GetImage());
				m_CommandBuffer->End();
				m_CommandBuffer->Execute();

				CreateImageViews();
			}
		}
		UI::Fonts::PushDefault();
		nativeTextureTooltip();
		UI::Fonts::Pop();

		if (ImGui::Button("Save") && m_IsSharkTexture)
		{
			Project::GetEditorAssetManager()->SaveAsset(m_TextureHandle);
		}
		UI::Fonts::PushDefault();
		nativeTextureTooltip();
		UI::Fonts::Pop();

		UI::Fonts::Pop();

		ImGui::EndHorizontal();
		ImGui::EndVertical();
	}

	void TextureEditorPanel::CreateImageViews()
	{
		const uint32_t mipLevels = m_Texture->GetMipLevels();
		m_PerMipView.resize(mipLevels);

		Ref<Image2D> image = m_Texture->GetImage();
		for (uint32_t i = 0; i < mipLevels; i++)
		{
			if (!m_PerMipView[i])
			{
				ImageViewSpecification specification;
				specification.Image = image;
				specification.MipSlice = i;
				m_PerMipView[i] = ImageView::Create(specification);
			}
			else
			{
				m_PerMipView[i]->RT_Invalidate();
			}
		}

		m_MipIndex = glm::clamp(m_MipIndex, 0u, mipLevels - 1);
	}

}
