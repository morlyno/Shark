#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Animation/Transform.h"

#include <span>

namespace Shark {

	struct Pose
	{
		static Scope<Pose> Allocate(uint32_t boneCount);

		float Duration     = 0.0f;
		float TimePosition = 0.0f;
		uint32_t BoneCount = 0;

		std::span<Transform>       GetBoneTransforms()       { return std::span(reinterpret_cast<Transform*>(this + 1), BoneCount); }
		std::span<const Transform> GetBoneTransforms() const { return std::span(reinterpret_cast<const Transform*>(this + 1), BoneCount); }

	private:
		Pose() = default;
		Pose(const Pose&) = delete;
		Pose& operator=(const Pose&) = delete;
	};

}
