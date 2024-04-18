#pragma once

#include <future>

namespace Shark {

	class Thread
	{
	public:
		Thread(const std::string& name);
		~Thread();

		template<typename TFunc, typename... TArgs>
		void Dispacht(const TFunc& func, TArgs&&... args)
		{
			m_Thread = std::thread(func, std::forward<TArgs>(args)...);
			SetName(m_Name);
		}

		void Join();
		void SetName(const std::string& name);

	private:
		std::string m_Name;
		std::thread m_Thread;
	};

	namespace Threading {

		template<typename T>
		class PromiseState
		{
		public:
			T m_Value;
			HANDLE m_FinishedEvent = NULL;
			std::vector<std::function<void(T)>> m_OnReadyCallbacks;
			std::atomic<bool> m_Finished = false;

			PromiseState() = default;
			~PromiseState()
			{
				if (m_FinishedEvent)
					CloseHandle(m_FinishedEvent);
			}

			PromiseState(const PromiseState&) = delete;
			PromiseState& operator=(const PromiseState&) = delete;
		};

		template<typename T>
		class Future
		{
		public:
			Future() = default;
			~Future() = default;

			explicit Future(T result)
			{
				m_State = std::make_shared<PromiseState<T>>();
				m_State->m_Value = result;
				m_State->m_Finished = true;
			}

			bool Valid() const { m_State != nullptr; }
			bool Ready() const { return m_State->m_Finished; }

			void Wait() { WaitForSingleObject(m_State->m_FinishedEvent, INFINITE); }
			void WaitFor(uint64_t milliseconds) { WaitForSingleObject(m_State->m_FinishedEvent, milliseconds); }

			T WaitAndGet()
			{
				Wait();
				return m_State->m_Value;
			}

			void OnReady(auto func)
			{
				if (m_State->m_Finished)
					func(m_State->m_Value);

				m_State->m_OnReadyCallbacks.push_back(func);
			}

		private:
			Future(std::shared_ptr<PromiseState<T>> state)
				: m_State(state) {}

		private:
			std::shared_ptr<PromiseState<T>> m_State;

			template<typename T>
			friend class Promise;
		};

		template<typename T>
		class Promise
		{
		public:
			Promise()
			{
				m_State = std::make_shared<PromiseState<T>>();
				m_State->m_FinishedEvent = CreateEvent(nullptr, true, false, nullptr);
			}

			Future<T> GetFuture()
			{
				return Future(m_State);
			}

			void SetValue(const T& value)
			{
				SK_CORE_VERIFY(m_State->m_Finished == false);
				m_State->m_Value = value;
				m_State->m_Finished = true;
				SetEvent(m_State->m_FinishedEvent);
				for (const auto& callback : m_State->m_OnReadyCallbacks)
					callback(m_State->m_Value);
			}

		private:
			std::shared_ptr<PromiseState<T>> m_State;

		};


	}

}
