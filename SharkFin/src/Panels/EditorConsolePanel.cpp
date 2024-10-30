#include "skfpch.h"
#include "EditorConsolePanel.h"

#include "Shark/Core/ConsoleSink.h"
#include "Shark/UI/UICore.h"
#include "Shark/UI/EditorResources.h"
#include "Shark/Debug/Profiler.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <fmt/printf.h>

namespace Shark {

	namespace utils {

		void PrintTextClipped(const std::string& text, const ImVec4& color)
		{
			const char* text_begin = text.c_str();
			const char* text_end = text_begin + text.size();

			ImGuiWindow* window = ImGui::GetCurrentWindow();
			const ImVec2 text_pos(window->DC.CursorPos.x, window->DC.CursorPos.y + window->DC.CurrLineTextBaseOffset);
			const float wrap_pos_x = window->DC.TextWrapPos;
			const bool wrap_enabled = (wrap_pos_x >= 0.0f);

			const float wrap_width = wrap_enabled ? ImGui::CalcWrapWidthForPos(window->DC.CursorPos, wrap_pos_x) : 0.0f;
			ImVec2 text_size = ImGui::CalcTextSize(text_begin, text_end, false, wrap_width);
			// add 1 so that characters like lowercase y don't get clipped
			text_size.y = ImGui::GetFontSize() * 2.0f + 1;

			ImRect bb(text_pos, text_pos + text_size);
			ImGui::ItemSize(text_size, 0.0f);
			if (ImGui::ItemAdd(bb, 0))
			{
				//ImGui::RenderTextClipped(bb.Min, bb.Max, text_begin, text_end, &text_size, ImVec2(0, 0), &bb);

				ImVec4 clipRect = bb.ToVec4();
				window->DrawList->AddText(
					GImGui->Font,
					GImGui->FontSize,
					text_pos,
					ImGui::ColorConvertFloat4ToU32(color),
					text_begin,
					text_end,
					wrap_width,
					&clipRect
				);
			}
		}

	}

	EditorConsolePanel::EditorConsolePanel(const std::string& panelName)
		: Panel(panelName)
	{
		m_Messages.reserve(m_MaxMessages);
		Log::SetConsoleSinkCallback([this](auto&& msg) { PushMessage(std::move(msg)); });
	}

	EditorConsolePanel::~EditorConsolePanel()
	{
	}

	void EditorConsolePanel::OnImGuiRender(bool& shown)
	{
		SK_PROFILE_FUNCTION();

		if (!shown)
			return;

		if (ImGui::Begin(m_PanelName.c_str(), &shown, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
		{
			Log::GetConsoleSink()->flush();

			DrawMenuBar();
			DrawMessages();
		}
		ImGui::End();

		DrawMessageInspector();
	}

	void EditorConsolePanel::Clear()
	{
		m_Messages.clear();
		m_ShowMessageInspector = false;
	}

	void EditorConsolePanel::OnScenePlay()
	{
		if (m_ClearOnPlay)
			Clear();
	}

	void EditorConsolePanel::PushMessage(ConsoleSinkMessage&& message)
	{
		SK_PROFILE_FUNCTION();

		if (m_Messages.size() > m_MaxMessages)
		{
			const uint32_t invRatio = 5;
			const uint32_t eraseCount = m_MaxMessages / invRatio;
			m_Messages.erase(m_Messages.begin(), m_Messages.begin() + eraseCount);
		}

		Message& msg = m_Messages.emplace_back();
		msg.Level = message.MessageLevel;
		msg.Time = fmt::format("{:%H:%M:%S}", std::chrono::floor<std::chrono::seconds>(message.Time));
		msg.Message = std::move(message.Message);

		const size_t messageLength = std::min({ msg.Message.length(), Message::MaxFiendlyMessageLength, msg.Message.find_first_of("\r\n") });
		msg.FriendlyMessage = std::string_view(msg.Message).substr(0, messageLength);
	}

	void EditorConsolePanel::DrawMessages()
	{
		SK_PROFILE_FUNCTION();

		const ImGuiStyle& style = ImGui::GetStyle();
		UI::ScopedStyle childRounding(ImGuiStyleVar_ChildRounding, 0);
		//UI::ScopedColor tableBorderColor(ImGuiCol_TableBorderLight, { 0.05, 0.05, 0.05, 1.0 });

		if (ImGui::BeginTable("Messages", 3, ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_Resizable | ImGuiTableFlags_PadOuterX))
		{
			ImGuiWindow* window = ImGui::GetCurrentWindow();
			ImGuiTable* table = ImGui::GetCurrentTable();

			ImGui::PushTextWrapPos(window->WorkRect.Max.x - window->WorkRect.Min.x - style.WindowPadding.x);

			const ImVec2 levelColumnSize = ImGui::CalcTextSize(" Critical ");
			const ImVec2 timeColumnSize = ImGui::CalcTextSize("00:00:00");
			ImGui::TableSetupColumn("Level", ImGuiTableColumnFlags_WidthFixed, levelColumnSize.x);
			ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, timeColumnSize.x);
			ImGui::TableSetupColumn("Message", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableHeadersRow();

			ImGuiListClipper clipper;
			clipper.Begin((int)m_Messages.size());
			while (clipper.Step())
			{
				for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
				{
					const auto& msg = m_Messages[i];

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);

					const ImVec2 cursorPosition = ImGui::GetCurrentWindowRead()->DC.CursorPos;

					std::string_view levelString = magic_enum::enum_name(msg.Level);
					ImGui::AlignTextToFramePadding();
					ImGui::Text(levelString);

					//ImVec2 levelTextSize = ImGui::CalcTextSize(levelString.data(), levelString.data() + levelString.size());
					//levelTextSize.y = 2.0f;
					//ImVec2 min = { cursorPosition.x, cursorPosition.y + ImGui::GetFontSize() + 2 };
					//ImVec2 max = min + levelTextSize;

					{
						ImDrawList* drawList = ImGui::GetWindowDrawList();
						ImRect underlineRect = UI::GetItemRect();
						underlineRect.Min.y = underlineRect.Max.y + 2;
						underlineRect.Max.y = underlineRect.Min.y + 2;

						drawList->AddRectFilled(underlineRect.Min, underlineRect.Max, GetMessageLevelColor(msg.Level));
					}

					ImGui::TableSetColumnIndex(1);
					ImGui::TextColored(ImColor(UI::Colors::Theme::LogTimeColor), msg.Time.c_str());

					ImGui::TableSetColumnIndex(2);
					ImGui::Text(msg.FriendlyMessage);

					if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && ImGui::IsWindowHovered())
					{
						ImRect r = window->InnerRect;
						r.Min.y = table->RowPosY1;
						r.Max.y = r.Min.y + ImGui::GetFontSize();
						if (r.Contains(ImGui::GetMousePos()))
						{
							m_ShowMessageInspector = true;
							m_InspectorMessage = msg;
							ImGui::SetWindowFocus("Message Inspector");
						}
					}
				}
			}

			ImGui::PopTextWrapPos();

			static bool AutoScroll = true;
			if (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
				ImGui::SetScrollHereY(1.0f);

			ImGui::EndTable();
		}
	}

	void EditorConsolePanel::DrawMenuBar()
	{
		SK_PROFILE_FUNCTION();

		if (ImGui::Button("Clear"))
			Clear();

		ImGui::SameLine();
		ImGui::Text("Clear On Play");
		ImGui::SameLine();
		ImGui::Checkbox("##ClearOnPlay", &m_ClearOnPlay);
	}

	void EditorConsolePanel::DrawMessageInspector()
	{
		SK_PROFILE_FUNCTION();

		if (m_ShowMessageInspector)
		{
			bool shown = true;
			ImGui::Begin("Message Inspector", &shown);
			auto& msg = m_InspectorMessage.Message;
			ImGui::TextUnformatted(msg.c_str());
			//ImGui::TextWrapped("%s", msg.c_str());
			ImGui::End();

			if (!shown)
				m_ShowMessageInspector = false;
		}
	}

	ImU32 EditorConsolePanel::GetMessageLevelColor(LogLevel level) const
	{
		switch (level)
		{
			case LogLevel::Trace: return ImGui::GetColorU32(UI::Colors::Theme::LogTrace);
			case LogLevel::Info: return ImGui::GetColorU32(UI::Colors::Theme::LogInfo);
			case LogLevel::Warn: return ImGui::GetColorU32(UI::Colors::Theme::LogWarn);
			case LogLevel::Error: return ImGui::GetColorU32(UI::Colors::Theme::LogError);
			case LogLevel::Critical: return ImGui::GetColorU32(UI::Colors::Theme::LogCritical);
		}
		return 0;
	}

}
