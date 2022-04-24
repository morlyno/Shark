#pragma once

#include <mono\metadata\object.h>

namespace Shark {

	class MonoCalls
	{
	public:
		static void RegsiterInternalCalls();
	};

	namespace InternalCalls {

		enum class LogLevel : uint16_t
		{
			Trace = 0,
			Debug = 1,
			Info = 2,
			Warn = 3,
			Error = 4,
			Critical = 5
		};

		void Log_LogLevel(LogLevel level, MonoString* message);

		void Matrix4_Inverse(glm::mat4* matrix, glm::mat4* out_Result);
		glm::mat4 Matrix4_Mul_MatMat(glm::mat4* lhs, glm::mat4* rhs);
		glm::vec4 Matrix4_Mul_MatVec(glm::mat4* lhs, glm::vec4* rhs);

	}

}
