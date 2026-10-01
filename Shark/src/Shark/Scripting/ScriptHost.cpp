#include "skpch.h"
#include "ScriptHost.h"

#include "Shark/File/FileSystem.h"
#include "Shark/Debug/Profiler.h"

namespace Shark {

	ScriptHost::ScriptHost()
	{
		SK_PROFILE_FUNCTION();

		Coral::HostSettings settings;
		settings.CoralDirectory = FileSystem::Absolute("DotNet").string();
		settings.MessageCallback = OnCoralMessage;
		settings.ExceptionCallback = OnCSException;

		m_Host = Scope<Coral::HostInstance>::Create();
		Coral::CoralInitStatus status = m_Host->Initialize(settings);

		if (status != Coral::CoralInitStatus::Success)
		{
			SK_CORE_ERROR_TAG(Log::Tag::Scripting, "{}", status);
			return;
		}

		SK_CORE_INFO_TAG(Log::Tag::Scripting, "Coral host initialized");
	}

	ScriptHost::~ScriptHost()
	{
		SK_PROFILE_FUNCTION();

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
			case Coral::MessageLevel::Trace: SK_CORE_TRACE_TAG(Log::Tag::Scripting, "{}", message); break;
			case Coral::MessageLevel::Info: SK_CORE_INFO_TAG(Log::Tag::Scripting, "{}", message); break;
			case Coral::MessageLevel::Warning: SK_CORE_WARNING_TAG(Log::Tag::Scripting, "{}", message); break;
			case Coral::MessageLevel::Error: SK_CORE_ERROR_TAG(Log::Tag::Scripting, "{}", message); break;
		}
	}

	void ScriptHost::OnCSException(std::string_view message)
	{
		SK_USER_ERROR("C# Exception: {}", message);
	}

}
