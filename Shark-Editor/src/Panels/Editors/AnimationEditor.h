#pragma once

#include "Panels/AssetEditorPanel.h"

namespace Shark {

	class AnimationEditor : public EditorPanel
	{
	public:
		AnimationEditor(const std::string& panelName, const AssetMetaData& metadata);

		virtual void OnImGuiRender(bool& shown, bool& destroy) override;

		virtual void DockWindow(ImGuiID dockspaceID) override;
		virtual void SetAsset(const AssetMetaData& metadata) override;
		virtual AssetHandle GetAsset() const override;

	private:
		AssetHandle m_Asset;
		bool m_AssetDirty = false;
		bool m_Loading = false;

		bool m_DockWindow = false;
		ImGuiID m_DockspaceID = 0;
	};

}
