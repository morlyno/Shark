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

		enum class MessageLevel
		{
			None = 0,
			Info,
			Warn,
			Error
		};
	public:
		EditorConsolePanel(const char* panelName);
		~EditorConsolePanel();

		virtual void OnImGuiRender(bool& shown) override;

		void Clear();
		bool ClearOnPlay() const { return m_ClearOnPlay; }

		static void PushMessage(Log::Level level, const std::string& time, const std::string& message);

	private:
		virtual void OnScenePlay() override;

	private:
		void PushMessage(MessageLevel level, const std::string& time, const std::string& message);

		void DrawMessages();
		void DrawMenuBar();
		void DrawMessageInspector();

		struct ConsoleMessage;
		bool Filter(const ConsoleMessage& message);
		RenderID GetIconRenderID(MessageLevel level) const;

		FilterFlag::Type LogLevelToFilterFlag(MessageLevel level);
		void SetFilter(FilterFlag::Type flag, bool enabled);
		void Refilter();
		bool AssureFilter();

		ImVec4 MessageColor(MessageLevel level) const;

	private:
		// Currently only one Console
		static EditorConsolePanel* s_Instance;

		struct ConsoleMessage
		{
			static constexpr size_t MaxFiendlyMessageLength = 300;

			MessageLevel Level;
			std::string Time;
			std::string FriendlyMessage;
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

	};

}
