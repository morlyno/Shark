#pragma once

#include "Shark/Physics/PhysicsTypes.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/MotionType.h>

#include <glm/glm.hpp>

namespace Shark::JoltUtils {

	inline glm::vec3 ToGLM(JPH::RVec3Arg vec) { return { vec.GetX(), vec.GetY(), vec.GetZ() }; }
	inline glm::quat ToGLM(JPH::QuatArg quat) { return { quat.GetX(), quat.GetY(), quat.GetZ(), quat.GetW() }; }
	glm::mat4 ToGLM(JPH::RMat44Arg mat);

	inline JPH::RVec3 ToJPH(glm::vec3 vec) { return JPH::RVec3(vec.x, vec.y, vec.z); }
	inline JPH::Quat ToJPH(glm::quat quat) { return JPH::Quat(quat.x, quat.y, quat.z, quat.w); }

	JPH::EMotionType GetMotionType(BodyType bodyType);

}
