#pragma once

#include "Panels/AssetEditorPanel.h"

namespace Shark {

	class AnimationEditor : public EditorPanel
	{
	public:
		AnimationEditor(const std::string& panelName, const AssetMetaData& metadata);

		virtual void DrawWindow(bool& showWindow) override;
		virtual ImGuiWindowFlags GetWindowFlags() const override;

	private:
		AssetHandle m_Asset;
		bool m_AssetDirty = false;
		bool m_Loading = false;
	};

}
