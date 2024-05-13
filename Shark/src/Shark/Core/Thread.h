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
		class Future
		{
			class FutureState
			{
			public:
				T m_Value;
				HANDLE m_FinishedEvent = NULL;
				std::vector<std::function<void(T)>> m_OnReadyCallbacks;
				std::atomic<bool> m_Finished = false;
				std::atomic<uint32_t> m_WaitingCount = 0;

				FutureState() = default;
				~FutureState()
				{
					if (m_FinishedEvent)
						CloseHandle(m_FinishedEvent);
				}

				FutureState(const FutureState&) = delete;
				FutureState& operator=(const FutureState&) = delete;
			};

		public:
			Future() = default;
			~Future() = default;

			Future(bool createState)
			{
				if (createState)
				{
					m_State = std::make_shared<FutureState>();
					m_State->m_FinishedEvent = CreateEvent(nullptr, true, false, nullptr);
				}
			}

			explicit Future(T result)
			{
				m_State = std::make_shared<FutureState>();
				m_State->m_Value = result;
				m_State->m_Finished = true;
			}

			bool Valid() const { m_State != nullptr; }
			bool Ready() const { return m_State->m_Finished; }
			bool AnyWaiting() const { return m_State->m_WaitingCount; }

			/////////////////////////////////////////////////////
			/// Set Interface ///////////////////////////////////
			void Set(const T& value)
			{
				SK_CORE_VERIFY(m_State->m_Finished == false);
				m_State->m_Value = value;
				m_State->m_Finished = true;
			}

			void Signal(bool wake = true, bool callback = true)
			{
				if (wake)
				{
					SetEvent(m_State->m_FinishedEvent);
				}

				if (callback)
				{
					for (const auto& callback : m_State->m_OnReadyCallbacks)
					{
						callback(m_State->m_Value);
					}
				}
			}

			void SetAndSignal(const T& value)
			{
				Set(value);
				Signal();
			}

			/////////////////////////////////////////////////////
			/// Get Interface ///////////////////////////////////
			void Wait()
			{
				m_State->m_WaitingCount++;
				WaitForSingleObject(m_State->m_FinishedEvent, INFINITE);
				m_State->m_WaitingCount--;
			}

			void WaitFor(uint64_t milliseconds)
			{
				m_State->m_WaitingCount++;
				WaitForSingleObject(m_State->m_FinishedEvent, milliseconds);
				m_State->m_WaitingCount--;
			}

			const T& WaitAndGet()
			{
				Wait();
				return m_State->m_Value;
			}

			const T& Get()
			{
				SK_CORE_VERIFY(m_State->m_Finished);
				return m_State->m_Value;
			}

			void OnReady(auto func)
			{
				if (m_State->m_Finished)
					func(m_State->m_Value);

				m_State->m_OnReadyCallbacks.push_back(func);
			}

		private:
			std::shared_ptr<FutureState> m_State;

			template<typename T>
			friend class Promise;
		};

	}

}
