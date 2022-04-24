#include "skpch.h"
#include "InternalCalls.h"

#include "Shark/Core/Log.h"

namespace Shark {

	void MonoCalls::RegsiterInternalCalls()
	{
		mono_add_internal_call("Shark.InternalCalls::Log_LogLevel(Shark.Log/Level,string)", &InternalCalls::Log_LogLevel);

		mono_add_internal_call("Shark.InternalCalls::Matrix4_Inverse(Shark.Matrix4&,Shark.Matrix4&)", &InternalCalls::Matrix4_Inverse);
		mono_add_internal_call("Shark.InternalCalls::Matrix4_Mul(Shark.Matrix4&,Shark.Matrix4&)", &InternalCalls::Matrix4_Mul_MatMat);
		mono_add_internal_call("Shark.InternalCalls::Matrix4_Mul(Shark.Matrix4&,Shark.Vector4&)", &InternalCalls::Matrix4_Mul_MatVec);
	}

	namespace InternalCalls {

		void Log_LogLevel(LogLevel level, MonoString* message)
		{
			char* msg = mono_string_to_utf8(message);
			switch (level)
			{
				case LogLevel::Trace: SK_TRACE(msg); break;
				case LogLevel::Debug: Shark::Log::GetClientLogger()->debug(msg); break;
				case LogLevel::Info: SK_INFO(msg); break;
				case LogLevel::Warn: SK_WARN(msg); break;
				case LogLevel::Error: SK_ERROR(msg); break;
				case LogLevel::Critical: SK_CRITICAL(msg); break;
			}
		}

		void Matrix4_Inverse(glm::mat4* matrix, glm::mat4* out_Result)
		{
			*out_Result = glm::inverse(*matrix);
		}

		glm::mat4 Matrix4_Mul_MatMat(glm::mat4* lhs, glm::mat4* rhs)
		{
			return *lhs * *rhs;
		}

		glm::vec4 Matrix4_Mul_MatVec(glm::mat4* lhs, glm::vec4* rhs)
		{
			return *lhs * *rhs;
		}

	}

}
