#pragma once

#include "Shark/Render/Texture.h"

#include "Shark/Editor/Panel.h"

#include <imgui.h>
#include "Shark/Math/Math.h"

namespace Shark {

	class EditorConsolePanel : public Panel
	{
	private:
		struct FilterFlag
		{
			enum Type : uint16_t
			{
				None = 0,
				Info = BIT(0),
				Warn = BIT(1),
				Error = BIT(2),

				All = Info | Warn | Error
			};

			using Flags = uint16_t;
		};
	public:
		EditorConsolePanel();
		~EditorConsolePanel();

		virtual void OnImGuiRender(bool& shown) override;
		virtual void OnEvent(Event& event) override;

		void Clear();
		bool ClearOnPlay() const { return m_ClearOnPlay; }

		void LogMessage(Log::Level level, const std::string& message);
		static void LogMessage(Log::Level level, const std::string& message, uint32_t console);

	private:
		void DrawMessages();
		void DrawMenuBar();
		void DrawMessageInspector();

		struct ConsoleMessage;
		bool Filter(const ConsoleMessage& message);
		RenderID GetIconRenderID(Log::Level level);

		FilterFlag::Type LogLevelToFilterFlag(Log::Level level);
		void SetFilter(FilterFlag::Type flag, bool enabled);
		void Refilter();
		bool AssureFilter();

	private:
		// Currently only one Console
		static EditorConsolePanel* s_Instance;

		struct ConsoleMessage
		{
			Log::Level Level;
			std::string Message;
		};

		std::vector<ConsoleMessage> m_Messages;
		std::vector<uint32_t> m_Filtered;
		uint32_t m_MaxMessages = 10000;
		FilterFlag::Flags m_ActiveFilters = FilterFlag::All;

		//bool m_InfoActive = true;
		//bool m_WarnActive = true;
		//bool m_ErrorActive = true;

		bool m_ClearOnPlay = false;

		bool m_ShowMessageInspector = false;
		ConsoleMessage m_InspectorMessage;

	private:
		Ref<Texture2D> m_InfoIcon;
		Ref<Texture2D> m_WarnIcon;
		Ref<Texture2D> m_ErrorIcon;

	};

}
