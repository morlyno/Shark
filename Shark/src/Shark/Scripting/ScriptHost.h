#pragma once

#include "Shark/Core/Base.h"
#include <Coral/HostInstance.hpp>

namespace Shark {

	class ScriptHost
	{
	public:
		ScriptHost();
		~ScriptHost();

		void Initialize();
		void Shutdown();

		bool IsInitialized() const { return m_Host && m_InitStatus == Coral::CoralInitStatus::Success; }
		Coral::CoralInitStatus GetCoralInitStatus() const { return m_InitStatus; }

		Coral::AssemblyLoadContext CreateAssemblyLoadContext(std::string_view name);
		void DestroyAssemblyLoadContext(Coral::AssemblyLoadContext& loadContext);

	private:
		static void OnCoralMessage(std::string_view message, Coral::MessageLevel level);
		static void OnCSException(std::string_view message);

	private:
		Scope<Coral::HostInstance> m_Host = nullptr;
		Coral::CoralInitStatus m_InitStatus = Coral::CoralInitStatus::Success;
	};

}
