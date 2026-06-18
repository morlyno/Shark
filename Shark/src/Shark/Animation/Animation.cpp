#include "skpch.h"
#include "Animation.h"

namespace Shark {

	Animation::Animation(const Skeleton* skeleton, std::vector<Channel> channels, float duration)
		: m_Channels(std::move(channels)), m_Skeleton(skeleton), m_Duration(duration)
	{
	}

}
