#pragma once

#include "Shark/Utils/PlatformUtils.h"
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

	struct Uninitialized {};
	static constexpr Uninitialized UninitializedTag;

	namespace Threading {

		class Signal
		{
		public:
			Signal(Uninitialized);
			Signal(bool manualReset, bool initialState);
			~Signal();

			Signal(Signal&& other);
			Signal& operator=(Signal&& other);

			Signal(const Signal&) = delete;
			Signal& operator=(const Signal&) = delete;

			void Set();
			void Reset();

			void Wait();
			void Wait(std::chrono::milliseconds time);

			operator bool() const;

		private:
			NativeHandle m_Handle = 0;
		};

		template<typename T>
		class Future
		{
		private:
			class FutureState
			{
			public:
				T m_Value;
				Signal m_FinishedEvent;
				std::vector<std::function<void(T)>> m_OnReadyCallbacks;
				std::atomic<bool> m_Finished = false;
				mutable std::mutex m_Mutex;

				FutureState(bool signaled);
				~FutureState() = default;

				FutureState(const FutureState&) = delete;
				FutureState& operator=(const FutureState&) = delete;
			};

		public:
			Future() = default;
			~Future() = default;

			Future(bool createState);
			explicit Future(T result);

			void MergeCallbacks(const Future& other);

			bool Valid() const { m_State != nullptr; }
			bool Ready() const { return m_State->m_Finished; }


			void Set(const T& value);
			void Signal(bool wake = true, bool callback = true);
			void SetAndSignal(const T& value);


			void Wait();
			void Wait(std::chrono::milliseconds milliseconds);

			const T& WaitAndGet();
			const T& Get();

			void OnReady(auto func);

		private:
			std::shared_ptr<FutureState> m_State;

			template<typename T>
			friend class Promise;
		};

	}

}

#include "Shark/Core/Threading.inl"
