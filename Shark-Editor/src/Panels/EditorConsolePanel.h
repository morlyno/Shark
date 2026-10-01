#pragma once

#include "Panel.h"

#include <imgui.h>

#include <spdlog/sinks/callback_sink.h>

namespace Shark {

	class EditorConsolePanel : public Panel
	{
	public:
		EditorConsolePanel();
		~EditorConsolePanel();

		virtual void OnImGuiRender(bool& shown) override;

		static const char* GetStaticID() { return "EditorConsolePanel"; }
		virtual const char* GetPanelID() const override { return GetStaticID(); }

		void Clear();
		bool ClearOnPlay() const { return m_ClearOnPlay; }

	private:
		virtual void OnScenePlay() override;

	private:
		void PushMessage(const spdlog::details::log_msg& msg);

		void DrawMessages();
		void DrawMenuBar();
		void DrawMessageInspector();

		ImU32 GetMessageLevelColor(Log::Level level) const;

	private:
		struct Message
		{
			static constexpr size_t MaxFiendlyMessageLength = 300;

			Log::Level Level;
			std::string Time;
			std::string Message;
			std::string_view FriendlyMessage;
		};

		std::shared_ptr<spdlog::sinks::callback_sink_st> m_Sink;

		std::vector<Message> m_Messages;
		uint32_t m_MaxMessages = 10000;

		bool m_ClearOnPlay = false;
		bool m_BringToFront = false;

		bool m_ShowMessageInspector = false;
		Message m_InspectorMessage;

	};

}
