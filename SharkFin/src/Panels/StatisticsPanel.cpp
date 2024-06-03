#include "skfpch.h"
#include "StatisticsPanel.h"

#include "Shark/Core/Application.h"
#include "Shark/Debug/Profiler.h"
#include "Shark/UI/UI.h"
#include "Shark/Utils/String.h"

namespace Shark {

	StatisticsPanel::StatisticsPanel(const std::string& panelName)
		: Panel(panelName)
	{
	}

	void StatisticsPanel::OnImGuiRender(bool& shown)
	{
		if (!shown)
			return;

		if (ImGui::Begin("Statistics", &shown))
		{
			if (ImGui::BeginTabBar("TabBar"))
			{
				UI_Memory();
				UI_Profiler();
				ImGui::EndTabBar();
			}
		}
		ImGui::End();
	}

	void StatisticsPanel::UI_Memory()
	{
		if (ImGui::BeginTabItem("Memory"))
		{
			UI::Text("Total allocated {}", String::BytesToString(Allocator::GetMemoryStats().TotalAllocated));
			UI::Text("Total freed {}", String::BytesToString(Allocator::GetMemoryStats().TotalFreed));
			UI::Text("Current Usage {}", String::BytesToString(Allocator::GetMemoryStats().CurrentUsage()));

			ImGui::Separator();

			static char SearchBuffer[250]{};
			UI::Search(UI::GenerateID(), SearchBuffer, (int)std::size(SearchBuffer));

			struct Entry
			{
				std::string Descriptor;
				bool IsFile = false;
				std::string Size;
				uint64_t ByteSize;
				bool operator>(const Entry& rhs) const { return ByteSize > rhs.ByteSize; }
			};

			std::vector<Entry> entries;

			for (const auto& [desc, size] : Allocator::GetAllocationStatsMap())
			{
				if (!String::Contains(desc, SearchBuffer, false))
					continue;

				auto& entry = entries.emplace_back();
				entry.Size = String::BytesToString(size);
				entry.ByteSize = size;

				std::string str = desc;
				if (str.find("class") != std::string::npos)
					String::RemovePrefix(str, 6);

				size_t i = str.find_last_of("\\/");
				if (i != std::string::npos)
				{
					str = str.substr(i + 1);
					entry.IsFile = true;
				}

				entry.Descriptor = str;
			}

			std::sort(entries.begin(), entries.end(), std::greater{});

			for (const auto& entry : entries)
			{
				UI::ScopedColorConditional color(ImGuiCol_Text, ImVec4(0.2f, 0.3f, 0.9f, 1.0f), entry.IsFile);
				ImGui::Text("%s %s", entry.Descriptor.c_str(), entry.Size.c_str());
			}

			ImGui::EndTabItem();
		}

	}

	void StatisticsPanel::UI_Profiler()
	{
		Application& app = Application::Get();
		PerformanceProfiler* profiler = app.GetSecondaryProfiler();

		if (ImGui::BeginTabItem("Profiler"))
		{
			if (profiler)
			{
				const auto& frameStorages = profiler->GetFrameStorage();

				m_FrameTimeAccumulator += app.GetFrameTime();
				m_CPUTimeAccumulator += app.GetCPUTime();
				m_GPUTimeAccumulator += app.GetGPUTime();
				m_ImGuiGPUTimeAccumulator += app.GetImGuiLayer().GetGPUTime();

				size_t index = 2;
				for (const auto& [descriptor, data] : frameStorages)
					m_ProfilerStatsAccumulator[(std::string)data.Descriptor] += data.Duration;

				if (++m_ProfilerSampleCount >= m_ProfilerSamples)
				{
					const auto sorter = [](const ProfilerEntry& lhs, const ProfilerEntry& rhs) -> bool { return lhs.Duration != rhs.Duration ? lhs.Duration > rhs.Duration : lhs.Descriptor > rhs.Descriptor; };

					m_ProfilerStats.clear();
					m_ProfilerStats.reserve(m_ProfilerStatsAccumulator.size());
					for (const auto& [descriptor, duration] : m_ProfilerStatsAccumulator)
					{
						ProfilerEntry entry = { descriptor, duration / (float)m_ProfilerSamples };
						const auto where = std::lower_bound(m_ProfilerStats.begin(), m_ProfilerStats.end(), entry, sorter);
						m_ProfilerStats.insert(where, entry);
					}

					m_FrameTime = m_FrameTimeAccumulator / (float)m_ProfilerSamples;
					m_CPUTime = m_CPUTimeAccumulator / (float)m_ProfilerSamples;
					m_GPUTime = m_GPUTimeAccumulator / (float)m_ProfilerSamples;
					m_ImGuiGPUTime = m_ImGuiGPUTimeAccumulator / (float)m_ProfilerSamples;

					m_FrameTimeAccumulator = 0;
					m_CPUTimeAccumulator = 0;
					m_GPUTimeAccumulator = 0;
					m_ImGuiGPUTimeAccumulator = 0;
					m_ProfilerStatsAccumulator.clear();
					m_ProfilerSampleCount = 0;
				}

				UI::BeginControls();
				UI::Control("Samples", m_ProfilerSamples);
				UI::EndControls();

				UI::BeginControlsGrid();
				UI::Property("Frame", m_FrameTime);
				UI::Property("CPU", m_CPUTime);
				UI::Property("GPU", m_GPUTime);
				UI::Property("ImGui GPU", m_ImGuiGPUTime);
				for (const auto& entry : m_ProfilerStats)
					UI::Property(entry.Descriptor, entry.Duration.ToString());
				UI::EndControlsGrid();
			}

			if (ImGui::TreeNodeEx("Physics", UI::DefaultThinHeaderFlags))
			{
				UI::BeginControlsGrid();
				const auto& profile = m_Scene->GetPhysicsScene().GetProfile();
				UI::Property("TimeStep", profile.TimeStep);
				UI::Property("Steps", profile.NumSteps);
				UI::Property("Step", profile.Step);
				UI::Property("Collide", profile.Collide);
				UI::Property("Solve", profile.Solve);
				UI::Property("SolveInit", profile.SolveInit);
				UI::Property("SolveVelocity", profile.SolveVelocity);
				UI::Property("SolvePosition", profile.SolvePosition);
				UI::Property("Broadphase", profile.Broadphase);
				UI::Property("SolveTOI", profile.SolveTOI);
				UI::EndControlsGrid();

				ImGui::TreePop();
			}

			ImGui::EndTabItem();
		}
	}

}
