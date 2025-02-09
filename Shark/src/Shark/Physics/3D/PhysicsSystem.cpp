#include "skpch.h"
#include "PhysicsSystem.h"

#include "Shark/Scene/Scene.h"

#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/JobSystemThreadPool.h>

namespace Shark {

	static void TraceFunction(const char* fmt, ...)
	{
		// Format the message
		va_list list;
		va_start(list, fmt);
		char buffer[1024];
		vsnprintf(buffer, sizeof(buffer), fmt, list);
		va_end(list);

		SK_CORE_TRACE_TAG("Jolt", buffer);
	}

	// bool(*)(const char *inExpression, const char *inMessage, const char *inFile, uint inLine)
	static bool AssertFailedFunc(const char* expression, const char* message, const char* file, JPH::uint line)
	{
		SK_CORE_ERROR_TAG("Jolt", "Expression '{}' failed! {} {}:{} ", expression, message, file, line);
		return true;
	}

	struct SystemData
	{
		JPH::TempAllocatorImpl* m_TempAllocator;
		JPH::JobSystemThreadPool* m_JobSystem;
	};
	static SystemData* s_SystemData = nullptr;

	void PhysicsSystem::Initialize()
	{
		SK_CORE_VERIFY(s_SystemData == nullptr);
		JPH::RegisterDefaultAllocator();
		JPH::Trace = &TraceFunction;
		JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = &AssertFailedFunc);

		SK_CORE_VERIFY(JPH::Factory::sInstance == nullptr);
		JPH::Factory::sInstance = new JPH::Factory();
		JPH::RegisterTypes();

		s_SystemData = sknew SystemData;
		s_SystemData->m_TempAllocator = new JPH::TempAllocatorImpl(10 * 2024 * 2024);
		s_SystemData->m_JobSystem = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, 6);
	}

	void PhysicsSystem::Shutdown()
	{
		delete s_SystemData->m_JobSystem;
		delete s_SystemData->m_TempAllocator;
		skdelete s_SystemData;
		s_SystemData = nullptr;

		delete JPH::Factory::sInstance;
		JPH::Factory::sInstance = nullptr;
	}

	Ref<PhysicsScene> PhysicsSystem::CreateScene(Ref<Scene> scene)
	{
		auto system = Ref<PhysicsScene>::Create(scene);
		return system;
	}

	JPH::TempAllocator* PhysicsSystem::GetAllocator()
	{
		return s_SystemData->m_TempAllocator;
	}

	JPH::JobSystem* PhysicsSystem::GetJobSystem()
	{
		return s_SystemData->m_JobSystem;
	}

}
