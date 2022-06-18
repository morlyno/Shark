#include "skpch.h"
#include "EditorConsolePanel.h"

#include "Shark/UI/UI.h"
#include "Shark/UI/Theme.h"

#include "Shark/Input/Input.h"
#include "Shark/Event/ApplicationEvent.h"

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

	EditorConsolePanel::EditorConsolePanel()
	{
		SK_PROFILE_FUNCTION();

		SK_CORE_ASSERT(!s_Instance);
		s_Instance = this;

		m_InfoIcon = Texture2D::Create("Resources/Console/Icon_Info.png");
		m_WarnIcon = Texture2D::Create("Resources/Console/Icon_Warn.png");
		m_ErrorIcon = Texture2D::Create("Resources/Console/Icon_Error.png");
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


		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		const bool open = ImGui::Begin("Console", &shown);
		ImGui::PopStyleVar();

		if (open)
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
		dispacher.DispachEvent<ScenePlayEvent>([this](ScenePlayEvent& e)
		{
			if (m_ClearOnPlay)
				Clear();
		});
	}

	void EditorConsolePanel::Clear()
	{
		SK_PROFILE_FUNCTION();

		m_Messages.clear();
		m_ShowMessageInspector = false;

		m_Filtered.clear();
	}

	void EditorConsolePanel::LogMessage(Log::Level level, const std::string& message)
	{
		SK_PROFILE_FUNCTION();

		if (level == Log::Level::Info || level == Log::Level::Warn || level == Log::Level::Error)
		{
			if (m_Messages.size() > m_MaxMessages)
			{
				m_Messages.shrink_to_fit();

				if (m_ActiveFilters == FilterFlag::All)
				{
					SK_CORE_ASSERT(m_Filtered.capacity() >= m_MaxMessages);
					m_Filtered.shrink_to_fit();
				}

				constexpr uint32_t invRatio = 5;
				const uint32_t eraseCount = m_MaxMessages / invRatio;
				m_Messages.erase(m_Messages.begin(), m_Messages.begin() + eraseCount);
				Refilter();
			}

			ConsoleMessage msg = { level, message };
			m_Messages.push_back(msg);
			if (Filter(msg))
				m_Filtered.emplace_back((uint32_t)m_Messages.size() - 1);

		}
	}

	void EditorConsolePanel::LogMessage(Log::Level level, const std::string& message, uint32_t console)
	{
		SK_PROFILE_FUNCTION();

		SK_CORE_ASSERT(console == 0);
		if (s_Instance)
		{
			s_Instance->LogMessage(level, message);
		}
	}

	void EditorConsolePanel::DrawMessages()
	{
		SK_PROFILE_FUNCTION();

		const ImGuiStyle& style = ImGui::GetStyle();

		const float twoLinesTextHeight = ImGui::GetFontSize() * 2.0f;
		const float imageSize = ImGui::GetFontSize() * 2.0f + style.FramePadding.y * 2.0f;

		if (ImGui::BeginTable("Messages", 2, ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg))
		{
			ImGuiWindow* window = ImGui::GetCurrentWindow();
			ImGuiTable* table = ImGui::GetCurrentTable();

			ImGui::Indent(style.WindowPadding.x);
			ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, imageSize + style.WindowPadding.x);

			ImGui::PushTextWrapPos(window->WorkRect.Max.x - window->WorkRect.Min.x - style.WindowPadding.x);

			ImGuiListClipper clipper;
			clipper.Begin((int)m_Filtered.size());
			while (clipper.Step())
			{
				for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
				{
					const auto& msg = m_Messages[m_Filtered[i]];

					if (!Filter(msg))
						continue;

					ImGui::TableNextRow(ImGuiTableRowFlags_None, imageSize);
					ImGui::TableSetColumnIndex(1);

					if (msg.Level == Log::Level::Critical)
						ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::ColorConvertFloat4ToU32(Theme::Colors::LogCriticalBg));

					ImGui::AlignTextToFramePadding();
					utils::PrintTextClipped(msg.Message, Theme::GetLogColor(msg.Level));

					if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
					{
						ImRect r = window->InnerRect;
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

		ImGuiWindow* consoleWindow = ImGui::GetCurrentWindow();
		ImDrawList* drawList = consoleWindow->DrawList;
		const ImGuiStyle& style = ImGui::GetStyle();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { style.WindowPadding.x * 0.5f, style.WindowPadding.y });
		consoleWindow->DC.CursorPos += style.WindowPadding;

		if (ImGui::Button("Clear"))
			Clear();

		ImGui::SameLine();
		ImGui::Text("Clear On Play");
		ImGui::SameLine();
		ImGui::Checkbox("##ClearOnPlay", &m_ClearOnPlay);

		const ImVec2 selectableSize = ImVec2(ImGui::GetFontSize() - style.FramePadding.x, ImGui::GetFontSize());

		const float imageSize = ImGui::GetFrameHeight() + style.WindowPadding.y * 1.5f;
		const float offsetFromEnd = imageSize * 3 + style.ItemSpacing.x * 2;
		const float contentRegionX = ImGui::GetContentRegionAvail().x - style.WindowPadding.x;
		const float offsetFromStart = contentRegionX - offsetFromEnd;

		const auto ImageButton = [](const char* strID, ImTextureID textureID, const ImVec2& size, bool selected) -> bool
		{
			ImGuiWindow* window = ImGui::GetCurrentWindow();
			ImRect r = { window->DC.CursorPos, window->DC.CursorPos + size };

			bool hovered, held;
			bool pressed = ImGui::ButtonBehavior(r, ImGui::GetID(strID), &hovered, &held);

			ImU32 bgColor = 0x00000000;
			if (held)
				bgColor = ImGui::GetColorU32(ImGuiCol_ButtonActive);
			else if (hovered)
				bgColor = ImGui::GetColorU32(ImGuiCol_ButtonHovered);
			else if (selected)
				bgColor = 0xff303030;

			ImDrawList* drawList = window->DrawList;
			drawList->AddRectFilled(r.Min, r.Max, bgColor, 5.0f);

			ImGui::Image(textureID, size);

			return pressed;
		};

		consoleWindow->DC.CursorPosPrevLine.y -= style.WindowPadding.y * 0.75f;

		ImGui::SameLine(offsetFromStart);
		const bool infoActive = m_ActiveFilters & FilterFlag::Info;
		if (ImageButton("InfoIcon", m_InfoIcon->GetViewID(), ImVec2(imageSize, imageSize), infoActive))
			SetFilter(FilterFlag::Info, !infoActive);

		ImGui::SameLine();
		const bool warnActive = m_ActiveFilters & FilterFlag::Warn;
		if (ImageButton("WarnIcon", m_WarnIcon->GetViewID(), ImVec2(imageSize, imageSize), warnActive))
			SetFilter(FilterFlag::Warn, !warnActive);

		ImGui::SameLine();
		const bool errorActive = m_ActiveFilters & FilterFlag::Error;
		if (ImageButton("ErrorIcon", m_ErrorIcon->GetViewID(), ImVec2(imageSize, imageSize), errorActive))
			SetFilter(FilterFlag::Error, !errorActive);

		consoleWindow->DC.CursorPos.y -= style.WindowPadding.y * 0.25f;
		consoleWindow->DC.CursorPosPrevLine.y -= style.WindowPadding.y * 0.25f;

		ImGui::PopStyleVar(); // WindowPadding

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
			ImGui::TextWrapped(msg.c_str());
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
			case Log::Level::Info: pass = m_ActiveFilters & FilterFlag::Info; break;
			case Log::Level::Warn: pass = m_ActiveFilters & FilterFlag::Warn; break;
			case Log::Level::Error: pass = m_ActiveFilters & FilterFlag::Error; break;
		}

		return pass;
	}

	RenderID EditorConsolePanel::GetIconRenderID(Log::Level level)
	{
		SK_PROFILE_FUNCTION();

		switch (level)
		{
			case Log::Level::Info: return m_InfoIcon->GetViewID();
			case Log::Level::Warn: return m_WarnIcon->GetViewID();
			case Log::Level::Error: return m_ErrorIcon->GetViewID();
		}
		return RenderID();
	}

	EditorConsolePanel::FilterFlag::Type EditorConsolePanel::LogLevelToFilterFlag(Log::Level level)
	{
		switch (level)
		{
			case Log::Level::Info:  return FilterFlag::Info;
			case Log::Level::Warn:  return FilterFlag::Warn;
			case Log::Level::Error: return FilterFlag::Error;
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

}
