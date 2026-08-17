#include "skpch.h"
#include "Pose.h"

namespace Shark {

	Scope<Pose> Pose::Allocate(uint32_t boneCount)
	{
		const uint64_t byteSize = sizeof(Pose) + sizeof(Transform) * boneCount;

		auto pose = static_cast<Pose*>(operator new(byteSize));
		new(pose) Pose();

		pose->BoneCount = boneCount;

		return pose;
	}

}
