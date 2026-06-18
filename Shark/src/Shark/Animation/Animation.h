#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Animation/Transform.h"

namespace Shark {
	class Skeleton;
}

namespace Shark {

	template<typename T>
	struct KeyFrame
	{
		float FrameTime;
		T Value;
	};

	struct Channel
	{
		std::vector<KeyFrame<glm::vec3>> Translations;
		std::vector<KeyFrame<glm::quat>> Rotations;
		std::vector<KeyFrame<float>> Scales;
		size_t Index;
	};

	class Animation
	{
	public:
		Animation(const Skeleton* skeleton, std::vector<Channel> channels, float duration);

		float GetDuration() const { return m_Duration; }
		size_t GetFrameCount() const { return m_Channels.front().Translations.size(); }
		const auto* GetSkeleton() const { return m_Skeleton; }
		const auto& GetChannels() const { return m_Channels; }

	private:
		const Skeleton* m_Skeleton = nullptr;
		std::vector<Channel> m_Channels;
		float m_Duration;

		friend class AssimpMeshImporter;
	};

}
