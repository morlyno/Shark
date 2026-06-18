#include "skpch.h"
#include "Skeleton.h"

#include <glm/gtx/transform.hpp>

namespace Shark {

	static Transform InverseTransform(const Transform& transform)
	{
		Transform result;
		result.Rotation = glm::inverse(transform.Rotation);
		result.Scale = 1.0f / transform.Scale;
		result.Translation = result.Rotation * (result.Scale * -transform.Translation);
		return result;
	}

	void Skeleton::Initialize()
	{
		const size_t boneCount = m_BoneNames.size();
		m_BoneLength.resize(boneCount);
		m_ModelSpaceRestPose.resize(boneCount);
		m_ModelSpaceInverseRestPose.resize(boneCount);
		m_ModelSpaceRestPose[0] = m_BoneTransform[0];
		m_ModelSpaceInverseRestPose[0] = InverseTransform(m_ModelSpaceRestPose[0]);

		for (size_t i = 1; i < boneCount; i++)
		{
			m_ModelSpaceRestPose[i].Translation = m_ModelSpaceRestPose[m_ParentBoneIndices[i]].Translation * m_BoneTransform[i].Translation;
			m_ModelSpaceRestPose[i].Rotation = m_ModelSpaceRestPose[m_ParentBoneIndices[i]].Rotation * m_BoneTransform[i].Rotation;
			m_ModelSpaceRestPose[i].Scale = m_ModelSpaceRestPose[m_ParentBoneIndices[i]].Scale * m_BoneTransform[i].Scale;
			m_ModelSpaceRestPose[i] = InverseTransform(m_ModelSpaceRestPose[i]);
		}

		for (uint32_t i = 0; i < boneCount; i++)
		{
			const auto& transform = m_ModelSpaceRestPose[i];
			auto children = GetChildBoneIndices(i);

			float length;
			if (children.empty())
			{
				if (m_ParentBoneIndices[i] != NullIndex)
					length = m_BoneLength[m_ParentBoneIndices[i]] * 0.5f;
				else
					length = 0.25f;
			}
			else
			{
				const auto& childTransform = m_ModelSpaceRestPose[children[0]];
				length = glm::length(childTransform.Translation - transform.Translation);
			}

			m_BoneLength[i] = length;
		}

	}

	size_t Skeleton::AddBone(std::string_view name, size_t parentIndex, const glm::vec3& translation, const glm::quat& rotation, float scale)
	{
		const size_t index = m_BoneNames.size();
		m_BoneNames.push_back(std::string(name));
		m_ParentBoneIndices.push_back(parentIndex);
		m_BoneTransform.push_back({ translation, rotation, scale });
		return index;
	}

	size_t Skeleton::GetBoneIndex(std::string_view name) const
	{
		const auto i = std::ranges::find(m_BoneNames, name);
		if (i != m_BoneNames.end())
			return std::distance(m_BoneNames.begin(), i);
		return NullIndex;
	}

	const Transform& Skeleton::GetModelSpaceRestPose(size_t boneIndex) const
	{
		return m_ModelSpaceRestPose[boneIndex];
	}

	const Transform& Skeleton::GetModelSpaceInverseRestPose(size_t boneIndex) const
	{
		return m_ModelSpaceInverseRestPose[boneIndex];
	}

	const Transform& Skeleton::GetRestposeTransform(size_t boneIndex) const
	{
		return m_BoneTransform[boneIndex];
	}

	const glm::mat4 Skeleton::GetRestposeTransformMatrix(size_t boneIndex) const
	{
		const auto& transform = m_BoneTransform[boneIndex];
		return glm::translate(transform.Translation) * glm::toMat4(transform.Rotation) * glm::scale(glm::vec3(transform.Scale));
	}

	std::vector<size_t> Skeleton::GetChildBoneIndices(size_t boneIndex)
	{
		std::vector<size_t> indices;
		for (size_t i = 0; i < m_ParentBoneIndices.size(); i++)
		{
			if (m_ParentBoneIndices[i] != boneIndex)
				continue;

			indices.push_back(i);
		}
		return indices;
	}

}
