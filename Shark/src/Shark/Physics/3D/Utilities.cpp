#include "skpch.h"
#include "Utilities.h"

namespace Shark::JoltUtils {

	JPH::EMotionType GetMotionType(BodyType bodyType)
	{
		switch (bodyType)
		{
			case BodyType::Static: return JPH::EMotionType::Static;
			case BodyType::Dynamic: return JPH::EMotionType::Dynamic;
			case BodyType::Kinematic: return JPH::EMotionType::Kinematic;
		}
		SK_CORE_VERIFY(false, "Unknown RigidBodyType");
		return JPH::EMotionType::Static;
	}

	glm::mat4 ToGLM(JPH::RMat44Arg mat)
	{
		glm::mat4 result;
		result[0] = { mat(0, 0), mat(1, 0), mat(2, 0) , mat(3, 0) };
		result[1] = { mat(0, 1), mat(1, 1), mat(2, 1) , mat(3, 1) };
		result[2] = { mat(0, 2), mat(1, 2), mat(2, 2) , mat(3, 2) };
		result[3] = { mat(0, 3), mat(1, 3), mat(2, 3) , mat(3, 3) };
		return result;
	}

	JPH::EMotionQuality GetMotionQuality(CollisionDetectionType collisionDetection)
	{
		switch (collisionDetection)
		{
			case CollisionDetectionType::Discrete: return JPH::EMotionQuality::Discrete;
			case CollisionDetectionType::Continuous: return JPH::EMotionQuality::LinearCast;
		}
		SK_CORE_VERIFY(false, "Unkown CollisionDetectionType");
		return JPH::EMotionQuality::Discrete;
	}

	JPH::EAllowedDOFs ConvertLockedAxesToAllowedDOFs(Axis lockedAxes)
	{
		JPH::EAllowedDOFs dofs = JPH::EAllowedDOFs::All;
		if ((lockedAxes & Axis::TranslationX) == Axis::TranslationX) dofs &= ~JPH::EAllowedDOFs::TranslationX;
		if ((lockedAxes & Axis::TranslationY) == Axis::TranslationY) dofs &= ~JPH::EAllowedDOFs::TranslationY;
		if ((lockedAxes & Axis::TranslationZ) == Axis::TranslationZ) dofs &= ~JPH::EAllowedDOFs::TranslationZ;
		if ((lockedAxes & Axis::RotationX) == Axis::RotationX) dofs &= ~JPH::EAllowedDOFs::RotationX;
		if ((lockedAxes & Axis::RotationY) == Axis::RotationY) dofs &= ~JPH::EAllowedDOFs::RotationY;
		if ((lockedAxes & Axis::RotationZ) == Axis::RotationZ) dofs &= ~JPH::EAllowedDOFs::RotationZ;
		return dofs;
	}

}
