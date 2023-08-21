#include "skpch.h"
#include "Application.h"

#include "Shark/Core/TimeStep.h"
#include "Shark/Core/Timer.h"

#include "Shark/Event/KeyEvent.h"
#include "Shark/Input/Input.h"
#include "Shark/Render/Renderer.h"
#include "Shark/Scripting/ScriptEngine.h"

#include "Shark/File/FileSystem.h"
#include "Shark/Utils/String.h"
#include "Shark/Utils/TimeUtils.h"
#include "Shark/Utils/PlatformUtils.h"
#include "Shark/Debug/Profiler.h"

#if SK_PROFILER == SK_PROFILER_OPTICK
#include <optick.h>
#include <optick_memory.h>
#endif

namespace Shark {

	Application* Application::s_Instance = nullptr;

	Application::Application(const ApplicationSpecification& specification)
		: m_Specification(specification)
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
		windowSpec.Fullscreen = specification.FullScreen;
		windowSpec.VSync = specification.VSync;
		windowSpec.EventListener = Ref<EventListener>::Create([app](Event& e) { app->OnEvent(e); });
		m_Window = Window::Create(windowSpec);
		if (specification.Maximized)
			m_Window->Maximize();
		else
			m_Window->CenterWindow();

		if (specification.EnableImGui)
		{
			m_ImGuiLayer = ImGuiLayer::Create();
			PushOverlay(m_ImGuiLayer);
		}

		ScriptEngine::Init(specification.ScriptConfig);
	}

	Application::~Application()
	{
		SK_PROFILE_FUNCTION();

		m_LayerStack.Clear();
		ScriptEngine::Shutdown();

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
		m_LayerStack.PopLayer(layer);
		layer->OnDetach();
	}

	void Application::PushOverlay(Layer* layer)
	{
		m_LayerStack.PushOverlay(layer);
		layer->OnAttach();
	}

	void Application::PopOverlay(Layer* layer)
	{
		m_LayerStack.PopOverlay(layer);
		layer->OnDetach();
	}

	void Application::Run()
	{
		OnInit();
		m_State = ApplicationState::Running;
		m_LastTickCount = PlatformUtils::GetTicks();

		while (m_Running)
		{
			SK_PROFILE_FRAME("MainThread");

			Timer cpuTimer;

			ProcessEvents();

			if (!m_Minimized)
			{
				Renderer::BeginFrame();
				ExecuteMainThreadQueue();

				ScriptEngine::Update();

				for (auto& layer : m_LayerStack)
					layer->OnUpdate(m_TimeStep);

				if (m_Specification.EnableImGui)
				{
					Application* app = this;
					Renderer::Submit([app]() { app->RenderImGui(); });
				}

				Renderer::EndFrame();

				// On Render Thread
				Renderer::WaitAndRender();

				m_CPUTime = cpuTimer.Elapsed();
				m_GPUTime = Renderer::GetRendererAPI()->GetGPUTime();
				m_Window->SwapBuffers();
				m_FrameCount++;
			}

			const uint64_t ticks = PlatformUtils::GetTicks();
			m_TimeStep = (float)(ticks - m_LastTickCount) / PlatformUtils::GetTicksPerSecond();
			SK_LOG_IF(m_TimeStep > 1.0f, Log::Logger::Core, Log::Level::Warn, Tag::Core, "Large Timestep! {}", m_TimeStep);
			m_TimeStep = std::min<float>(m_TimeStep, 0.33f);
			m_LastTickCount = ticks;
			m_Time += m_TimeStep;

			if (m_SecondaryProfiler)
				m_SecondaryProfiler->Clear();
			std::swap(m_Profiler, m_SecondaryProfiler);
		}
		
		m_State = ApplicationState::Shutdown;
	}

	void Application::QueueEvent(const std::function<void()>& func)
	{
		m_EventQueue.push(func);
	}

	void Application::AddEventCallback(const std::function<bool(Event&)>& func)
	{
		m_EventCallbacks.push_back(func);
	}

	void Application::SubmitToMainThread(const std::function<void()>& func)
	{
		std::scoped_lock lock(m_MainThreadMutex);

		m_MainThreadQueue.push_back(func);
	}

	void Application::RenderImGui()
	{
		m_ImGuiLayer->Begin();

		for (auto& layer : m_LayerStack)
			layer->OnImGuiRender();

		m_ImGuiLayer->End();
	}

	void Application::ProcessEvents()
	{
		SK_PROFILE_FUNCTION();

		FileSystem::ProcessEvents();
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

		if (m_Specification.EnableImGui)
		{
			event.Handled |= event.IsInCategory(EventCategory::Mouse) && m_ImGuiLayer->BlocksMouseEvents();
			event.Handled |= event.IsInCategory(EventCategory::Keyboard) && m_ImGuiLayer->BlocksKeyboardEvents();
		}

		Input::OnEvent(event);

		if (event.Handled)
			return;

		EventDispacher dispacher(event);
		dispacher.DispachEvent<WindowCloseEvent>(SK_BIND_EVENT_FN(Application::OnWindowClose));
		dispacher.DispachEvent<WindowResizeEvent>(SK_BIND_EVENT_FN(Application::OnWindowResize));
		dispacher.DispachEvent<WindowLostFocusEvent>(SK_BIND_EVENT_FN(Application::OnWindowLostFocus));

		for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend() && !event.Handled; ++it)
			(*it)->OnEvent(event);

		for (const auto& callback : m_EventCallbacks)
			callback(event);
	}

	bool Application::OnWindowClose(WindowCloseEvent& event)
	{
		SK_CORE_WARN("Window Closed");
		CloseApplication();

		// Note(moro): hack so thack ImGui dosn't crash because the window dosn't exist anymore
		m_Minimized = true;
		return false;
	}

	bool Application::OnWindowResize(WindowResizeEvent& event)
	{
		m_Minimized = event.IsMinimized();
		if (m_Minimized)
			return false;

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
			tempOut << "Total allocated " << Utils::BytesToString(Allocator::GetMemoryStats().TotalAllocated) << '\n';
			tempOut << "Total freed " << Utils::BytesToString(Allocator::GetMemoryStats().TotalFreed) << '\n';
			tempOut << "Current Usage " << Utils::BytesToString(Allocator::GetMemoryStats().CurrentUsage()) << '\n';

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
				entry.Size = Utils::BytesToString(size);

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

				out << fmt::format("  {} {} {}\n", makeDesc(alloc), alloc.Memory, Utils::BytesToString(alloc.Size));
			}
			out << "======================\n";
		}

	}

	namespace Core {

		void Initialize()
		{
			Log::Initialize();
			Input::Initialize();
			FileSystem::Initialize();

			SK_CORE_INFO("Core Initialized");
		}

		void Shutdown()
		{
			SK_CORE_INFO("Core Shutting down");

			FileSystem::Shutdown();
			Input::Shutdown();
			Renderer::ReportLiveObejcts();
			Log::Shutdown();

			utils::DumpMemory(std::cout);
			//utils::DumpMemoryAllocations(std::cout);
		}

	}

}