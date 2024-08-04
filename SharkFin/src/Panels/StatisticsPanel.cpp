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
				const auto& frameStorages = profiler->GetFrameData();
				bool entriesChanged = false;

				m_FrameTimeAccumulator += app.GetFrameTime();
				m_CPUTimeAccumulator += app.GetCPUTime();
				m_GPUTimeAccumulator += app.GetGPUTime();
				m_ImGuiGPUTimeAccumulator += app.GetImGuiLayer().GetGPUTime();

				size_t index = 2;
				for (const auto& [name, perFrameData] : frameStorages)
				{
					auto& entry = m_ProfilerStatsAccumulator[std::string(name)];
					entry.Time += perFrameData.Time;
					entry.Min = std::min(entry.Min, perFrameData.Time);
					entry.Max = std::max(entry.Max, perFrameData.Time);
					entry.Samples += perFrameData.Samples;
				}

				if (++m_ProfilerSampleCount >= m_ProfilerSamples)
				{
					const auto sorter = [](const ProfilerEntry& lhs, const ProfilerEntry& rhs) -> bool { return lhs.Name > rhs.Name; };

					m_ProfilerStats.clear();
					m_ProfilerStats.reserve(m_ProfilerStatsAccumulator.size());
					for (const auto& [name, accumulatorEntry] : m_ProfilerStatsAccumulator)
					{
						ProfilerEntry entry = {
							.Name = name,
							.Time = accumulatorEntry.Time / (float)m_ProfilerSamples,
							.Min = accumulatorEntry.Min,
							.Max = accumulatorEntry.Max,
							.AvgSamples = (float)accumulatorEntry.Samples / (float)m_ProfilerSamples,
						};
						m_ProfilerStats.emplace_back(entry);
						//const auto where = std::lower_bound(m_ProfilerStats.begin(), m_ProfilerStats.end(), entry, sorter);
						//m_ProfilerStats.insert(where, entry);
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
					entriesChanged = true;
				}

				UI::BeginControls();
				UI::Control("Samples", m_ProfilerSamples);
				UI::EndControls();

				UI::BeginControlsGrid();
				UI::Property("Frame", m_FrameTime);
				UI::Property("CPU", m_CPUTime);
				UI::Property("GPU", m_GPUTime);
				UI::Property("ImGui GPU", m_ImGuiGPUTime);
				UI::EndControlsGrid();

				ImGuiTableFlags tableFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable |
					                         ImGuiTableFlags_Sortable | ImGuiTableFlags_BordersV;

				enum
				{
					NameID = 0,
					TimeID = 1,
					AvgID = 2,
					MinID = 3,
					MaxID = 4,
					CallsID = 5,

				};

				const float contentRegionWidth = ImGui::GetContentRegionAvail().x;
				if (ImGui::BeginTable("##profilerTable", 6, tableFlags))
				{
					const ImGuiStyle& style = ImGui::GetStyle();
					const float with = ImGui::CalcTextSize("00.000ms").x + style.FramePadding.x * 2.0f + style.CellPadding.x * 2.0f;
					const float firstWidth = contentRegionWidth - with * 5;

					ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthStretch, 0.0f, NameID);
					ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, with, TimeID);
					ImGui::TableSetupColumn("Avg", ImGuiTableColumnFlags_WidthFixed, with, AvgID);
					ImGui::TableSetupColumn("Min", ImGuiTableColumnFlags_WidthFixed, with, MinID);
					ImGui::TableSetupColumn("Max", ImGuiTableColumnFlags_WidthFixed, with, MaxID);
					ImGui::TableSetupColumn("Calls", ImGuiTableColumnFlags_WidthFixed, with, CallsID);
					ImGui::TableHeadersRow();

					if (ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs())
					{
						if (sortSpecs->SpecsDirty || entriesChanged)
						{
							std::sort(m_ProfilerStats.begin(), m_ProfilerStats.end(), [sortSpecs](const ProfilerEntry& lhs, const ProfilerEntry& rhs)
							{
								for (int i = 0; i < sortSpecs->SpecsCount; i++)
								{
									const ImGuiTableColumnSortSpecs& colSortSpec = sortSpecs->Specs[i];
									float delta = 0.0f;
									switch (colSortSpec.ColumnUserID)
									{
										case NameID: delta = (float)lhs.Name.compare(rhs.Name); break;
										case TimeID: delta = lhs.Time - rhs.Time; break;
										case AvgID: delta = (lhs.Time / lhs.AvgSamples) - (rhs.Time / rhs.AvgSamples); break;
										case MinID: delta = lhs.Min - rhs.Min; break;
										case MaxID: delta = lhs.Max - rhs.Max; break;
										case CallsID: delta = lhs.AvgSamples - rhs.AvgSamples; break;
									}

									if (delta > 0.0f)
										return colSortSpec.SortDirection == ImGuiSortDirection_Descending;
									if (delta < 0.0f)
										return colSortSpec.SortDirection == ImGuiSortDirection_Ascending;
								}

								return lhs.Name < rhs.Name;
							});
							sortSpecs->SpecsDirty = false;
						}
					}

					for (const ProfilerEntry& entry : m_ProfilerStats)
					{
						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						ImGui::Text(entry.Name.c_str());
						ImGui::TableNextColumn();
						UI::Text("{0:.3}", entry.Time);
						ImGui::TableNextColumn();
						UI::Text("{0:.3}", entry.Time / entry.AvgSamples);
						ImGui::TableNextColumn();
						UI::Text("{0:.3}", entry.Min);
						ImGui::TableNextColumn();
						UI::Text("{0:.3}", entry.Max);
						ImGui::TableNextColumn();
						UI::Text("{0}", entry.AvgSamples);
					}

					ImGui::EndTable();
				}

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
