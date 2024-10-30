#pragma once

#define SK_TEXTURE_EDITOR_PANEL_NEW_UI 0

#include "Shark/Event/Event.h"
#include "Shark/Event/KeyEvent.h"
#include "Shark/Core/TimeStep.h"
#include "Shark/Asset/Asset.h"
#include "Shark/Scene/Scene.h"
#include "Shark/Scene/Entity.h"
#include "Shark/Render/SceneRenderer.h"
#include "Shark/Render/EditorCamera.h"
#include "Shark/UI/UICore.h"

#include "Panels/AssetEditorPanel.h"

namespace Shark {

	class TextureEditorPanel : public EditorPanel
	{
	public:
		TextureEditorPanel(const std::string& panelName, const AssetMetaData& metadata);
		~TextureEditorPanel();

		virtual void DockWindow(ImGuiID dockspace) override;
		virtual void SetAsset(const AssetMetaData& metadata) override;
		virtual AssetHandle GetAsset() const override { return m_TextureHandle; }

		virtual void OnImGuiRender(bool& shown, bool& destroy) override;
		virtual void OnEvent(Event& event) override;

	private:
		void UI_DrawSettings();

		void CreateImageViews();

	private:
		bool m_Active = true;
		bool m_SetupWindows = false;

		bool m_Focused = false;

		bool m_DockWindow = false;
		ImGuiID m_DockWindowID = 0;

		bool m_IsSharkTexture = false;
		AssetHandle m_TextureHandle;

		Ref<RenderCommandBuffer> m_CommandBuffer;
		Ref<Texture2D> m_Texture;
		Ref<Texture2D> m_BackupTexture;

		std::vector<Ref<ImageView>> m_PerMipView;
		uint32_t m_MipIndex = 0;
	};

}
