#include "skpch.h"
#include "ScriptHost.h"

#include "Shark/File/FileSystem.h"

namespace Shark {

	ScriptHost::ScriptHost()
	{
	}

	ScriptHost::~ScriptHost()
	{
		if (m_Host)
		{
			Shutdown();
		}
	}

	void ScriptHost::Initialize()
	{
		Coral::HostSettings settings;
		settings.CoralDirectory = FileSystem::Absolute("DotNet").string();
		settings.MessageCallback = OnCoralMessage;
		settings.ExceptionCallback = OnCSException;

		m_Host = Scope<Coral::HostInstance>::Create();
		Coral::CoralInitStatus status = m_Host->Initialize(settings);

		if (status != Coral::CoralInitStatus::Success)
		{
			SK_CORE_ERROR_TAG("Scripting", "{}", status);
			return;
		}

		SK_CORE_INFO_TAG("Scripting", "Coral host initialized");
	}

	void ScriptHost::Shutdown()
	{
		m_Host->Shutdown();
		m_Host = nullptr;
	}

	Coral::AssemblyLoadContext ScriptHost::CreateAssemblyLoadContext(std::string_view name)
	{
		return m_Host->CreateAssemblyLoadContext(name);
	}

	void ScriptHost::DestroyAssemblyLoadContext(Coral::AssemblyLoadContext& loadContext)
	{
		m_Host->UnloadAssemblyLoadContext(loadContext);
	}

	void ScriptHost::OnCoralMessage(std::string_view message, Coral::MessageLevel level)
	{
		switch (level)
		{
			case Coral::MessageLevel::Trace: SK_CORE_TRACE_TAG("Scripting", "{}", message); break;
			case Coral::MessageLevel::Info: SK_CORE_INFO_TAG("Scripting", "{}", message); break;
			case Coral::MessageLevel::Warning: SK_CORE_WARN_TAG("Scripting", "{}", message); break;
			case Coral::MessageLevel::Error: SK_CORE_ERROR_TAG("Scripting", "{}", message); break;
		}
	}

	void ScriptHost::OnCSException(std::string_view message)
	{
		SK_CONSOLE_ERROR("C# Exception: {}", message);
	}

}
