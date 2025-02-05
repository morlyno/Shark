#include "skpch.h"
#include "Application.h"

#include "Shark/Core/TimeStep.h"
#include "Shark/Core/Timer.h"
#include "Shark/Asset/AssetManager.h"

#include "Shark/Input/Input.h"
#include "Shark/Render/Renderer.h"
#include "Shark/Scripting/ScriptEngine.h"

#include "Shark/File/FileSystem.h"
#include "Shark/Utils/String.h"
#include "Shark/Utils/PlatformUtils.h"
#include "Shark/Debug/Profiler.h"

namespace Shark {

	Application* Application::s_Instance = nullptr;

	Application::Application(const ApplicationSpecification& specification)
		: m_Specification(specification), m_MainThreadID(std::this_thread::get_id())
	{
		SK_PROFILE_FUNCTION();

		SK_CORE_VERIFY(!s_Instance, "Application allready set");
		s_Instance = this;
		Application* app = this;

		m_Profiler = sknew PerformanceProfiler;
		m_SecondaryProfiler = sknew PerformanceProfiler;

		Renderer::Init();
		Renderer::WaitAndRender();

		WindowSpecification windowSpec;
		windowSpec.Title = specification.Name;
		windowSpec.Width = specification.WindowWidth;
		windowSpec.Height = specification.WindowHeight;
		windowSpec.Decorated = specification.Decorated;
		windowSpec.CustomTitlebar = specification.CustomTitlebar;
		windowSpec.Fullscreen = specification.FullScreen;
		windowSpec.VSync = specification.VSync;
		m_Window = Window::Create(windowSpec, [app](Event& e) { app->OnEvent(e); });
		if (specification.Maximized)
			m_Window->Maximize();
		else
			m_Window->CenterWindow();

		m_ScriptHost.Initialize();
	}

	Application::~Application()
	{
		SK_PROFILE_FUNCTION();

		Renderer::WaitAndRender();

		m_LayerStack.Clear();
		m_ScriptHost.Shutdown();

		m_Window = nullptr;
		Renderer::ShutDown();

		skdelete m_Profiler;
		skdelete m_SecondaryProfiler;

		s_Instance = nullptr;
	}

	void Application::PushLayer(Layer* layer)
	{
		m_LayerStack.PushLayer(layer);
		layer->OnAttach();
	}

	void Application::PopLayer(Layer* layer)
	{
		layer->OnDetach();
		m_LayerStack.PopLayer(layer);
	}

	void Application::PushOverlay(Layer* layer)
	{
		m_LayerStack.PushOverlay(layer);
		layer->OnAttach();
	}

	void Application::PopOverlay(Layer* layer)
	{
		layer->OnDetach();
		m_LayerStack.PopOverlay(layer);
	}

	void Application::Run()
	{
		OnInitialize();
		m_State = ApplicationState::Running;
		m_LastTickCount = Platform::GetTicks();

		while (m_Running)
		{
			SK_PROFILE_MAIN_FRAME();
			SK_PROFILE_FUNCTION();

			Timer cpuTimer;
			TimeStep waitAndRenderTime;

			AssetManager::SyncWithAssetThread();

			ProcessEvents();
			ExecuteMainThreadQueue();

			if (!m_Minimized)
			{
				{
					Timer waitAndRenderTimer;
					Renderer::WaitAndRender();
					waitAndRenderTime = waitAndRenderTimer.Elapsed();
				}
				Renderer::BeginFrame();

				m_LayerStack.Invoke(&Layer::OnBeginFrame);
				m_LayerStack.Invoke(&Layer::OnUpdate, m_TimeStep);

				m_LayerStack.Invoke(&Layer::OnBeginUI);
				m_LayerStack.Invoke(&Layer::OnImGuiRender);
				m_LayerStack.Invoke(&Layer::OnEndUI);

				m_LayerStack.Invoke(&Layer::OnEndFrame);

				Renderer::EndFrame();

				Application* app = this;
				Renderer::Submit([app]() { app->m_Window->SwapBuffers(); });

				m_CPUTime = cpuTimer.Elapsed() - waitAndRenderTime;
			}

			const uint64_t ticks = Platform::GetTicks();
			m_TimeStep = (float)(ticks - m_LastTickCount) / Platform::GetTicksPerSecond();
			SK_LOG_IF(m_TimeStep > 1.0f, LoggerType::Core, LogLevel::Warn, "Core", "Large Timestep! {}", m_TimeStep);

			m_TimeStep = std::min<float>(m_TimeStep, 0.33f);
			m_LastTickCount = ticks;
			m_Time += m_TimeStep;

			if (m_SecondaryProfiler)
				m_SecondaryProfiler->Clear();
			std::swap(m_Profiler, m_SecondaryProfiler);

			m_FrameCount++;
		}
		
		m_State = ApplicationState::Shutdown;
		OnShutdown();
	}

	void Application::SubmitToMainThread(const std::function<void()>& func)
	{
		std::scoped_lock lock(m_MainThreadMutex);

		m_MainThreadQueue.push_back(func);
	}

	void Application::ProcessEvents()
	{
		SK_PROFILE_FUNCTION();

		Input::TransitionStates();

		m_Window->ProcessEvents();

		while (!m_EventQueue.empty())
		{
			auto& func = m_EventQueue.front();
			func();
			m_EventQueue.pop();
		}
	}

	void Application::ExecuteMainThreadQueue()
	{
		std::vector<std::function<void()>> queue;

		{
			std::scoped_lock lock(m_MainThreadMutex);

			queue = std::move(m_MainThreadQueue);
		}

		for (const auto& func : queue)
			func();
	}

	void Application::OnEvent(Event& event)
	{
		SK_PROFILE_FUNCTION();

#if 0
		if (m_Specification.EnableImGui)
		{
			event.Handled |= event.IsInCategory(EventCategory::Mouse) && m_ImGuiLayer->BlocksMouseEvents();
			event.Handled |= event.IsInCategory(EventCategory::Keyboard) && m_ImGuiLayer->BlocksKeyboardEvents();
		}
#endif

		Input::OnEvent(event);

		if (event.Handled)
			return;

		EventDispacher dispacher(event);
		dispacher.DispachEvent<WindowCloseEvent>(SK_BIND_EVENT_FN(Application::OnWindowClose));
		dispacher.DispachEvent<WindowMinimizedEvent>(SK_BIND_EVENT_FN(Application::OnWindowMinimized));
		dispacher.DispachEvent<WindowLostFocusEvent>(SK_BIND_EVENT_FN(Application::OnWindowLostFocus));

		for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend() && !event.Handled; ++it)
			(*it)->OnEvent(event);

	}

	bool Application::OnWindowClose(WindowCloseEvent& event)
	{
		SK_CORE_WARN("Window Closed");
		CloseApplication();

		// Note(moro): hack so that ImGui doesn't crash because the window is gone
		m_Minimized = true;

		return false;
	}

	bool Application::OnWindowMinimized(WindowMinimizedEvent& event)
	{
		m_Minimized = event.GetMinimized();
		return false;
	}

	bool Application::OnWindowLostFocus(WindowLostFocusEvent& event)
	{
		if (m_Window->IsFullscreen())
		{
			Application::Get().SubmitToMainThread([this]
			{
				m_Window->SetFullscreen(false);
				//auto swapchain = m_Window->GetSwapChain();
				//swapchain->Resize(m_Window->GetWidth(), m_Window->GetHeight());
			});
		}

		return false;
	}

	namespace utils {

		static void DumpMemory(std::ostream& out)
		{
			if (Allocator::GetAllocationStatsMap().empty())
				return;

			AllocatorData::AllocationStatsMap statsMap = Allocator::GetAllocationStatsMap();

			std::ostringstream tempOut;

			tempOut << "======================\n";
			tempOut << " Current Memory Usage \n";
			tempOut << "======================\n";
			tempOut << "Total allocated " << String::BytesToString(Allocator::GetMemoryStats().TotalAllocated) << '\n';
			tempOut << "Total freed " << String::BytesToString(Allocator::GetMemoryStats().TotalFreed) << '\n';
			tempOut << "Current Usage " << String::BytesToString(Allocator::GetMemoryStats().CurrentUsage()) << '\n';

			tempOut << "----------------------\n";

			struct Entry
			{
				std::string Descriptor;
				bool IsFile = false;
				std::string Size;
				uint64_t ByteSize;
				bool operator>(const Entry& rhs) const { return ByteSize > rhs.ByteSize; }
			};
			std::vector<Entry> entries;

			for (const auto& [desc, size] : statsMap)
			{
				if (size == 0)
					continue;

				Entry entry;
				entry.ByteSize = size;
				entry.Size = String::BytesToString(size);

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

				const auto where = std::lower_bound(entries.begin(), entries.end(), entry, std::greater<>{});
				entries.insert(where, entry);
			}

			if (entries.empty())
				return;

			for (const auto& entry : entries)
			{
				tempOut << fmt::format("  {} {}", entry.Descriptor, entry.Size) << '\n';
			}
			tempOut << "======================\n";

			out << tempOut.str();
		}

		static void DumpMemoryAllocations(std::ostream& out, bool ignoreEntriesWithNullDescriptor = false)
		{
			if (Allocator::GetAllocationMap().empty())
				return;

			AllocatorData::AllocationMap allocMap = Allocator::GetAllocationMap();

			out << "======================\n";
			out << " Current Allocations \n";
			out << "======================\n";

			for (const auto& [memory, alloc] : allocMap)
			{
				if (ignoreEntriesWithNullDescriptor && (!alloc.Descriptor || strcmp(alloc.Descriptor, "(null)") == 0))
					continue;

				auto makeDesc = [](const Allocation& alloc) -> std::string
				{
					std::string result = alloc.Descriptor;

					if (alloc.Line != -1)
						result += fmt::format("({})", alloc.Line);

					return result;
				};

				out << fmt::format("  {} {} {}\n", makeDesc(alloc), alloc.Memory, String::BytesToString(alloc.Size));
			}
			out << "======================\n";
		}

	}

	namespace Core {

		void Initialize()
		{
			SK_PROFILE_FUNCTION();
			Log::Initialize();
			Platform::Initialize();
			Input::Initialize();
			FileSystem::Initialize();

			SK_CORE_INFO("Core Initialized");
		}

		void Shutdown()
		{
			SK_PROFILE_FUNCTION();
			SK_CORE_INFO("Core Shutting down");

			FileSystem::Shutdown();
			Input::Shutdown();
			//Renderer::ReportLiveObejcts();
			Platform::Shutdown();
			Log::Shutdown();

			utils::DumpMemory(std::cout);
			//utils::DumpMemoryAllocations(std::cout);
		}

	}

}