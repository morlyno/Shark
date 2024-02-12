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
#include "Shark/UI/UI.h"

#include "Panels/AssetEditorPanel.h"

namespace Shark {

	class TextureEditorPanel : public EditorPanel
	{
	public:
		TextureEditorPanel(const std::string& panelName, const AssetMetaData& metadata);
		~TextureEditorPanel();

		void SetAsset(const AssetMetaData& metadata);
		void DockWindow(ImGuiID dockspace);

		virtual void OnImGuiRender(bool& shown, bool& destroy) override;

	private:
		void UI_DrawSettings();

		void CreateImageViews();

	private:
		bool m_Active = true;
		bool m_SetupWindows = false;

		AssetHandle m_TextureHandle;
		Ref<Texture2D> m_EditTexture;

		std::string m_ImageFormat;

		bool m_GenerateMips;
		FilterMode m_FilterMode;
		WrapMode m_WrapMode;
		uint32_t m_MaxAnisotropy;

		std::vector<Ref<ImageView>> m_Views;
		uint32_t m_MipIndex = 0;

		bool m_DockWindow = false;
		ImGuiID m_DockWindowID = 0;

		static constexpr std::string_view s_FilterItems[] = { "None", "Neares", "Linear", "Anisotropic"};
		static constexpr std::string_view s_WrapItems[] = { "None", "Repeat", "Clamp", "Mirror" };
	};

}
