#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Animation/Transform.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Shark {

	class Skeleton
	{
	public:
		void Initialize();
		size_t AddBone(std::string_view name, size_t parentIndex, const glm::vec3& translation, const glm::quat& rotation, float scale);

		size_t GetBoneCount() const { return m_BoneNames.size(); }
		size_t GetBoneIndex(std::string_view name) const;
		size_t GetParentBoneIndex(size_t index) const { return m_ParentBoneIndices[index]; }
		const std::string& GetBoneName(size_t index) const { return m_BoneNames[index]; }
		const auto& GetBoneNames() const { return m_BoneNames; }

		const glm::vec3& GetBoneTranslation(size_t index) const { return m_BoneTransform[index].Translation; }
		const glm::quat& GetBoneRotation(size_t index) const { return m_BoneTransform[index].Rotation; }
		float            GetBoneScale(size_t index) const { return m_BoneTransform[index].Scale; }

		const Transform& GetModelSpaceRestPose(size_t boneIndex) const;
		const Transform& GetModelSpaceInverseRestPose(size_t boneIndex) const;
		const Transform& GetRestposeTransform(size_t boneIndex) const;
		const glm::mat4 GetRestposeTransformMatrix(size_t boneIndex) const;

		static constexpr size_t NullIndex = ~0;

	private:
		std::vector<size_t> GetChildBoneIndices(size_t boneIndex);

	private:
		std::vector<std::string> m_BoneNames;
		std::vector<size_t> m_ParentBoneIndices;

		std::vector<Transform> m_BoneTransform;
		std::vector<Transform> m_ModelSpaceRestPose;
		std::vector<Transform> m_ModelSpaceInverseRestPose;
		std::vector<float> m_BoneLength;
	};

}
