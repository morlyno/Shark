#pragma once

#include "Panels/AssetEditorPanel.h"

namespace Shark {

	class SoundConfigEditor : public EditorPanel
	{
	public:
		SoundConfigEditor(const std::string& panelName, const AssetMetaData& metadata);

		virtual void OnImGuiRender(bool& shown, bool& destroy) override;

		virtual void DockWindow(ImGuiID dockspaceID) override;
		virtual void SetAsset(const AssetMetaData& metadata) override;
		virtual AssetHandle GetAsset() const override;

	private:
		AssetHandle m_Asset;
		bool m_ConfigDirty = false;

		bool m_DockWindow = false;
		ImGuiID m_DockspaceID = 0;

	};

}
