#pragma once

#define SK_TEXTURE_EDITOR_PANEL_NEW_UI 0

#include "Shark/Asset/AssetTypes.h"

#include "Panels/AssetEditorPanel.h"

namespace Shark {
	class Event;
	struct AssetMetaData;

	class RenderCommandBuffer;
	class Texture2D;
	class ImageView;
}

namespace Shark {

	class TextureEditorPanel : public EditorPanel
	{
	public:
		TextureEditorPanel(const std::string& panelName, const AssetMetaData& metadata);
		~TextureEditorPanel();

		virtual void DockWindow(ImGuiID dockspace) override;
		virtual void OnImGuiRender(bool& showWindow) override;
		virtual void OnEvent(Event& event) override;

	private:
		void SetAsset(const AssetMetaData& metadata);
		void UI_DrawSettings();
		void CreateImageViews();

	private:
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
