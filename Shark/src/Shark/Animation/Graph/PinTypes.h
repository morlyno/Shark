#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Animation/Transform.h"

#include "Shark/NodeGraph/PinTypes.h"

namespace Shark::NodeGraph {

	namespace Types {

		struct IPose
		{
			float Duration;
			float TimePosition;
			uint32_t BoneCount;

			Transform* GetBoneTransforms() { return reinterpret_cast<Transform*>(this + 1); }
			const Transform* GetBoneTransforms() const { return reinterpret_cast<const Transform*>(this + 1); }
		};

	}

	inline choc::value::Type CreateTypeTransform()
	{
		auto type = choc::value::Type::createObject("Transform");
		type.addObjectMember("Translation", choc::value::Type::createVectorFloat32(3));
		type.addObjectMember("Rotation", choc::value::Type::createVectorFloat32(4));
		type.addObjectMember("Scale", choc::value::Type::createVectorFloat32(1));
		return type;
	}

	inline choc::value::Type CreateTypePose(uint32_t boneCount)
	{
		auto type = choc::value::Type::createObject("Pose");
		type.addObjectMember("Duration", choc::value::Type::createFloat32());
		type.addObjectMember("TimePosition", choc::value::Type::createFloat32());
		type.addObjectMember("BoneCount", choc::value::Type::createInt32());
		//type.addObjectMember("BoneTransforms", choc::value::Type::createArray(CreateTypeTransform(), boneCount));
		type.addObjectMember("BoneTransforms", choc::value::Type::createArray<float>(boneCount * 8)); // Transform has 8 floats
		static_assert(sizeof(Transform) == sizeof(float) * 8);
		return type;
	}

	inline choc::value::Value CreatePose(uint32_t boneCount)
	{
		return choc::value::Value(CreateTypePose(boneCount));
	}

}
