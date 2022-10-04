#include "skpch.h"
#include "EditorConsolePanel.h"

#include "Shark/UI/UI.h"
#include "Shark/UI/Theme.h"

#include "Shark/Input/Input.h"
#include "Shark/Event/ApplicationEvent.h"

#include "Shark/Editor/Icons.h"

#include "Shark/Debug/Instrumentor.h"

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

	EditorConsolePanel* EditorConsolePanel::s_Instance = nullptr;

	EditorConsolePanel::EditorConsolePanel(const char* panelName)
		: Panel(panelName)
	{
		SK_PROFILE_FUNCTION();

		SK_CORE_ASSERT(!s_Instance);
		s_Instance = this;

		m_Messages.reserve(m_MaxMessages);
		m_Filtered.reserve(m_MaxMessages);
	}

	EditorConsolePanel::~EditorConsolePanel()
	{
		SK_PROFILE_FUNCTION();

		s_Instance = nullptr;
	}

	void EditorConsolePanel::OnImGuiRender(bool& shown)
	{
		SK_PROFILE_FUNCTION();

		if (!shown)
			return;

		if (ImGui::Begin(PanelName, &shown, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
		{
			DrawMenuBar();
			DrawMessages();
		}
		ImGui::End();

		DrawMessageInspector();
	}

	void EditorConsolePanel::OnEvent(Event& event)
	{
		SK_PROFILE_FUNCTION();

		EventDispacher dispacher(event);
		dispacher.DispachEvent<ScenePlayEvent>([this](ScenePlayEvent& e) { if (m_ClearOnPlay) Clear(); return false; });
	}

	void EditorConsolePanel::Clear()
	{
		SK_PROFILE_FUNCTION();

		m_Messages.clear();
		m_ShowMessageInspector = false;

		m_Filtered.clear();
	}

	void EditorConsolePanel::PushMessage(LogLevelType level, const std::string& time, const std::string& message)
	{
		SK_CORE_ASSERT(s_Instance);
		switch (level)
		{
			case LogLevelType::Debug:
			case LogLevelType::Trace:
			case LogLevelType::Info:
				s_Instance->PushMessage(MessageLevel::Info, time, message);
				break;
			case LogLevelType::Warn:
				s_Instance->PushMessage(MessageLevel::Warn, time, message);
				break;
			case LogLevelType::Error:
			case LogLevelType::Critical:
				s_Instance->PushMessage(MessageLevel::Error, time, message);
				break;
		}

	}

	void EditorConsolePanel::PushMessage(MessageLevel level, const std::string& time, const std::string& message)
	{
		SK_PROFILE_FUNCTION();

		if (m_Messages.size() > m_MaxMessages)
		{
			const uint32_t invRatio = 5;
			const uint32_t eraseCount = m_MaxMessages / invRatio;
			m_Messages.erase(m_Messages.begin(), m_Messages.begin() + eraseCount);
			Refilter();
		}

		ConsoleMessage msg;
		msg.Level = level;
		msg.Time = time;
		SK_CORE_ASSERT(msg.Time.size() == 8);
		msg.Message = message;
		size_t count = message.find_first_of("\r\n");
		count = message.find_first_not_of("\r\n", count);
		count = message.find_first_of("\r\n", count);
		count = message.find_first_not_of("\r\n", count);
		//count = message.find('\n', count + 1);
		//SK_CORE_ASSERT(!(message.length() > ConsoleMessage::MaxFiendlyMessageLength && count > ConsoleMessage::MaxFiendlyMessageLength));
		msg.FriendlyMessage = std::string(message, 0, std::min({ message.length(), ConsoleMessage::MaxFiendlyMessageLength, count }));
		//msg.FriendlyMessage = message;

		m_Messages.push_back(msg);
		if (Filter(msg))
			m_Filtered.emplace_back((uint32_t)m_Messages.size() - 1);
	}

	void EditorConsolePanel::DrawMessages()
	{
		SK_PROFILE_FUNCTION();

		const ImGuiStyle& style = ImGui::GetStyle();
		const ImGuiWindow* consoleWindow = ImGui::GetCurrentWindow();

		UI::ScopedStyle childRounding(ImGuiStyleVar_ChildRounding, 0);

		const float twoLinesTextHeight = ImGui::GetFontSize() * 2.0f;
		const float imageSize = ImGui::GetFontSize() * 2.0f + style.FramePadding.y * 2.0f;

		if (ImGui::BeginTable("Messages", 3, ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_PadOuterX))
		{
			ImGuiWindow* window = ImGui::GetCurrentWindow();
			ImGuiTable* table = ImGui::GetCurrentTable();

			ImGui::PushTextWrapPos(window->WorkRect.Max.x - window->WorkRect.Min.x - style.WindowPadding.x);

			const ImVec2 textSize = ImGui::CalcTextSize("00:00:00");
			ImGui::TableSetupColumn("##levelImage", ImGuiTableColumnFlags_WidthFixed, imageSize);
			ImGui::TableSetupColumn("##time", ImGuiTableColumnFlags_WidthFixed, textSize.x);
			ImGui::TableSetupColumn("##message", ImGuiTableColumnFlags_WidthStretch);

			ImGuiListClipper clipper;
			clipper.Begin((int)m_Filtered.size());
			while (clipper.Step())
			{
				for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
				{
					const auto& msg = m_Messages[m_Filtered[i]];

					if (!Filter(msg))
						continue;

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(1);
					UI::MoveCursorY((imageSize - ImGui::GetFontSize()) * 0.5f);
					ImGui::TextColored(Theme::Colors::LogTimeColor, msg.Time.c_str());

					ImGui::TableSetColumnIndex(2);

					ImGui::AlignTextToFramePadding();
					utils::PrintTextClipped(msg.FriendlyMessage, MessageColor(msg.Level));

					if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && ImGui::IsWindowHovered())
					{
						ImRect r = window->InnerRect;
						r.Min.y = table->RowPosY1;
						r.Max.y = r.Min.y + imageSize;
						if (r.Contains(ImGui::GetMousePos()))
						{
							m_ShowMessageInspector = true;
							m_InspectorMessage = msg;
							ImGuiWindow* inspectorWindow = ImGui::FindWindowByName("Message Inspector");
							if (inspectorWindow)
								ImGui::FocusWindow(inspectorWindow);
						}
					}

					ImGui::TableSetColumnIndex(0);
					ImGui::Image(GetIconRenderID(msg.Level), { imageSize, imageSize });
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

		const ImGuiStyle& style = ImGui::GetStyle();

		//ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { style.WindowPadding.x * 0.5f, style.WindowPadding.y });
		//UI::MoveCursorX(-style.WindowPadding.x * 0.5f);
		ImGuiWindow* consoleWindow = ImGui::GetCurrentWindow();
		//consoleWindow->DC.CursorPos += style.WindowPadding;

		if (ImGui::Button("Clear"))
			Clear();

		ImGui::SameLine();
		ImGui::Text("Clear On Play");
		ImGui::SameLine();
		ImGui::Checkbox("##ClearOnPlay", &m_ClearOnPlay);

		const ImVec2 selectableSize = ImVec2(ImGui::GetFontSize() - style.FramePadding.x, ImGui::GetFontSize());

		const float imageSize = ImGui::GetFrameHeight() + style.WindowPadding.x * 1.5f/* + style.FramePadding.y * 2.0f*/;
		const float offsetFromEnd = imageSize * 3.0f + style.ItemSpacing.x * 2.0f;
		const float contentRegionX = ImGui::GetContentRegionAvail().x + style.FramePadding.x * 0.5f;
		const float offsetFromStart = contentRegionX - offsetFromEnd;

		const auto ImageButton = [](const char* strID, ImTextureID textureID, const ImVec2& size, bool selected) -> bool
		{
			ImGuiWindow* window = ImGui::GetCurrentWindow();
			const ImGuiStyle& style = ImGui::GetStyle();
			ImRect r = { window->DC.CursorPos, window->DC.CursorPos + size };
			const ImGuiID id = ImGui::GetID(strID);

			ImGui::ItemSize(r);
			if (!ImGui::ItemAdd(r, id))
				return false;

			bool hovered, held;
			bool pressed = ImGui::ButtonBehavior(r, id, &hovered, &held);

			ImU32 bgColor = 0x00000000;
			if (held)
				bgColor = ImGui::GetColorU32(ImGuiCol_ButtonActive);
			else if (hovered)
				bgColor = ImGui::GetColorU32(ImGuiCol_ButtonHovered);
			else if (selected)
				bgColor = 0xff303030;

			ImGui::RenderFrame(r.Min - ImVec2{ 2.0f, 2.0f }, r.Max + ImVec2{ 2.0f, 2.0f }, bgColor, true, 5.0f);

			ImDrawList* drawList = window->DrawList;
			drawList->AddImage(textureID, r.Min, r.Max);
			//drawList->AddImage(textureID, r.Min, r.Max);

			return pressed;
		};

		{
			consoleWindow->DC.CursorPosPrevLine.y -= style.WindowPadding.y * 0.75f;
			UI::ScopedStyle itemSpacing(ImGuiStyleVar_ItemSpacing, { style.ItemSpacing.x, style.ItemSpacing.y * 0.5f });

			ImGui::SameLine(offsetFromStart);
			const bool infoActive = m_ActiveFilters & FilterFlag::Info;
			if (ImageButton("InfoIcon", Icons::InfoIcon->GetViewID(), ImVec2(imageSize, imageSize), infoActive))
				SetFilter(FilterFlag::Info, !infoActive);

			ImGui::SameLine();
			const bool warnActive = m_ActiveFilters & FilterFlag::Warn;
			if (ImageButton("WarnIcon", Icons::WarnIcon->GetViewID(), ImVec2(imageSize, imageSize), warnActive))
				SetFilter(FilterFlag::Warn, !warnActive);

			ImGui::SameLine();
			const bool errorActive = m_ActiveFilters & FilterFlag::Error;
			if (ImageButton("ErrorIcon", Icons::ErrorIcon->GetViewID(), ImVec2(imageSize, imageSize), errorActive))
				SetFilter(FilterFlag::Error, !errorActive);
		}

		ImGui::Separator();

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

	bool EditorConsolePanel::Filter(const ConsoleMessage& message)
	{
		SK_PROFILE_FUNCTION();

		bool pass = false;

		switch (message.Level)
		{
			case MessageLevel::Info: pass = m_ActiveFilters & FilterFlag::Info; break;
			case MessageLevel::Warn: pass = m_ActiveFilters & FilterFlag::Warn; break;
			case MessageLevel::Error: pass = m_ActiveFilters & FilterFlag::Error; break;
		}

		return pass;
	}

	RenderID EditorConsolePanel::GetIconRenderID(MessageLevel level) const
	{
		SK_PROFILE_FUNCTION();

		switch (level)
		{
			case MessageLevel::Info: return Icons::InfoIcon->GetViewID();
			case MessageLevel::Warn: return Icons::WarnIcon->GetViewID();
			case MessageLevel::Error: return Icons::ErrorIcon->GetViewID();
		}
		return RenderID();
	}

	EditorConsolePanel::FilterFlag::Type EditorConsolePanel::LogLevelToFilterFlag(MessageLevel level)
	{
		switch (level)
		{
			case MessageLevel::Info:  return FilterFlag::Info;
			case MessageLevel::Warn:  return FilterFlag::Warn;
			case MessageLevel::Error: return FilterFlag::Error;
		}

		SK_CORE_ASSERT(false, "Invalid Love Level");
		return FilterFlag::None;
	}

	void EditorConsolePanel::SetFilter(FilterFlag::Type flag, bool enabled)
	{
		// Null or combied flags not allowed
		SK_CORE_ASSERT(Math::SingleBitSet(flag));
		if (!Math::SingleBitSet(flag))
		{
			Refilter();
			return;
		}

		// Nothing changed
		if ((bool)(m_ActiveFilters & flag) == enabled)
			return;

		if (enabled)
		{
			m_ActiveFilters |= flag;
			Refilter();

		}
		else
		{
			m_ActiveFilters &= ~flag;
			for (auto i = m_Filtered.begin(); i != m_Filtered.end();)
			{
				const auto& msg = m_Messages[*i];
				FilterFlag::Type messageFlag = LogLevelToFilterFlag(msg.Level);

				if (messageFlag == flag)
				{
					i = m_Filtered.erase(i);
					continue;
				}

				i++;
			}
		}

		SK_CORE_ASSERT(AssureFilter());
	}

	void EditorConsolePanel::Refilter()
	{
		m_Filtered.clear();
		for (uint32_t i = 0; i < m_Messages.size(); i++)
		{
			const auto& msg = m_Messages[i];
			if (!Filter(msg))
				continue;

			m_Filtered.emplace_back(i);
		}
		
	}

	bool EditorConsolePanel::AssureFilter()
	{
		if (!std::is_sorted(m_Filtered.begin(), m_Filtered.end()))
			return false;

		for (uint32_t i = 0; i < m_Messages.size(); i++)
		{
			const auto& msg = m_Messages[i];
			const bool hasIndex = std::binary_search(m_Filtered.begin(), m_Filtered.end(), i);
			if (Filter(msg) != hasIndex)
				return false;
		}
		return true;
	}

	ImVec4 EditorConsolePanel::MessageColor(MessageLevel level) const
	{
		switch (level)
		{
			case MessageLevel::Info:  return Theme::Colors::LogInfo;
			case MessageLevel::Warn:  return Theme::Colors::LogWarn;
			case MessageLevel::Error: return Theme::Colors::LogError;
		}

		SK_CORE_ASSERT(false, "Unkown Message Level");
		return ImGui::GetStyleColorVec4(ImGuiCol_Text);
	}

}
