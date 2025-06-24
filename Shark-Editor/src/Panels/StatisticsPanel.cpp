#include "StatisticsPanel.h"

#include "Shark/Core/Application.h"
#include "Shark/Debug/Profiler.h"
#include "Shark/UI/UICore.h"
#include "Shark/UI/Widgets.h"
#include "Shark/UI/Controls.h"
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
			ImGui::Text(fmt::format("Total allocated {}", String::BytesToString(Allocator::GetMemoryStats().TotalAllocated)));
			ImGui::Text(fmt::format("Total freed {}", String::BytesToString(Allocator::GetMemoryStats().TotalFreed)));
			ImGui::Text(fmt::format("Current Usage {}", String::BytesToString(Allocator::GetMemoryStats().CurrentUsage())));
			ImGui::Text(fmt::format("Alive Allocations {}", Allocator::GetAllocationMap().size()));

			ImGui::Separator();

			static char SearchBuffer[250]{};
			UI::Widgets::Search(SearchBuffer);

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
				UI::ScopedColor color(ImGuiCol_Text, ImVec4(0.2f, 0.3f, 0.9f, 1.0f), entry.IsFile);
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

				const auto AddTime = [](auto& accumulator, TimeStep time, uint32_t samples)
				{
					accumulator.Time += time;
					accumulator.Min = std::min(time, accumulator.Min);
					accumulator.Max = std::max(time, accumulator.Max);
					accumulator.Samples += samples;
				};

				AddTime(m_ApplicationAccumulators[0], app.GetFrameTime(), 1);
				AddTime(m_ApplicationAccumulators[1], app.GetCPUTime(), 1);
				AddTime(m_ApplicationAccumulators[2], app.GetImGuiLayer().GetGPUTime(), 1);

				for (const auto& [name, perFrameData] : frameStorages)
				{
					auto& accumulator = m_ProfilerStatsAccumulator[std::string(name)];
					AddTime(accumulator, perFrameData.Time, perFrameData.Samples);
				}

				if (m_Scene->RunsPhysicsSimulation())
				{
					const auto& profile = m_Scene->GetPhysicsScene().GetProfile();
					AddTime(m_PhysicsAccumulators[0], profile.TimeStep, 1);
					AddTime(m_PhysicsAccumulators[1], profile.Step, profile.NumSteps);
					AddTime(m_PhysicsAccumulators[2], profile.Collide, profile.NumSteps);
					AddTime(m_PhysicsAccumulators[3], profile.Solve, profile.NumSteps);
					AddTime(m_PhysicsAccumulators[4], profile.SolveInit, profile.NumSteps);
					AddTime(m_PhysicsAccumulators[5], profile.SolveVelocity, profile.NumSteps);
					AddTime(m_PhysicsAccumulators[6], profile.SolvePosition, profile.NumSteps);
					AddTime(m_PhysicsAccumulators[7], profile.Broadphase, profile.NumSteps);
					AddTime(m_PhysicsAccumulators[8], profile.SolveTOI, profile.NumSteps);
				}

				if (++m_ProfilerSampleCount >= m_ProfilerSamples)
				{
					const auto& SetStat = [this](std::string_view name, ProfilerEntry& stats, const AccumulatorEntry& accumulator)
					{
						stats.Name = name;
						stats.Time = accumulator.Time / (float)m_ProfilerSamples;
						stats.Min = accumulator.Min;
						stats.Max = accumulator.Max;
						stats.AvgSamples = (float)accumulator.Samples / (float)m_ProfilerSamples;
					};

					const auto sorter = [](const ProfilerEntry& lhs, const ProfilerEntry& rhs) -> bool { return lhs.Name > rhs.Name; };

					m_ProfilerStats.clear();
					m_ProfilerStats.reserve(m_ProfilerStatsAccumulator.size());
					for (const auto& [name, accumulatorEntry] : m_ProfilerStatsAccumulator)
					{
						SetStat(name, m_ProfilerStats.emplace_back(), accumulatorEntry);
					}

					constexpr std::array names = { "Frame", "CPU", "ImGui GPU", };
					for (uint32_t index = 0; const char* name : names)
					{
						auto& stats = m_ApplicationStats[index];
						auto& accumulator = m_ApplicationAccumulators[index++];
						SetStat(name, stats, accumulator);
					}

					constexpr std::array physicsNames = { "TimeStep", "Step", "Collide", "Solve", "SolveInit", "SolveVelocity", "SolvePosition", "Broadphase", "SolveTOI" };
					for (uint32_t index = 0; const char* name : physicsNames)
					{
						auto& stats = m_PhysicsStats[index];
						auto& accumulator = m_PhysicsAccumulators[index++];
						SetStat(name, stats, accumulator);
					}

					m_ApplicationAccumulators.fill({});
					m_ProfilerStatsAccumulator.clear();
					m_PhysicsAccumulators.fill({});
					m_ProfilerSampleCount = 0;
					entriesChanged = true;
				}

				UI::BeginControlsGrid();
				UI::Control("Samples", m_ProfilerSamples);
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
					CallsID = 5
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

					const ImGuiTreeNodeFlags treeFlags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_SpanAllColumns | ImGuiTreeNodeFlags_DefaultOpen;

					const auto drawStats = [](std::span<const ProfilerEntry> stats)
					{
						for (const ProfilerEntry& entry : stats)
						{
							ImGui::TableNextRow();
							ImGui::TableNextColumn();
							ImGui::Text(entry.Name.c_str());
							ImGui::TableNextColumn();
							ImGui::Text(fmt::format("{0:.3f}", entry.Time));
							ImGui::TableNextColumn();
							ImGui::Text(fmt::format("{0:.3f}", entry.Time / entry.AvgSamples));
							ImGui::TableNextColumn();
							ImGui::Text(fmt::format("{0:.3f}", entry.Min));
							ImGui::TableNextColumn();
							ImGui::Text(fmt::format("{0:.3f}", entry.Max));
							ImGui::TableNextColumn();
							ImGui::Text(fmt::format("{0}", (uint32_t)glm::round(entry.AvgSamples)));
						}
					};

					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					if (ImGui::TreeNodeEx("Application", treeFlags))
					{
						drawStats(m_ApplicationStats);
						ImGui::TreePop();
					}

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::Separator();

					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					if (ImGui::TreeNodeEx("Profiler", treeFlags))
					{
						drawStats(m_ProfilerStats);
						ImGui::TreePop();
					}

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::Separator();

					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					if (ImGui::TreeNodeEx("Physics", treeFlags))
					{
						drawStats(m_PhysicsStats);
						ImGui::TreePop();
					}

					ImGui::EndTable();
				}
			}

			ImGui::EndTabItem();
		}
	}

}
