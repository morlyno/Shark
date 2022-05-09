#pragma once

#include "Shark/Core/Base.h"

namespace Shark {

	template<typename T = float>
	class Bounds2
	{
	public:
		using value_type = T;

	public:
		Bounds2() = default;
		Bounds2(const glm::tvec2<T>& lowerBound, const glm::tvec2<T>& upperBound)
			: LowerBound(lowerBound), UpperBound(upperBound)
		{
		}

		glm::tvec2<T> GetSize() const
		{
			return UpperBound - LowerBound;
		}

		T GetWidth() const
		{
			return UpperBound.x - LowerBound.x;
		}

		T GetHeight() const
		{
			return UpperBound.y - LowerBound.y;
		}

	public:
		glm::tvec2<T> LowerBound;
		glm::tvec2<T> UpperBound;

	};

	using Bounds2i = Bounds2<int>;
	using Bounds2u = Bounds2<uint32_t>;

}
