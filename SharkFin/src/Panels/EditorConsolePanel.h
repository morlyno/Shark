#pragma once

#include "Panel.h"

#include <imgui.h>

namespace Shark {

	class EditorConsolePanel : public Panel
	{
	public:
		EditorConsolePanel(const std::string& panelName);
		~EditorConsolePanel();

		virtual void OnImGuiRender(bool& shown) override;

		void Clear();
		bool ClearOnPlay() const { return m_ClearOnPlay; }

	private:
		virtual void OnScenePlay() override;

	private:
		void PushMessage(ConsoleSinkMessage&& message);

		void DrawMessages();
		void DrawMenuBar();
		void DrawMessageInspector();

		ImU32 GetMessageLevelColor(LogLevel level) const;

	private:
		struct Message
		{
			static constexpr size_t MaxFiendlyMessageLength = 300;

			LogLevel Level;
			std::string Time;
			std::string Message;
			std::string_view FriendlyMessage;
		};

		std::vector<Message> m_Messages;
		uint32_t m_MaxMessages = 10000;

		bool m_ClearOnPlay = false;

		bool m_ShowMessageInspector = false;
		Message m_InspectorMessage;

	};

}
